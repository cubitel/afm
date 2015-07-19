DIY Atomic Force Microscope
---------------------------

DIY Atomic Force Microscope / Scanning Tunneling Microscope project.
More information can be found in [Project page](http://npk.cubitel.org/?/hardware/afm) (in Russian).

Please note that this project is under development yet.

### Hardware

Hardware folder contains hardware design files.

_Design tools_
- Schematic, PCB -- KiCAD
- 3D parts -- FreeCAD

### Firmware

Firmware folder contains firmware source files for STM32F407 controller.
Firmware using FreeRTOS and STM32 USB stack (bundled with the source tree).

### Software

Software folder contains control application source files.
Software using wxWidgets and libusb and designed to be cross-platform,
but current Makefile assumes Windows/MinGW.

_Build requirements_
- TDM-GCC
- wxWidgets
- libusb

### License
GPLv3
