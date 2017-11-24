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

// User Task: user_apmode_init()
// Desc: Switches the ESP8266 to SoftAP mode, configures it to service connected clients
//      and creates a captive portal for users to enter configuration data
void ICACHE_FLASH_ATTR user_apmode_init(os_event_t *e);

#endif
