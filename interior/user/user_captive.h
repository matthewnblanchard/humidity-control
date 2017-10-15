// user_captive.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Contains functionality to open and maintain a captive portal
//      through which a user may configure the ESP8266

#ifndef USER_CAPTIVE_H
#define USER_CAPTIVE_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include "user_task.h"

// Port definitions
#define HTTP_PORT       80

// IP Macros
#define IP_OCTET(ip, oct) (ip >> (8 * oct)) % 256

// Task message queues
os_event_t * ap_init_msg_queue;

// SoftAP configuration
extern struct softap_config ap_config;

// espconn structs - Connection control structures
struct espconn tcp_captive_conn;
struct _esp_tcp tcp_captive_proto;

// User Task: user_apmode_init()
// Desc: Switches the ESP8266 to SoftAP mode, configures it to service connected clients
//      and creates a captive portal for users to enter configuration data
void ICACHE_FLASH_ATTR user_apmode_init(os_event_t *e);

// Callback Function: user_captive_connect_cb(void *arg)
// Desc: Called when a client connects the the captive portal server.
// Args:
//      void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_captive_connect_cb(void *arg);

#endif
