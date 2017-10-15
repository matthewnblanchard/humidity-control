// user_captive.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Contains functionality to open and maintain a captive portal
//      through which a user may configure the ESP8266

#ifndef USER_CAPTIVE_H
#define USER_CAPTIVE_H

#include <user_interface.h>
#include <espconn.h>
#include <osapi.h>
#include "user_task.h"

// Port definitions
#define HTTP_PORT       80

// IP Macros
#define IP_OCTET(ip, oct) (ip >> (8 * oct)) % 256

// Task message queues
os_event_t * ap_init_msg_queue;

// SoftAP configuration
extern struct softap_config ap_config;

// espconn structs - Connection control structures
struct espconn tcp_captive_conn;
struct _esp_tcp tcp_captive_proto;

// User Task: user_apmode_init()
// Desc: Switches the ESP8266 to SoftAP mode, configures it to service connected clients
//      and creates a captive portal for users to enter configuration data
void ICACHE_FLASH_ATTR user_apmode_init(os_event_t *e);

// Callback Function: user_captive_connect_cb(void *arg)
// Desc: Called when a client connects the the captive portal server.
// Args:
//      void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_captive_connect_cb(void *arg);

// Callback Function: user_captive_recv_cb(void *arg, sint8 err)
// Desc: Reconnect callback. This is called when an error occurs
//      in the TCP connection
// Args:
//      void *arg: pointer to the espconn which called this function
//      sint8 err: error code
void ICACHE_FLASH_ATTR user_captive_recon_cb(void *arg, sint8 err);

// Callback Function: user_captive_recv_cb(void *arg)
// Desc: Disconnect callback. Called when a disconnection occurs
// Args:
//      void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_captive_discon_cb(void *arg);

// Callback Function: user_captive_recv_cb(void *arg, char *pusrdata, unsigned short length)
// Desc: Data receipt callback. Called when the client sents a packet to the server
// Args:
//      void *arg: pointer to the espconn which called this function
//      char *pusrdata: received client data
//      unsigned short length: length of user data
void ICACHE_FLASH_ATTR user_captive_recv_cb(void *arg, char *pusrdata, unsigned short length);

// Callback Function: user_captive_sent_cb(void *arg)
// Desc: Data sent callback. Called when data is sent to the client
// Args:
//      void *arg: pointer to the espconn which called this function
void ICACHE_FLASH_ATTR user_captive_sent_cb(void *arg);

#endif
