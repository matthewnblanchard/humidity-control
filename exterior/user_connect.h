// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: Facilitates connections with the user program/exterior sensor. 

#ifndef USER_CONNECT_H
#define USER_CONNECT_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include "user_network.h"
#include "user_humidity.h"

// Port definitions
#define UDP_DISCOVERY_PORT 5000
#define HTTP_PORT 80
#define ESPCONNECT 4000

// IP Macros
#define IP_OCTET(ip, oct) (ip >> (8 * oct)) % 256

// espconn structs - these are control structures for TCP/UDP connections
struct espconn udp_broadcast_conn;
struct _esp_udp udp_broadcast_proto;
struct espconn tcp_connect_conn;
struct _esp_tcp tcp_connect_proto;
struct station_config station_conn;

int server_counter;

/* ------------------- */
/* Function prototypes */
/* ------------------- */

// Task: udp_broadcast_send()
// Desc: Broadcasts UDP discovery data over wireless network.
// Args: none 
void ICACHE_FLASH_ATTR udp_broadcast();

// Task: user_tcp_accept();
// Desc: Configures exterior system to host a TCP connection
// Args: None
void ICACHE_FLASH_ATTR user_tcp_accept();

// Task: user_tcp_connect();
// Desc: Configures system to establish TCP connection
// Args: None
void ICACHE_FLASH_ATTR user_tcp_connect();

// Callback Function: tcp_timer_cb();
// Desc: Called after 1s of timer while waiting for interior 
// 	to connect to the exterior.
// Args: None 
void ICACHE_FLASH_ATTR tcp_timer_cb();

// Callback Function: udp_listen_cb(void *arg, char *pdata, unsigned short len)
// Desc: Callback function to be called when packets are received over the
//      UDP discovery port. NEED MORE INFO HERE
// Args:
//      void *arg:              Passed espconn struct for the triggering connection
//      char *pdata:            Packet data
//      unsigned short len:     Data length
void ICACHE_FLASH_ATTR udp_broadcast_cb(void *arg);

// Callback Function: user_tcp_connect_cb(void *arg);
// Desc: Callback function to be called when a tcp connection is made
// 	over HTTP_PORT.
// Args:
// 	void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg);

// Callback Function: user_tcp_accept_cb(void *arg);
// Desc: Callback function to be called when  tcp connection is made over port
// 	ESPCONNECT.
// Args:
// 	void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_tcp_accept_cb(void *arg);

// Callback Function: user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length);
// Desc: Data receipt callback. Called when the server sends a packet to the client
// Args:
// 	void *arg: pointer to the espconn which called this function
// 	char *pusrdata: recieved client data
// 	unsigned short length: length of user data
void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length);

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

// Callback Function: acknowledge_tcp();
// Desc: Periodically sends data over tcp so the connection doesn't
// 	timeout and end the connection
// Args:
// 	None
void ICACHE_FLASH_ATTR acknowledge_tcp(void);

#endif /* USER_CONNECT_H */
