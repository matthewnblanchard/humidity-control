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

#define SENSOR_ADDR     0x27

// Function Prototypyes:

// Callback Function: user_read_humidity()
// Desc: Gets a single humidity reading from the humidity sensor via I2C.
void ICACHE_FLASH_ATTR user_read_humidity(void);

#endif
