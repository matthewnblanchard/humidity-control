// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: Facilitates connections with the user program/exterior sensor. 

#ifndef USER_CONNECT_H
#define USER_CONNECT_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include "mbedtls/sha1.h"
#include "mbedtls/base64.h"
#include "user_task.h"
#include "user_humidity.h"
#include "user_fan.h"

// Port definitions
#define UDP_DISCOVERY_PORT 5000
#define HTTP_PORT 80

// IP Macros
#define IP_OCTET(ip, oct) (ip >> (8 * oct)) % 256

#define WS_UPDATE_TIME 1500      // Update interval for the WebSocket in milliseconds

/* ------------------- */
/* Function prototypes */
/* ------------------- */

// User Task: void user_front_init(os_event_t *e)
// Desc: Prepares the ESP8266 for HTTP connections to the user webpage front end
// Args:
//      os_event_t *e: Message signal for the task
// Returns:
//      Nothing
void ICACHE_FLASH_ATTR user_front_init(os_event_t *e);

// Callback Function: user_front_connect_cb(void *arg)
// Desc: Called when a client connects the front end webserver.
// Args:
//      void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_front_connect_cb(void *arg);
// Callback Function: user_front_recon_cb(void *arg, sint8 err)
// Desc: Reconnect callback. This is called when an error occurs
//      in the TCP connection
// Args:
//      void *arg: pointer to the espconn which called this function
//      sint8 err: error code
void ICACHE_FLASH_ATTR user_front_recon_cb(void *arg, sint8 err);

// Callback Function: user_front_discon_cb(void *arg)
// Desc: Disconnect callback. Called when a disconnection occurs
// Args:
//      void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_front_discon_cb(void *arg);

// Callback Function: user_front_recv_cb(void *arg, char *pusrdata, unsigned short length)
// Desc: Data receipt callback. Called when the client sents a packet to the server
// Args:
//      void *arg: pointer to the espconn which called this function
//      char *pusrdata: received client data
//      unsigned short length: length of user data
void ICACHE_FLASH_ATTR user_front_recv_cb(void *arg, char *pusrdata, unsigned short length);

// Callback Function: user_front_sent_cb(void *arg)
// Desc: Data sent callback. Called when data is sent to the client
// Args:
//      void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_front_sent_cb(void *arg);

// Callback Function: user_ws_recv_cb(void *arg, char *pusrdata, unsigned short length)
// Desc: Data receipt callback for the WebSocket connection.
// Args:
//      void *arg: pointer to the espconn which called this function
//      char *pusrdata: received client data
//      unsigned short length: length of user data
void ICACHE_FLASH_ATTR user_ws_recv_cb(void *arg, char *pusrdata, unsigned short length);

// Callback Function: user_ws_update(void *parg)
// Desc: Timer function which periodically sends updated information to the websocket
// Args:
//      void *parg: Pointer to espconn containing the websocket
void ICACHE_FLASH_ATTR user_ws_update(void *parg);

// Application Function: user_endian_flip(uint8 *buf, uint8 n)
// Desc: Flips the endianness of the next n bytes of buf 
// Args:
//      uint8 *buf: Data buffer
//      uint8 n: Number of bytes to invert
void ICACHE_FLASH_ATTR user_endian_flip(uint8 *buf, uint8 n);

// Application Function: user_ws_parse_data(uint8 *data, uint16 len)
// Desc: Parses data received from the WebSocket and takes action accordingly
// Args:
//	uint8 *data: Received data
//	uint16 len:  Length of data
void ICACHE_FLASH_ATTR user_ws_parse_data(uint8 *data, uint16 len);

// Application Function: uint32 user_atoi(uint8 *str, uint16 len)
// Desc: Converts an ASCII string to an integer
// Args:
//	uint8 *str:	String to be converted
//	uint16 len:	Length of string
uint32 ICACHE_FLASH_ATTR user_atoi(uint8 *str, uint16 len);

#endif /* USER_CONNECT_H */
