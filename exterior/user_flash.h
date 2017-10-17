// Name: user_flash.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Contains function / definitions to facilitate storage of user
//      data in flash

#ifndef USER_FLASH_H
#define USER_FLASH_H

#include <user_interface.h>

// Memory Addresses - the ESP8266 doesn't have dedicated EEPROM, however the SDK
// provides functions to write to flash memory, and guarantees that certain sectors
// are left available for user data. Addresses below are handpicked from those 
// sectors, starting from 0x0003C000. Flash is handled in 4KB sectors, numbered 
// starting from 0

#define USER_DATA_START_ADDR            0x00070000
#define USER_DATA_START_SECT            USER_DATA_START_ADDR / (4 * 1024)
#define USER_DATA_END_ADDR              0x00080000
#define USER_DATA_END_SECT              USER_DATA_END_SECT / (4 * 1024)        

// Structures for saved data - all data to be saved/read from flash memory
// must be 4 byte aligned. Additional padding is added where necessary

struct user_data_station_config {       // Client configuration info (SSID, etc) - 104 bytes
        struct station_config config;   // Station config info - 103 bytes 
        uint8 pad[1];                   // Extra padding - 1 byte
};

#endif /* USER_FLASH_H */
