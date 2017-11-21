# === Compiler === #
# Open source compiler for on board MCU
CC = xtensa-lx106-elf-gcc

# === Compiler Flags === #
# -I. : check for headers in current directory
# -mlongcalls : translate direct calls to indirect calls, unless it can
#       be determined that the call is in range. Ensures calls to API
#       functions (defined elswhere) work correctly
CFLAGS = -I. -mlongcalls -DICACHE_FLASH

# === Linker Libraries === #
# -nostdlib : disallow linker for searching the standard library for libraries. Only 
#       explicitly specified library directories are allowed.
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -lc -Wl,--end-group -lgcc

# === Linker Flags === #
LDFLAGS = -Teagle.app.v6.ld

TARGET = user_main
OBJ = user_main.o user_connect.o user_network.o user_task.o user_humidity.o user_i2c.o

user_main-0x00000.bin: $(TARGET)
	esptool.py elf2image $^

user_main: $(OBJ)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $(TARGET)

%.0: %.c
	$(CC) $(CFLAGS) $< $(LDFLAGS) $(LDLIBS)

flash: user_main-0x00000.bin
	esptool.py write_flash 0 user_main-0x00000.bin 0x10000 user_main-0x10000.bin

default: esp_init_data_default.bin
	esptool.py write_flash 0x3FC000 esp_init_data_default.bin

erase:
	esptool.py erase_flash

clean:
	rm -f $(TARGET) $(OBJ) user_main-0x00000.bin user_main-0x10000.bin
