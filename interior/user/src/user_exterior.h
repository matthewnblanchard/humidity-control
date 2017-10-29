// user_exterior.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_exterior.h"

char *discovery_recv_key = "Hello Interior";
uint16 discovery_recv_keylen = 15;


void ICACHE_FLASH_ATTR user_broadcast_init(os_event_t *e)
{
	sint8 result = 0;

	// Listen on UDP port 5000
	os_memset(&udp_broadcast_conn, 0, sizeof(udp_broadcast_conn));
	os_memset(&udp_broadcast_proto, 0, sizeof(udp_broadcast_proto));
	udp_broadcast_proto.local_port = BROADCAST_PORT;
	udp_broadcast_conn.type = ESPCONN_UDP;
	udp_broadcast_conn.proto.udp = &udp_broadcast_proto;
		
};

void ICACH_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *client_conn = arg;	// Grab connection info

	// Check if the connecting client provided the discovery key
	if (os_strncmp(pusrdata, discovery_recv_key, discovery_recv_keylen) {
		
		// if the key matches, attempt to connect to the device on the EXTERIOR_PORT
										
	}

};
