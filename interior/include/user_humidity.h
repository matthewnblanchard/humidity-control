// user_humidity.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: contains data structures/functions pertaining to to the gathering 
//      and storing of humidity data

#ifndef USER_HUMIDITY_H
#define USER_HUMIDITY_H

#include <user_interface.h>
#include <gpio.h>
#include <osapi.h>
#include "user_i2c.h"

// I2C address of the HIH8121 humidity sensor
#define SENSOR_ADDR     0x27

// Humidity data storage buffers. NOTE: The buffer is circular
#define SENSOR_BUFFER_SIZE      256
float *sensor_data_int;         // Interior humidity
extern uint16 data_index_int;   // Index of next available data slot

// Function Prototypyes:

// Callback Function: user_read_humidity()
// Desc: Gets a single humidity reading from the humidity sensor via I2C.
void ICACHE_FLASH_ATTR user_read_humidity(void);

// Application Function: user_endian_flip(uint8 *buf, uint8 n)
// Desc: Flips the endianness of the next n bytes of buf 
// Args:
//      uint8 *buf: Data buffer
//      uint8 n: Number of bytes to invert
void user_endian_flip(uint8 *buf, uint8 n);

#endif
