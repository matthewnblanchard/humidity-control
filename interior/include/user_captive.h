// user_captive.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Contains functionality to open and maintain a captive portal
//      through which a user may configure the ESP8266

#ifndef USER_CAPTIVE_H
#define USER_CAPTIVE_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include <mem.h>
#include <spi_flash.h>
#include "user_task.h"
#include "user_flash.h"

// Port definitions
#define HTTP_PORT       80
#define EXT_CONFIG_PORT 4000

// User Task: user_apmode_init()
// Desc: Switches the ESP8266 to SoftAP mode, configures it to service connected clients
//      and creates a captive portal for users to enter configuration data
// Args:
//	os_event_t *e: Pointer to OS event data
// Return:
//	Nothing
void ICACHE_FLASH_ATTR user_apmode_init(os_event_t *e);

// Callback Function: user_captive_connect_cb(void *arg)
// Desc: Called when a client connects the the captive portal server.
// Args:
//      void *arg: pointer to the espconn which called this function
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_captive_connect_cb(void *arg);

// Callback Function: user_captive_recv_cb(void *arg, sint8 err)
// Desc: Reconnect callback. This is called when an error occurs
//      in the TCP connection
// Args:
//      void *arg: pointer to the espconn which called this function
//      sint8 err: error code
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_captive_recon_cb(void *arg, sint8 err);

// Callback Function: user_captive_discon_cb(void *arg)
// Desc: Disconnect callback. Called when a disconnection occurs
// Args:
//      void *arg: pointer to the espconn which called this function
// static void ICACHE_FLASH_ATTR user_captive_discon_cb(void *arg);

// Callback Function: user_captive_recv_cb(void *arg, char *pusrdata, unsigned short length)
// Desc: Data receipt callback. Called when the client sents a packet to the server
// Args:
//      void *arg: pointer to the espconn which called this function
//      char *pusrdata: received client data
//      unsigned short length: length of user data
// static void ICACHE_FLASH_ATTR user_captive_recv_cb(void *arg, char *pusrdata, unsigned short length);

// Callback Function: user_captive_sent_cb(void *arg)
// Desc: Data sent callback. Called when data is sent to the client
// Args:
//      void *arg: pointer to the espconn which called this function
// static void ICACHE_FLASH_ATTR user_captive_sent_cb(void *arg);

// Callback Function: user_captive_ext_connect_cb(void *arg)
// Desc: Called when the exterior system connects to the interior system during the captive portal phase
// Args:
//      void *arg: pointer to the espconn which called this function
// static void ICACHE_FLASH_ATTR user_captive_ext_connect_cb(void *arg);

// Callback Function: user_captive_ext_recv_cb(void *arg, sint8 err)
// Desc: Reconnect callback. This is called when an error occurs
//      in the TCP connection
// Args:
//      void *arg: pointer to the espconn which called this function
//      sint8 err: error code
// static void ICACHE_FLASH_ATTR user_captive_ext_recon_cb(void *arg, sint8 err);

// Callback Function: user_captive_ext_discon_cb(void *arg)
// Desc: Disconnect callback. Called when a disconnection occurs
// Args:
//      void *arg: pointer to the espconn which called this function
// static void ICACHE_FLASH_ATTR user_captive_ext_discon_cb(void *arg);

// Callback Function: user_captive_ext_recv_cb(void *arg, char *pusrdata, unsigned short length)
// Desc: Data receipt callback. Called when the exterior system sents a packet to the server
// Args:
//      void *arg: pointer to the espconn which called this function
//      char *pusrdata: received client data
//      unsigned short length: length of user data
// static void ICACHE_FLASH_ATTR user_captive_ext_recv_cb(void *arg, char *pusrdata, unsigned short length);

// Callback Function: user_captive_ext_sent_cb(void *arg)
// Desc: Data sent callback. Called when data is sent to the exterior system
// Args:
//      void *arg: pointer to the espconn which called this function
// static void ICACHE_FLASH_ATTR user_captive_ext_sent_cb(void *arg);

// Callback Function: user_ext_send_cred(void)
// Desc: Attempts to send WiFi credentials to the exterior system,
//	if present
void ICACHE_FLASH_ATTR user_ext_send_cred(void);

// User Task: user_apmode_cleanup(os_event_t *e)
// Desc: Performs cleanup from AP mode (disconnected tcp connections, etc)
void ICACHE_FLASH_ATTR user_apmode_cleanup(os_event_t *e);

// Application Function: uint16 user_http_post_fix(uint8 *str, uint16 len)
// Desc: Parses an HTTP post string and replaces escaped characters
// 	with their equivalents (+ --> space, %XX to ASCII value)
// Args:
//	uint8 *str: HTTP post string
//	uint16 len: Length of the string
// Returns:
//	The new length of the string
uint16 ICACHE_FLASH_ATTR user_http_post_fix(uint8 *str, uint16 len);

// Application Function: uint32 user_axtoi(uint8 *str, uint16 len)
// Desc: Converts a string of ASCII characters representing a 
//	hexadecimal integer and coverts it to the corresponding
//	integer
// Args:
//	uint8 *str: Hexadecimal string
//	uint16 len: Length of the string
// Returns:
//	The value of the hexadecimal string
uint32 ICACHE_FLASH_ATTR user_axtoi(uint8 *str, uint16 len);

#endif
