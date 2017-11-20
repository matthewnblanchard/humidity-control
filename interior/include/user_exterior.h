// user_exterior.h
// Authors: Christian Auspland & Matthew Blanchard
// Desc: Contains functions/definitions related to
//	the connection between the interior/exterior systems

#ifndef _USER_EXTERIOR_H
#define _USER_EXTERIOR_H

#include <user_interface.h>
#include <osapi.h>
#include <espconn.h>
#include "user_task.h"
#include "user_humidity.h"
#include "user_network.h"
#include "user_fan.h"
#include "user_connect.h"

#define BROADCAST_PORT 5000
#define ESP_CONNECT_PORT 6000
#define EXT_PACKET_SIZE 4
#define EXT_WAIT_TIME 60000

// UDP broadcast connections for discovery
struct espconn udp_broadcast_conn;
struct _esp_udp udp_broadcast_proto;

// TCP connections for interior - exterior connection
struct espconn tcp_espconnect_conn;
struct _esp_tcp tcp_espconnect_proto;

// Discovery key info
extern char * discovery_recv_key;
extern uint16 discovery_recv_keylen;

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

// Callback Function: user_ext_notfound_cb(void)
// Desc: Called when the exterior connection wait timer
//	runs out. Switches back to configure mode
void ICACHE_FLASH_ATTR user_ext_notfound_cb(void);

#endif /* _USER_EXTERIOR_H */
