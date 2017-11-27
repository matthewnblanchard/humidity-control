// user_exterior.h
// Authors: Christian Auspland & Matthew Blanchard
// Desc: Contains functions/definitions related to
//	the connection between the interior/exterior systems

#ifndef _USER_EXTERIOR_H
#define _USER_EXTERIOR_H

#include <user_interface.h>
#include <osapi.h>
#include <espconn.h>
#include "user_humidity.h"
#include "user_task.h"
#include "user_connect.h"

//#define MDNS_PORT 5353
#define BROADCAST_PORT 5000
#define ESP_CONNECT_PORT 6000
#define EXT_PACKET_SIZE 4
#define EXT_WAIT_TIME 10000

// User Task: user_mdns_init(os_event_t *e)
// Desc: Initializes mDNS to discover the exterior system
//void ICACHE_FLASH_ATTR user_mdns_init(os_event_t *e);

// Callback Function: user_mdns_recv_cb(void *arg, char *pusrdata, unsigned short len)
// Desc: Callback for when the systme receives an mDNS packet.
//	responded to queries for its hostname
//void ICACHE_FLASH_ATTR user_mdns_recv_cb(void *arg, char *pusrdata, unsigned short len);

// User Task: user_broadcast_init(os_event_t *e)
// Desc: Initializes the UDP broadcast connection for discovery of the
//	exterior system.
// Args:
//	os_event_t *e: Pointer to event data
void ICACHE_FLASH_ATTR user_broadcast_init(os_event_t *e);

// Callback Function: user_broadcast_recv_cb(void *arg)
// Desc: Called when a UDP BROADCAST_PORT packet is received
// Args:
//	void *arg: espconn for connection 
//	char *pusrdata: Received data
//	unsigned short length: Received data length
void ICACHE_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length);

// User Task: user_espconnect_init(os_event_t *e)
// Desc: Initializes the TCP connection with the exterior system
// Args:
//      os_event_t *e: Pointer to event 
void ICACHE_FLASH_ATTR user_espconnect_init(os_event_t *e);

// Callback Function: user_espconnect_connect_cb(void *arg)
// Desc: Called when the TCP ESPCONNECT connection is made
// Args:
//      void *arg: espconn for connection
void ICACHE_FLASH_ATTR user_espconnect_connect_cb(void *arg);

// Callback Function: user_espconnect_recv_cb(void *arg, char *pusrdata, unsigned short length)
// Desc: Called when humidity data is received from the exterior system
// Args:
//      void *arg: Pointer to espconn
//      char *pusrdata: Received data
//      unsigned short length: Length of received data
void ICACHE_FLASH_ATTR user_espconnect_recv_cb(void *arg, char *pusrdata, unsigned short length);

// Callback Function: user_espconnect_sent_cb(void *arg)
// Desc: Called when config data is sent to the exterior system
// Args:
//      void *arg: Pointer to espconn
void ICACHE_FLASH_ATTR user_espconnect_sent_cb(void *arg);

// Callback Function: user_espconnect_recon_cb(void *arg, sint8 err);
// Desc: Called when an error occurs in the connection with the exterior
// Args:
//      void *arg: Pointer to espconn
//      sint8 err: Error signal
void ICACHE_FLASH_ATTR user_espconnect_recon_cb(void *arg, sint8 err);

// Callback Function: user_espconnect_discon_cb(void *arg)
// Desc: Called when the connection to the exterior system ends
// Args:
//      void *arg: Pointer to espconn
void ICACHE_FLASH_ATTR user_espconnect_discon_cb(void *arg);

// Callback Function: user_ext_timeout(void)
// Desc: Called when the exterior connection wait timer
//	runs out. Switches back to configure mode
void ICACHE_FLASH_ATTR user_ext_timeout(void);

#endif /* _USER_EXTERIOR_H */
