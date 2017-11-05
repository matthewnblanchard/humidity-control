// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: Facilitates connections with the user program/exterior sensor. 

#ifndef USER_CONNECT_H
#define USER_CONNECT_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include "user_network.h"

// Port definitions
#define UDP_DISCOVERY_PORT 5000
#define HTTP_PORT 80

// IP Macros
#define IP_OCTET(ip, oct) (ip >> (8 * oct)) % 256

// espconn structs - these are control structures for TCP/UDP connections
struct espconn udp_broadcast_conn;
struct _esp_udp udp_broadcast_proto;
struct station_config station_conn;

/* ------------------- */
/* Function prototypes */
/* ------------------- */

// Task: udp_broadcast_send()
// Desc: Broadcasts UDP discovery data over wireless network.
// Args: none 
void ICACHE_FLASH_ATTR udp_broadcast();


// Callback Function: udp_listen_cb(void *arg, char *pdata, unsigned short len)
// Desc: Callback function to be called when packets are received over the
//      UDP discovery port. NEED MORE INFO HERE
// Args:
//      void *arg:              Passed espconn struct for the triggering connection
//      char *pdata:            Packet data
//      unsigned short len:     Data length
void ICACHE_FLASH_ATTR udp_broadcast_cb(void *arg);

#endif /* USER_CONNECT_H */
