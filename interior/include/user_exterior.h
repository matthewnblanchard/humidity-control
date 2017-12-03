// user_exterior.h
// Authors: Christian Auspland & Matthew Blanchard
// Desc: Contains functions/definitions related to
//	the connection between the interior/exterior systems

#ifndef _USER_EXTERIOR_H
#define _USER_EXTERIOR_H

#include <user_interface.h>
#include <osapi.h>
#include <espconn.h>
#include <mem.h>
#include "user_humidity.h"
#include "user_task.h"
#include "user_connect.h"

// Constants (ports, timing, etc)
//#define MDNS_PORT 5353	
#define BROADCAST_PORT 5000
#define ESP_CONNECT_PORT 6000
#define EXT_PACKET_SIZE 4	// Size of data contained within a packet from the exterior system
#define EXT_WAIT_TIME 60000	// Maximum wait time (in ms) for discovery of the exterior system before fallback to config mode

// User Task: user_mdns_init(os_event_t *e)
// Desc: Initializes mDNS to discover the exterior system
// Args:
//	os_event_t *e: Pointer to OS event data
// Return:
//	Nothing
//void ICACHE_FLASH_ATTR user_mdns_init(os_event_t *e);

// Callback Function: user_mdns_recv_cb(void *arg, char *pusrdata, unsigned short len)
// Desc: Callback for when the systme receives an mDNS packet.
//	responded to queries for its hostname
//void ICACHE_FLASH_ATTR user_mdns_recv_cb(void *arg, char *pusrdata, unsigned short len);

// User Task: user_broadcast_init(os_event_t *e)
// Desc: Initializes the UDP broadcast connection for discovery of the
//	exterior system.
// Args:
//	os_event_t *e: Pointer to OS event data
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_broadcast_init(os_event_t *e);

// Callback Function: user_broadcast_recv_cb(void *arg)
// Desc: Called when a UDP BROADCAST_PORT packet is received. Checks for IP information from the
//	    exterior sensor system, then attempts to connect if the data is valid
// Args:
//	void *arg: espconn for connection 
//	char *pusrdata: Received data
//	unsigned short length: Received data length
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length);

// User Task: user_espconnect_init(os_event_t *e)
// Desc: Initializes the TCP connection with the exterior system
// Args:
//      os_event_t *e: Pointer to event 
void ICACHE_FLASH_ATTR user_espconnect_init(os_event_t *e);

// Callback Function: user_espconnect_connect_cb(void *arg)
// Desc: Called when the connection to the exterior system is made
// Args:
//      void *arg: espconn for connection
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_espconnect_connect_cb(void *arg);

// Callback Function: user_espconnect_recv_cb(void *arg, char *pusrdata, unsigned short length)
// Desc: Called when humidity data is received from the exterior system
// Args:
//      void *arg: Pointer to espconn
//      char *pusrdata: Received data
//      unsigned short length: Length of received data
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_espconnect_recv_cb(void *arg, char *pusrdata, unsigned short length);

// Callback Function: user_espconnect_sent_cb(void *arg)
// Desc: Called when configuration data is sent to the exterior system
// Args:
//      void *arg: Pointer to espconn
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_espconnect_sent_cb(void *arg);

// Callback Function: user_espconnect_recon_cb(void *arg, sint8 err);
// Desc: Called when an error occurs in the connection with the exterior
// Args:
//      void *arg: Pointer to espconn
//      sint8 err: Error signal
// Returns:
//	Nothing
// static void ICACHE_FLASH_ATTR user_espconnect_recon_cb(void *arg, sint8 err);

// Callback Function: user_espconnect_discon_cb(void *arg)
// Desc: Called when the connection to the exterior system ends
// Args:
//      void *arg: Pointer to espconn
// Return:
//	Nothing
// static void ICACHE_FLASH_ATTR user_espconnect_discon_cb(void *arg);

// Callback Function: user_ext_timeout(void)
// Desc: Called when the exterior connection wait timer
//	runs out. Cleans up discovery connections.
// Args:
//	None
// Return:
//	None
void ICACHE_FLASH_ATTR user_ext_timeout(void);

#endif
