// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_connect.h"

void ICACHE_FLASH_ATTR udp_broadcast()
{
	//Stuff will go here soon.
	char buffer[40] = {"Test data"};
	char macaddr[6];
	struct ip_info ipsend;

	const char udp_ip[4] = {255, 255, 255, 255};
	//remote_ip needs to be reset everytime espconn_sent is called
	os_memcpy(udp_broadcast_conn.proto.udp->remote_ip, udp_ip, 4);
	//remote_port needs to be reset everytime espconn_sent is called
	udp_broadcast_conn.proto.udp->remote_port = UDP_DISCOVERY_PORT;	
	
	wifi_get_macaddr(STATION_IF, macaddr);
	
	espconn_send(&udp_broadcast_conn, buffer, os_strlen(buffer));	
}	
void ICACHE_FLASH_ATTR udp_broadcast_cb(void *arg)
{
        struct espconn *rec_conn = arg;         // Pull espconn from passed args
        //remot_info *info = NULL;                // Connection info

        os_printf("data sent\r\n");
	
	os_timer_disarm(&timer_1);
	os_timer_setfn(&timer_1, (os_timer_func_t *)udp_broadcast, NULL);
	os_timer_arm(&timer_1, 1000, 0);
	
	/*
        // Get connection info, print source IP
        espconn_get_connection_info(rec_conn, &info, 0);
        os_printf("src_ip=%d.%d.%d.%d\r\n",
                info->remote_ip[0],
                info->remote_ip[1],
                info->remote_ip[2],
                info->remote_ip[3]
        );
	*/
}
