// user_network.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: contains/manages Station Mode/SoftAP configurations for the ESP8266.
//      Provides functionality to connect to a network:

#ifndef USER_NETWORK_H
#define USER_NETWORK_H

#include <user_interface.h>
#include <osapi.h>
#include <spi_flash.h>
#include <mem.h>
#include "user_flash.h"
#include "user_connect.h"
#include "user_captive.h"
#include "user_task.h"
#include "user_humidity.h"
#include "user_fan.h"

// User Task signals
#define SIG_CAPTIVE_DONE 1      // Sent to user_scan when the captive portal has received data.
                                // Causes user_scan to disconnect and delete the captive portal
                                // connection.

// Global configurations (must be accessible from callback functions)
struct station_config client_config;            // Station configuration

// Software Timers
os_timer_t timer_1;

/* ------------------- */
/* Function Prototypes */
/* ------------------- */

// User Task: user_scan(os_event_t *e)
// Desc: Pulls SSID/pass from flash memory then attempts to find an AP
//      broadcasting that SSID. If it finds one, attempts to connect.
//      if it doesn't, switches to SoftAP mode to establish its own
//      SSID, so that a user can connect and give it config
// Args: None
void ICACHE_FLASH_ATTR user_scan(os_event_t *e);

// Callback Function: user_scan_done(void *arg, STATUS status)
// Desc: Callback once AP scan is complete. Interprets AP scan results
//      and connects/sets up SoftAP accordingly
// Args:
//      void *arg: Information on AP's found by wifi_station_scan
//      STATUS status:  get status
static void ICACHE_FLASH_ATTR user_scan_done(void *arg, STATUS status);


// Callback Function: user_check_ip()
// Desc: Called every 1000ms to check if the ESP8266 has received an IP. Disarms timer
//      And moves forward when an IP is obtained.
void ICACHE_FLASH_ATTR user_check_ip(void);

#endif /* USER_NETWORK_H */
