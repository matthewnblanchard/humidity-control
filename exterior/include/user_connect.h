// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: Facilitates connections with the user program/exterior sensor. 

#ifndef USER_CONNECT_H
#define USER_CONNECT_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include "user_task.h"
#include "user_network.h"
#include "user_humidity.h"

// Port definitions
#define ESPCONNECT_ACCEPT 6000

/* ------------------- */
/* Function prototypes */
/* ------------------- */

// User Task: user_tcp_init();
// Desc: Configures exterior system to host a TCP connection
// Args: None
void ICACHE_FLASH_ATTR user_int_connect_init(os_event_t *e);

// Callback Function: user_tcp_accept_cb(void *arg);
// Desc: Callback function to be called when  tcp connection is made over port
// 	ESPCONNECT.
// Args:
// 	void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_int_connect_cb(void *arg);

// Callback Function: user_tcp_accept_recv_cb(void *arg, char *pusrdata, unsigned short length);
// Desc: Data receipt callback (accept version). Called when the client sends a packet to the server
// Args:
// 	void *arg: pointer to the espconn which called this function
// 	char *pusrdata: recieved client data
// 	unsinged short length: length of the user data
void ICACHE_FLASH_ATTR user_tcp_accept_recv_cb(void *arg, char *pusrdata, unsigned short length);

// Callback Function: user_tcp_recon_cb(void *arg, sint8 err);
// Desc: Reconnect callback. This is called when an error occurs in the TCP connection
// Args:
//	void *arg: pointer to the espconn which called this function
//	sing8 err: error code
void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg, sint8 err);

// Callback Function: user_tcp_discon_cb(void *arg);
// Desc: Disconnect callback. Called when a disconnection occurs.
// Args:
// 	void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg);

// Callback Function: user_captive_sent_cb(void *arg);
// Desc: Data sent callback. Called when data is sent to the server.
// Args:
// 	void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg);

// User Task: user_int_connect_cleanup(void *arg)
// Desc: Cleans up the interior connections if it fails to connect
void ICACHE_FLASH_ATTR user_int_connect_cleanup(os_event_t *e);

#endif /* USER_CONNECT_H */
