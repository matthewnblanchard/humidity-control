# === Compiler === #
# Open source compiler for on board MCU
CC = xtensa-lx106-elf-gcc

# === Compiler Flags === #
# -I. : check for headers in current directory
# -mlongcalls : translate direct calls to indirect calls, unless it can
#       be determined that the call is in range. Ensures calls to API
#       functions (defined elswhere) work correctly
CFLAGS = -I./include -mlongcalls -DICACHE_FLASH

# === Linker Libraries === #
# -nostdlib : disallow linker for searching the standard library for libraries. Only 
#       explicitly specified library directories are allowed.
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -lc -Wl,--end-group -lgcc

# === Linker Flags === #
LDFLAGS = -Teagle.app.v6.ld

BINDIR = user/bin
OBJDIR = user/obj
SRCDIR = user/src

OBJ = user_main.o user_connect.o user_network.o user_humidity.o user_i2c.o user_discover.o user_captive.o
OBJ := $(addprefix $(OBJDIR)/, $(OBJ))
SRC = user_main.c user_connect.c user_network.c user_humidity.c user_i2c.c user_discover.h user_captive.h
SRC := $(addprefix $(SRCDIR)/, $(SRC))
TARGET = $(BINDIR)/user_main

$(BINDIR)/user_main-0x00000.bin: $(TARGET)
	esptool.py elf2image $^

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@

flash: $(BINDIR)/user_main-0x00000.bin
	esptool.py --port /dev/ttyUSB1 write_flash 0 $(BINDIR)/user_main-0x00000.bin 0x10000 $(BINDIR)/user_main-0x10000.bin

default: $(BINDIR)/esp_init_data_default.bin
	esptool.py --port /dev/ttyUSB1 write_flash 0x3FC000 $(BINDIR)/esp_init_data_default.bin

erase:
	esptool.py --port /dev/ttyUSB1 --baud 9600 erase_flash

clean:
	rm -f $(TARGET) $(OBJ) $(BINDIR)/user_main-0x00000.bin $(BINDIR)/user_main-0x10000.bin
