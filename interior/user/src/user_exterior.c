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

        result = espconn_create(&udp_broadcast_conn);
        if (result != 0) {
                os_printf("failed to listen for broadcast, result=%d\r\n", result);
        } else {
                os_printf("listening on udp %d\r\n", BROADCAST_PORT);
        } 

        return;      
};

void ICACHE_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *client_conn = arg;	// Grab connection info

	// Check if the connecting client provided the discovery key
	if (os_strncmp(pusrdata, discovery_recv_key, discovery_recv_keylen)) {
		
		// If the key matches, save IP and attempt to connect to the device on the EXTERIOR_PORT
                os_memset(&tcp_espconnect_conn, 0, sizeof(tcp_espconnect_conn));
                os_memset(&tcp_espconnect_proto, 0, sizeof(tcp_espconnect_proto));

                os_memcpy(tcp_espconnect_proto.remote_ip, client_conn->proto.udp->remote_ip, 4);

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
};

void ICACHE_FLASH_ATTR user_espconnect_connect_cb(void *arg)
{
        struct espconn *client_conn = arg;      // Retrieve connection structure

        os_printf("connected to exterior system\r\n");
        
        // Register callbacks for connected client
        espconn_regist_recvcb(client_conn, user_espconnect_recv_cb);
        espconn_regist_reconcb(client_conn, user_espconnect_recon_cb);
        espconn_regist_disconcb(client_conn, user_espconnect_discon_cb);
        espconn_regist_sentcb(client_conn, user_espconnect_sent_cb);

        return;
};

void ICACHE_FLASH_ATTR user_espconnect_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
        float humidity_ext = 0;         // Exterior humidity

        // Check packet size. If it's less than EXT_PACKET_SIZE then the packet must be malformed, discard
        if (length != EXT_PACKET_SIZE) {
                os_printf("received malformed exterior packet\r\n");
                return;
        }

        // Retreive exterior humidity from packet
        os_memcpy(&humidity_ext, pusrdata[0], 4);

        // Store humidity. Rotate to bottom of buffer if full
        sensor_data_ext[data_index_ext] = humidity_ext;
        (data_index_ext >= SENSOR_BUFFER_SIZE - 1) ? (data_index_ext = 0) : (data_index_ext++);         // Wrap around buffer if full

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
