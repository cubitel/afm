
FWVERMAJOR=0
FWVERMINOR=1

CC=arm-none-eabi-gcc
OBJCOPY=arm-none-eabi-objcopy
ARCH=arm-none-eabi-ar
DEBUG=-g
OPTIM=-O2
LDSCRIPT=stm32/stm32_flash.ld

CFLAGS= \
-Isrc -Ifreertos -Istm32 -Istm32usb \
-mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
-Wall  \
-Wextra  \
-Wstrict-prototypes  \
-Wmissing-prototypes  \
-Wmissing-declarations  \
-Wno-strict-aliasing  \
-Wno-unused-parameter \
-Wno-unused-function \
-D GCC_ARM7_ECLIPSE  \
-D THUMB_INTERWORK \
-D FWVERMAJOR=$(FWVERMAJOR) -D FWVERMINOR=$(FWVERMINOR) \
-T$(LDSCRIPT) \
$(DEBUG)  \
$(OPTIM) \
-fomit-frame-pointer

LINKER_FLAGS=-Xlinker -ofirmware.elf -Xlinker -M -Xlinker -Map=firmware.map

CRT0=stm32/startup_stm32f4xx.s

SRC= \
  src/main.c \
  src/microscope.c \
  src/usb.c \
  src/usb_class.c \
  src/usbd_desc.c \
  freertos/tasks.c \
  freertos/queue.c \
  freertos/list.c \
  freertos/port.c \
  freertos/heap_2.c \
  stm32/system_stm32f4xx.c \
  stm32usb/usb_core.c \
  stm32usb/usb_dcd.c \
  stm32usb/usb_dcd_int.c \
  stm32usb/usbd_core.c \
  stm32usb/usbd_req.c \
  stm32usb/usbd_ioreq.c

OBJ = $(SRC:.c=.o)

all: firmware.bin

firmware.bin : firmware.elf
	$(OBJCOPY) $< -O binary $@

firmware.elf : $(OBJ) Makefile
	$(CC) $(CFLAGS) $(OBJ) $(CRT0) $(LINKER_FLAGS)

$(OBJ) : %.o : %.c $(LDSCRIPT) Makefile
	$(CC) -c $(CFLAGS) $< -o $@
