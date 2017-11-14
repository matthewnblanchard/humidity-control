// user_exterior.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_exterior.h"

char *discovery_recv_key = "Hello Interior";
uint16 discovery_recv_keylen = 14;

os_timer_t timer_exterior;	// Timer for waiting for exterior connection

void ICACHE_FLASH_ATTR user_broadcast_init(os_event_t *e)
{
	sint8 result = 0;

	// Listen on UDP port BROADCAST_PORT
	os_memset(&udp_broadcast_conn, 0, sizeof(udp_broadcast_conn));
	os_memset(&udp_broadcast_proto, 0, sizeof(udp_broadcast_proto));
	udp_broadcast_proto.local_port = BROADCAST_PORT;
	udp_broadcast_conn.type = ESPCONN_UDP;
	udp_broadcast_conn.proto.udp = &udp_broadcast_proto;
	udp_broadcast_conn.recv_callback = user_broadcast_recv_cb;

	// Begin listening
        result = espconn_create(&udp_broadcast_conn);
        if (result != 0) {
                os_printf("failed to listen for broadcast, result=%d\r\n", result);
		CALL_ERROR(ERR_FATAL);
		return;
        } else {
                os_printf("listening on udp %d\r\n", BROADCAST_PORT);
        }

	// Arm timer to wait for exterior system. If this runs out before the exterior connects,
	// the system will switch back to configure mode and wait for it, under the assumption
	// that the exterior is either not present, or lacks wifi credentials. Both systems should
	// fall back to this state in the absence of this connection, so they will become re-synced
	// after this point. 
        os_timer_setfn(&timer_exterior, user_ext_notfound_cb, NULL);
        os_timer_arm(&timer_exterior, EXT_WAIT_TIME, false);

        return;      
};

void ICACHE_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *client_conn = arg;	// Grab connection info

	os_printf("recevied data (udp %d): data=%s\r\n)", BROADCAST_PORT, pusrdata);

	// Check if the connecting client provided the discovery key
	if (os_strncmp(pusrdata, discovery_recv_key, discovery_recv_keylen)) {
		
		// If the key matches, save IP and attempt to connect to the device on the EXTERIOR_PORT
                os_memset(&tcp_espconnect_conn, 0, sizeof(tcp_espconnect_conn));
                os_memset(&tcp_espconnect_proto, 0, sizeof(tcp_espconnect_proto));

                os_memcpy(tcp_espconnect_proto.remote_ip, client_conn->proto.udp->remote_ip, 4);

		espconn_delete(&udp_broadcast_conn);

                system_os_task(user_espconnect_init, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH);
                system_os_post(USER_TASK_PRIO_1, 0, 0);                
	}
        
        return;
};

void ICACHE_FLASH_ATTR user_espconnect_init(os_event_t *e)
{
        sint8 result = 0;
        
        // Complete ESPCONNECT connection information, then connect.
        tcp_espconnect_proto.remote_port = ESP_CONNECT_PORT;
        tcp_espconnect_proto.local_port = ESP_CONNECT_PORT;
       	tcp_espconnect_conn.proto.tcp = &tcp_espconnect_proto;
	
	espconn_regist_connectcb(&tcp_espconnect_conn, user_espconnect_connect_cb);
	espconn_connect(&tcp_espconnect_conn);
};

void ICACHE_FLASH_ATTR user_espconnect_connect_cb(void *arg)
{
        struct espconn *client_conn = arg;      // Retrieve connection structure

        os_printf("connected to exterior system\r\n");

	// Disarm the exterior wait timer
	os_timer_disarm(timer_exterior);

	// Stop listening for broadcasts
	//espconn_delete(udp_broadcast_conn);

        // Register callbacks for connected client
        espconn_regist_recvcb(client_conn, user_espconnect_recv_cb);
        espconn_regist_reconcb(client_conn, user_espconnect_recon_cb);
        espconn_regist_disconcb(client_conn, user_espconnect_discon_cb);
        espconn_regist_sentcb(client_conn, user_espconnect_sent_cb);

	// Initialize the fan driving timer
	user_fan_init();
        os_printf("fan Initialized\r\n");

        // Register humidity reading timer
        os_timer_setfn(&timer_humidity, user_read_humidity, NULL);
        os_timer_arm(&timer_humidity, HUMIDITY_READ_INTERVAL, true);
        os_printf("commencing humidity readings\r\n");

        // Initiate webserver
        os_printf("initiazting webserver\r\n");
        system_os_task(user_front_init, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH);
        system_os_post(USER_TASK_PRIO_1, 0, 0);

        return;
};

void ICACHE_FLASH_ATTR user_espconnect_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *ext_conn = arg;	// Grab control structure
	
        float humidity_ext = 0;         // Exterior humidity

        // Check packet size. If it's less than EXT_PACKET_SIZE then the packet must be malformed, discard
        if (length != EXT_PACKET_SIZE) {
                os_printf("received malformed exterior packet\r\n");
                return;
        }

        // Retreive exterior humidity from packet
        os_memcpy(&humidity_ext, pusrdata[0], 4);

        // Store humidity.
        sensor_data_ext = humidity_ext;

	// Send ACK
        espconn_send(ext_conn, "ack", 3); 

        return;
};

void ICACHE_FLASH_ATTR user_espconnect_sent_cb(void *arg)
{
        os_printf("data sent to exterior\r\n");       

        return;
};

void ICACHE_FLASH_ATTR user_espconnect_recon_cb(void *arg, sint8 err)
{
        os_printf("error %d occured in connection with exterior\r\n");

        return;
};

void ICACHE_FLASH_ATTR user_espconnect_discon_cb(void *arg)
{
        os_printf("exterior system disconnected\r\n");

        return;
};

void ICACHE_FLASH_ATTR user_ext_notfound_cb(void)
{
	os_printf("Maximum exterior connection wait time elapsed\r\n");

	// Switch to AP mode
        system_os_task(user_apmode_init, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH);
        system_os_post(USER_TASK_PRIO_1, SIG_EXT_ABORT, 0);

	return; 
};
