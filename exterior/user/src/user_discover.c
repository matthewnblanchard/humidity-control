// user_discover.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_discover.h"

// espconn structs - these are control structures for TCP/UDP connections
struct espconn udp_broadcast_conn;
struct _esp_udp udp_broadcast_proto;

char *discovery_key = "hbfcd_exterior_confirm";
uint16 discovery_keylen = 22;
char *discovery_packet = "key=%s&ip=%d.%d.%d.%d.";
bool int_conn = false;

void ICACHE_FLASH_ATTR user_broadcast_init(os_event_t *e)
{
	const char udp_ip[4] = {255, 255, 255, 255};	// UDP global broadcast IP
	sint8 result = 0;

	// Clear UDP control structures
	os_memset(&udp_broadcast_conn, 0, sizeof(udp_broadcast_conn)); 
	os_memset(&udp_broadcast_proto, 0, sizeof(udp_broadcast_proto)); 

	// Reset appropriate parameters and send UDP broadcast packet
	os_memcpy(udp_broadcast_proto.remote_ip, udp_ip, 4);
	udp_broadcast_proto.remote_port = UDP_DISCOVERY_PORT;
	udp_broadcast_conn.type = ESPCONN_UDP;
	udp_broadcast_conn.proto.udp = &udp_broadcast_proto;

	// Open UDP connection
	result = espconn_create(&udp_broadcast_conn);
	if (result < 0 ) {
		os_printf("ERROR %d: failed to open UDP broadcasts\r\n", result);
		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_OPEN_FAILURE);
		return;
	};

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_CONFIG_COMPLETE);
	return;
};

void ICACHE_FLASH_ATTR user_send_broadcast(void)
{
	static send_cnt = 0;		// Count of packets sent thus far
	struct ip_info ip_config;	// Current IP info
	sint8 result = 0;		// API result
	uint8 packet_buf[128];		// Buffer for packet data
	uint16 packet_len = 0;		// Length of the packet
	
	// Increment the sent packet counter. If we've already sent the max amount, send the timeout signal
	send_cnt++;	
	if (send_cnt > BROADCAST_NUM) {
		send_cnt = 0;
		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_TIMEOUT);	
		return;	
	};

	// Retrieve current IP
	if (wifi_get_ip_info(STATION_IF, &ip_config) == false) {
		os_printf("ERROR: failed to check IP address for UDP broadcast\r\n");
		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_BROADCAST_FAILURE);
		return;	
	};

	// Create UDP packet
	packet_len = os_sprintf(packet_buf, discovery_packet, discovery_key, IP2STR(&(ip_config.ip.addr)));

	result = espconn_send(&udp_broadcast_conn, packet_buf, packet_len);
	if (result < 0) {
		os_printf("ERROR: failed to send UDP broadcast packet\r\n");
		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_BROADCAST_FAILURE);
		return;	
	};
	
	return;
};

void ICACHE_FLASH_ATTR user_broadcast_stop(os_event_t *e)
{
	// Delete the broadcast connection
	espconn_delete(&udp_broadcast_conn);

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_BROADCAST_CLEANUP);
	return;
};
