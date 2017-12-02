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
#include "user_fan.h"
#include "user_task.h"

// I2C address of the HIH8121 humidity sensor
#define SENSOR_ADDR     0x27

// Humidity data read interval in ms
#define HUMIDITY_READ_INTERVAL 3000

// Humidity data storage.
extern float sensor_data_int;         // Interior humidity
extern float sensor_data_ext;	      // Exterior humidity
extern float threshold_humidity;      // The system will not try to reduce the humidity below this

// Function Prototypyes:

// Callback Function: user_read_humidity()
// Desc: Gets a single humidity reading from the humidity sensor via I2C.
// Args:
//	None
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_read_humidity(void);

// Application Function: user_humidity_cmp(void)
// Desc: Compares the interior and exterior humidities,
//	then drives the fan accordingly
// Args:
//	None
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_humidity_cmp(void);

#endif
