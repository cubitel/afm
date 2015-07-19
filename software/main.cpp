/* Copyright (c) 2015 Vasily Voropaev <vvg@cubitel.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#include <wx/wx.h>
#include <wx/intl.h>
#include <wx/progdlg.h>

#include "device.h"
#include "image.h"
#include "dfu.h"


#define UPDATE_TIMER_CONNECTED		500
#define UPDATE_TIMER_DISCONNECTED	2000


class MyApp: public wxApp
{
public:
	Device *afm;
    virtual bool OnInit();
    virtual int OnExit();
protected:
    wxLanguage m_lang;  // language specified by user
    wxLocale m_locale;  // locale we'll be using
private:
	void CheckDFU();
};

class MainFrame: public wxFrame
{
public:
    MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size);
    void UpdateAFMState();
private:
    wxTimer *tmrUpdate;
    wxStaticText *stMicroType;
    wxStaticText *stHeightControl;
    wxStaticText *stHeightValue;
    wxButton *btnStart;
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnRun(wxCommandEvent& event);
    void OnUpdateTimer(wxTimerEvent& evt);
    wxDECLARE_EVENT_TABLE();
};

enum
{
    ID_Run = 1,
    ID_UpdateTimer
};

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(wxID_EXIT,  MainFrame::OnExit)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
    EVT_COMMAND(ID_Run, wxEVT_COMMAND_BUTTON_CLICKED, MainFrame::OnRun)
    EVT_TIMER(ID_UpdateTimer, MainFrame::OnUpdateTimer)
wxEND_EVENT_TABLE()

wxIMPLEMENT_APP(MyApp);



bool MyApp::OnInit()
{
	int i;
	char buf[100];

    if ( !wxApp::OnInit() )
        return false;

    m_lang = wxLANGUAGE_DEFAULT;
    m_locale.Init(m_lang, wxLOCALE_DONT_LOAD_DEFAULT);

    wxLocale::AddCatalogLookupPathPrefix("./lang");

    m_locale.AddCatalog("messages");
    m_locale.AddCatalog("wxstd");

    /* Create AFM device instance */
    afm = new Device();

    if (!afm->Connect()) CheckDFU();

    MainFrame *frame = new MainFrame(_("AFM Control"),
    		wxPoint(50, 50),
    		wxSize(450, 340) );
    frame->Show( true );

    return true;
}

int MyApp::OnExit()
{
	afm->Disconnect();
	return 0;
}

void MyApp::CheckDFU()
{
	if (!dfuOpen()) return;

	do {
		int answer = wxMessageBox(
				_("Working device is not found, but empty STM32 device is found. Do you want to download AFM firmware to that device?"),
				_("Firmware update"),
				wxYES_NO, NULL);

		if (answer != wxYES) break;

		wxFileDialog openFile(NULL, _("Open firmware file"), "", "firmware.bin",
				"Firmware files (*.bin)|*.bin", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		if (openFile.ShowModal() == wxID_CANCEL) break;

	} while(0);

	dfuClose();
}


MainFrame::MainFrame(const wxString& title, const wxPoint& pos, const wxSize& size)
        : wxFrame(NULL, wxID_ANY, title, pos, size, wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN)
{
	/* Main menu */

    wxMenu *menuFile = new wxMenu;
    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, _("&File") );
    menuBar->Append( menuHelp, _("&Help") );
    SetMenuBar( menuBar );

    /* Status bar */

    CreateStatusBar();
   	SetStatusText("");

   	/* Status controls */
   	wxFlexGridSizer *grid;

   	wxPanel *panel = new wxPanel(this);

   	wxBoxSizer *cols = new wxBoxSizer(wxHORIZONTAL);

   	/* Status TextBox */
   	wxStaticBoxSizer *statusBox = new wxStaticBoxSizer(wxVERTICAL, panel, _("Status"));
   	grid = new wxFlexGridSizer(1, 2, 5, 5);
   	/* Microscope type */
   	grid->Add(new wxStaticText(statusBox->GetStaticBox(), wxID_ANY, _("Microscope type:")) );
   	stMicroType = new wxStaticText(statusBox->GetStaticBox(), wxID_ANY, "");
   	grid->Add(stMicroType);

   	statusBox->Add(grid, 0, 0);

   	cols->Add(statusBox, 0, wxALL, 5);

   	/* Probe TextBox */
   	wxStaticBoxSizer *probeBox = new wxStaticBoxSizer(wxVERTICAL, panel, _("Probe"));
   	grid = new wxFlexGridSizer(2, 2, 5, 5);

   	/* Height control mode */
   	grid->Add(new wxStaticText(probeBox->GetStaticBox(), wxID_ANY, _("Height control:")) );
   	stHeightControl = new wxStaticText(probeBox->GetStaticBox(), wxID_ANY, _("OFF"));
   	grid->Add(stHeightControl);
   	/* Height value */
   	grid->Add(new wxStaticText(probeBox->GetStaticBox(), wxID_ANY, _("Height:")) );
   	stHeightValue = new wxStaticText(probeBox->GetStaticBox(), wxID_ANY, "N/A");
   	grid->Add(stHeightValue);

   	probeBox->Add(grid, 0, 0);

   	cols->Add(probeBox, 0, wxALL, 5);

   	/* Scan TextBox */
   	wxStaticBoxSizer *scanBox = new wxStaticBoxSizer(wxVERTICAL, panel, _("Scan"));
   	btnStart = new wxButton(panel, ID_Run, _("Start"));
   	scanBox->Add(btnStart, 0, 0);

   	cols->Add(scanBox, 0, wxALL, 5);

   	panel->SetSizer(cols);

   	/* Update status */
   	tmrUpdate = new wxTimer(this, ID_UpdateTimer);
   	UpdateAFMState();
}

void MainFrame::UpdateAFMState()
{
	Device *afm;

	afm = wxGetApp().afm;

	if (!afm->IsConnected())
		afm->Connect();

	if (afm->IsConnected()) {
		afm->UpdateStatus();

	   	SetStatusText(_("Connected"));
	   	stMicroType->SetLabel(_("STM"));

	   	/* Restart update timer */
	   	tmrUpdate->StartOnce(UPDATE_TIMER_CONNECTED);
	} else {
	   	SetStatusText(_("Device is not connected or driver is not installed"));
	   	stMicroType->SetLabel("");

	   	/* Restart update timer */
	   	tmrUpdate->StartOnce(UPDATE_TIMER_DISCONNECTED);
	}
}

void MainFrame::OnExit(wxCommandEvent& event)
{
    Close( true );
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox( _("Atomic Force Microscope control program."),
                  _("About"), wxOK | wxICON_INFORMATION );
}

void MainFrame::OnRun(wxCommandEvent& event)
{
	Device *afm;
	AFMImage *image;

	/* Start scanning */
	afm = wxGetApp().afm;
	if (!afm->Run(0, 0, 100, 100)) {
		wxMessageBox( _("Failed to start a scan. Check parameters and try again."),
				_("Scanning"), wxOK | wxICON_ERROR);
		return;
	}

	/* Create image object */
	image = new AFMImage();

	wxProgressDialog progress( _("Scanning"), _("Retrieving image data") );
	do {
		/* Read image from the device */
		if (!afm->ReadImage(image, NULL)) {
			wxMessageBox( _("Failed to retrieve scan image."),
					_("Scanning"), wxOK | wxICON_ERROR);
			break;
		}

		/* Hide progress dialog */
		progress.Update(100);

		/* Save file dialog */
		wxFileDialog saveFile(NULL, _("Save image"), "", "",
				"Gwyddion Simple Field (*.gsf)|*.gsf", wxFD_SAVE);

		if (saveFile.ShowModal() == wxID_CANCEL) break;

		/* Save image to file */
		image->SaveAsGSF(saveFile.GetFilename().ToStdString());
	} while (0);

	delete image;
}

void MainFrame::OnUpdateTimer(wxTimerEvent& evt)
{
	UpdateAFMState();
}
