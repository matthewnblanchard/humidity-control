// user_network.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Contains/manages station mode configurations for the ESP8266.
//      Provides functionality to connect to a network.

#ifndef _USER_NETWORK_H
#define _USER_NETWORK_H

#include <user_interface.h>
#include <osapi.h>
#include <spi_flash.h>
#include <mem.h>
#include <espconn.h>
#include "user_flash.h"
#include "user_task.h"

/* ------------------- */
/* Function Prototypes */
/* ------------------- */

// User Task: user_scan(os_event_t *e)
// Desc: Pulls SSID/pass from flash memory then attempts to find an AP
//      broadcasting that SSID.
// Args: 
//	os_event_t *e: Pointer to OS event data
// Return:
//	Nothing
void ICACHE_FLASH_ATTR user_scan(os_event_t *e);

// Callback Function: user_scan_done(void *arg, STATUS status)
// Desc: Callback once AP scan is complete. Interprets AP scan results
//      and connects if an AP is found, or switches to config mode if not
// Args:
//      void *arg: Information on AP's found by wifi_station_scan
//      STATUS status: Status of AP scan results (validity)
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_scan_done(void *arg, STATUS status);


// Callback Function: void user_check_ip(void)
// Desc: Called every 1000ms to check if the ESP8266 has received an IP. Disarms timer
//      And moves forward when an IP is obtained.
// Args:
//	None
// Return:
//	Nothing
void ICACHE_FLASH_ATTR user_check_ip(void);

#endif
