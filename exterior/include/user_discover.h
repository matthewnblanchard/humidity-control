// user_discover.h
// Authors: Cristian Auspland & Matthew Blanchard
// Description: Facilitates the discover of the interior system

#ifndef USER_DISCOVER_H
#define USER_DISCOVER_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include "user_task.h"

// Port definitions
#define UDP_DISCOVERY_PORT 5000

// Broadcast timing constants
#define BROADCAST_PERIOD 1000
#define BROADCAST_NUM 60

// User Task: udp_broadcast_init()
// Desc: Broadcasts UDP discovery data over wireless network.
// Args: 
//	os_event_t *e: Pointer to OS event data
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_broadcast_init(os_event_t *e);

// Callback Function: user_send_discover(void)
// Desc: Broadcasts a discovery packet
// Args:
//	Nothing
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_send_broadcast(void);

// User Task: udp_broadcast_stop()
// Desc: Terminates the UDP broadcasts
// Args:
//	os_event_t *e: Pointer to OS event data 
// Returns:
//	Nothing
void ICACHE_FLASH_ATTR user_broadcast_stop(os_event_t *e);

#endif
