// user_exterior.h
// Authors: Christian Auspland & Matthew Blanchard
// Desc: Contains functions/definitions related to
//	the connection between the interior/exterior systems

#ifndef _USER_EXTERIOR_H
#define _USER_EXTERIOR_H

#include <user_interface.h>
#include <osapi.h>
#include <espconn.h>

#define BROADCAST_PORT 5000

// UDP broadcast connections for discovery
struct espconn udp_broadcast_conn;
struct _esp_udp udp_broadcast_proto;

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
void ICACHE_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length);

#endif /* _USER_EXTERIOR_H */

