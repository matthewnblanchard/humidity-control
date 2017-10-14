// user_network_ext.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: contains/manages Station Mode/SoftAP configurations for the ESP8266.
//      Provides functionality to connect to a network.

#ifndef USER_NETWORK_H
#define USER_NETWORK_H

#include <user_interface.h>
#include <osapi.h>
#include <spi_flash.h>
#include <mem.h>
#include "user_flash.h"

// Global configurations (must be accessible from callback functions)
struct station_config client_config;            // Station configuration

// Software Timers
os_timer_t timer_1;

/* ------------------- */
/* Function Prototypes */
/* ------------------- */

// Application Function: user_scan(void)
// Desc: Pulls SSID/pass from flash memory then attempts to find an AP
//      broadcasting that SSID. If it finds one, attempts to connect.
//      if it doesn't, searches for the interior sensor system's configuration
//      SSID.
// Args: None
void ICACHE_FLASH_ATTR user_scan(void);

// Callback Function: user_scan_done(void *arg, STATUS status)
// Desc: Callback once AP scan is complete. Interprets AP scan results
//      and connects/scans again accordingly
// Args:
//      void *arg: Information on AP's found by wifi_station_scan
//      STATUS status:  get status
static void ICACHE_FLASH_ATTR user_scan_done(void *arg, STATUS status);


// Callback Function: user_check_ip()
// Desc: Called every 1000ms to check if the ESP8266 has received an IP. Disarms timer
//      And moves forward when an IP is obtained.
void ICACHE_FLASH_ATTR user_check_ip(void);

#endif /* USER_NETWORK_H */
