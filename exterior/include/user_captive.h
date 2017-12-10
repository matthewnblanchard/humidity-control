// user_captive.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Connects the exterior with the interior during the configuration stage

#ifndef USER_CAPTIVE_H
#define USER_CAPTIVE_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include "user_task.h"
#include "user_flash.h"

// Port Definitions
#define CONFIG_PORT 4000

// User Task: user_config_assoc_init(os_event_t *e)
// Desc: Configure the system to asscoiate with the interior system when it
//	is in AP mode
// Args:
//	os_event_t *e: Point to OS event data
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_config_assoc_init(os_event_t *e);

// Callback Function: user_config_assoc(void)
// Desc: Attempt to connect to the interior system
// Args:
//	Nothing
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_config_assoc(void);

// Task: user_config_connect_init();
// Desc: Configures system to establish TCP connection
// Args:
//	os_event_t *e: Pointer to OS event data
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_config_connect_init(os_event_t *e);

// Callback Function: user_tcp_connect_cb(void *arg);
// Desc: Callback function to be called when a tcp connection is made
// 	over HTTP_PORT.
// Args:
// 	void *arg: pointer to the espconn which called this function
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg);

// Callback Function: user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length);
// Desc: Data receipt callback. Called when the server sends a packet to the client
// Args:
// 	void *arg: Pointer to the espconn which called this function
// 	char *pusrdata: Received client data
// 	unsigned short length: Length of user data
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length);

// User Task: user_config_cleanup(os_event_t *e)
// Desc: Config mode cleanup function. Stops connections.
// Args:
//	os_event_t *e: Pointer to OS event data
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_config_cleanup(os_event_t *e);

#endif
