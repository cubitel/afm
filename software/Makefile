
# wxWidgets location
WXDIR=C:/wxWidgets-3.0.2
WXLIBDIR=C:/wxWidgets-3.0.2/lib/gcc481TDM_dll

# lubusb location
LIBUSBDIR=C:/Projects/LIB/libusb
LIBUSBLIB=$(LIBUSBDIR)/MinGW32/static

BIN=afm-control.exe
OBJS=main.o device.o image.o compat.o dfu.o
INCLUDE=-I$(WXLIBDIR)/mswu -I$(WXDIR)/include -I$(LIBUSBDIR)/include/libusb-1.0 -I../firmware/src
LIBS=-L$(WXLIBDIR) -L$(LIBUSBLIB) -lwxbase30u -lwxmsw30u_core -lusb-1.0

CFLAGS=-mwindows -DWIN32 -D__WXMSW__ -DNDEBUG -Wno-cpp
LDFLAGS=-mwindows -fPIC

all: $(BIN)

$(BIN): $(OBJS)
	g++ $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
 
%.o: %.cpp
	g++ -c $(CFLAGS) $(INCLUDE) -o $@ $<

%.o: %.c
	gcc -c $(CFLAGS) $(INCLUDE) -o $@ $<
	