// user_exterior.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_exterior.h"

char *discovery_recv_key = "hbfcd_exterior_confirm";
uint16 discovery_recv_keylen = 22;

bool ext_conn_flag = false;	// Flag indicating if the exterior is connected

//struct mdns_info mdns_config;	// mDNS configuration

// mDNS connection control structures
//struct espconn udp_mdns_conn;
//struct _esp_udp udp_mdns_proto;		

// UDP broadcast connections for discovery
struct espconn udp_broadcast_conn;
struct _esp_udp udp_broadcast_proto;

// TCP connections for interior - exterior connection
struct espconn tcp_espconnect_conn;
struct _esp_tcp tcp_espconnect_proto;

/*
void ICACHE_FLASH_ATTR user_mdns_init(os_event_t *e)
{
	//struct ip_info ip_config;	// Current IP info
	//uint8 i = 0;			// Loop index
	sint8 result = 0;		// API result
	

	if (wifi_get_ip_info(STATION_IF, &ip_config) == false) {
		os_printf("ERROR: failed to check IP address\r\n");
		TASK_RETURN(SIG_MDNS, PAR_MDNS_INIT_FAILURE);
		return;	
	};

	
	mdns_config.host_name = "hbfcd_interior";
	mdns_config.ipAddr = ip_config.ip.addr;		// Current IP
	mdns_config.server_name = "hbfcd_exterior";
	mdns_config.server_port = MDNS_PORT;
	mdns_config.txt_data[0] = "key = test";

	espconn_mdns_init(&mdns_config);
	os_printf("mDNS configuration completed\r\n");

	espconn_mdns_enable(); 

	// Configure the mDNS connection (UDP over 5353)
        os_memset(&udp_mdns_conn, 0, sizeof(udp_mdns_conn));          	// Clear connection settings
        os_memset(&udp_mdns_proto, 0, sizeof(udp_mdns_proto));
	udp_mdns_proto.local_port = MDNS_PORT;				// Special MDNS port (5353)
	udp_mdns_proto.remote_port = MDNS_PORT;
	udp_mdns_proto.local_ip[0] = 251;				// Special multicast address: 224.0.0.251
	udp_mdns_proto.local_ip[1] = 0;
	udp_mdns_proto.local_ip[2] = 0;
	udp_mdns_proto.local_ip[3] = 224;
	udp_mdns_proto.remote_ip[0] = 255;				// Special multicast address: 224.0.0.251
	udp_mdns_proto.remote_ip[1] = 255;
	udp_mdns_proto.remote_ip[2] = 255;
	udp_mdns_proto.remote_ip[3] = 255;
	udp_mdns_conn.type = ESPCONN_UDP;				// UDP Multicast
	udp_mdns_conn.proto.udp = &udp_mdns_proto;			
	
	// Register mDNS callback
	result = espconn_regist_recvcb(&udp_mdns_conn, user_mdns_recv_cb);
        if (result < 0) {
                os_printf("ERROR: failed to register receive callback, error=%d\r\n", result);
                TASK_RETURN(SIG_MDNS, PAR_MDNS_INIT_FAILURE);
		return; 
        }
		
        // Start listening
        result = espconn_create(&udp_mdns_conn);      
        if (result < 0) {
                os_printf("ERROR: failed to start mDNS connection, error=%d\r\n", result);
                TASK_RETURN(SIG_MDNS, PAR_MDNS_INIT_FAILURE);
		return; 
        }

	os_printf("mDNS enabled\r\n");
	TASK_RETURN(SIG_MDNS, PAR_MDNS_CONFIG_COMPLETE);	

	return;	
};


void ICACHE_FLASH_ATTR user_mdns_recv_cb(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *recv_conn = arg;	// Retrieve connection info

	os_printf("received mdns packet. data=%s\r\n", pusrdata);

	return;
};
*/

void ICACHE_FLASH_ATTR user_broadcast_init(os_event_t *e)
{
	sint8 result = 0;

	// Listen on UDP port BROADCAST_PORT
	os_memset(&udp_broadcast_conn, 0, sizeof(udp_broadcast_conn));
	os_memset(&udp_broadcast_proto, 0, sizeof(udp_broadcast_proto));
	udp_broadcast_proto.local_port = BROADCAST_PORT;
	udp_broadcast_proto.remote_port = BROADCAST_PORT;
	os_memset(udp_broadcast_proto.remote_ip, 0xFF, 4);		// Global broadcasts
	udp_broadcast_conn.type = ESPCONN_UDP;
	udp_broadcast_conn.proto.udp = &udp_broadcast_proto;
	udp_broadcast_conn.recv_callback = user_broadcast_recv_cb;

	// Begin listening
        result = espconn_create(&udp_broadcast_conn);
        if (result != 0) {
                os_printf("ERROR: failed to listen for broadcast, result=%d\r\n", result);
		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_LISTEN_FAILURE);
		return;
        } else {
                os_printf("listening on udp %d\r\n", BROADCAST_PORT);
        }

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_CONFIG_COMPLETE);
	return;
};

void ICACHE_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	// Expecting discovery data in the format key=xxxxx,ip=xxxxxx	
	struct espconn *client_conn = arg;	// Grab connection info
	char *p1 = NULL;			// Data pointer 1 (for navigation)
	char *p2 = NULL;			// Data pointer 2 (for navigation)
	uint32 octet = 0;			// IP octet value
	uint8 i = 0;				// Loop index

	os_printf("recevied data (udp %d): data=%s\r\n)", BROADCAST_PORT, pusrdata);

	// Check if the connecting client provided the discovery key
	p1 = (uint8 *)os_strstr(pusrdata, discovery_recv_key);
	if (p1 != NULL) {
	
		os_printf("Discovery key match detected, from ip=%d.%d.%d.%d\r\n",
                	client_conn->proto.tcp->remote_ip[0],
                	client_conn->proto.tcp->remote_ip[1],
                	client_conn->proto.tcp->remote_ip[2],
                	client_conn->proto.tcp->remote_ip[3]);

		// Skip to end of key, then to start of IP    V
		p1 += discovery_recv_keylen;  // key=xxxxxxxxx&ip=xxxxxxxx
		p1 += 4;		      //                  ^
	
		// If the exterior is not yet connected, retrieve the exterior's IP from the discovery packet
		// and apply it to the control structure	
		if (ext_conn_flag == false) {

			// Clear exterior TCP control structures in preparation for the new data
               	 	os_memset(&tcp_espconnect_conn, 0, sizeof(tcp_espconnect_conn));
                	os_memset(&tcp_espconnect_proto, 0, sizeof(tcp_espconnect_proto));

			// Iterate through each octet and convert	
			for (i = 0; i < 4; i++) {

				// Look for next decimal point
				p2 = (uint8 *)os_strstr(p1, ".");
			
				// If a decimal point isn't found, the ip is malformed
				if (p2 == NULL) {	
					os_printf("ERROR: received malformed discovery ip\r\n");
					TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_MALFORMED);
					return;
				}

				// Convert and save octet
				octet = user_atoi(p1, p2 - p1);
				tcp_espconnect_proto.remote_ip[i] = octet;
			
				// Move first pointer up and repeat
				p1 = p2;
				p1++;		// Discard decimal point
			}

			// Convert the last octet
			//os_printf("I'm here: %s\r\n", p1);
			//tcp_espconnect_proto.remote_ip[3] = user_atoi(p1, pusrdata + length - p1); 
	
			os_printf("discovery ip=%d.%d.%d.%d\r\n",
                		tcp_espconnect_proto.remote_ip[0],
                		tcp_espconnect_proto.remote_ip[1],
                		tcp_espconnect_proto.remote_ip[2],
                		tcp_espconnect_proto.remote_ip[3]);


			TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_FOUND);				
		}            
	}
        
        return;
};

void ICACHE_FLASH_ATTR user_espconnect_init(os_event_t *e)
{
        sint8 result = 0;

	// Set connection flag
	ext_conn_flag = true;

	// Stop listening for broadcasts
	espconn_delete(&udp_broadcast_conn);
        
        // Complete ESPCONNECT connection information, then connect.
	tcp_espconnect_conn.type = ESPCONN_TCP;
        tcp_espconnect_proto.remote_port = ESP_CONNECT_PORT;
        tcp_espconnect_proto.local_port = ESP_CONNECT_PORT;
       	tcp_espconnect_conn.proto.tcp = &tcp_espconnect_proto;
	
	espconn_regist_connectcb(&tcp_espconnect_conn, user_espconnect_connect_cb);
	result = espconn_connect(&tcp_espconnect_conn);
        if (result < 0) {
                os_printf("ERROR: failed to connect to exterior, error=%d\r\n", result);
                TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_CONN_FAILED);
		return; 
        }
	os_printf("attempted to connect to exterior, result=%d\r\n", result);

	// Set TCP timeout interval to 1800 second
	espconn_regist_time(&tcp_espconnect_conn, 1800, 0);

	return;
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

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_CONNECTED);
	return;

/*	// Initialize the fan driving timer
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

        return; */
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

void ICACHE_FLASH_ATTR user_ext_timeout(void)
{
	// Run only if the exterior is not yet connected
	if (ext_conn_flag == false) {
		
		os_printf("Maximum exterior connection wait time elapsed\r\n");

		// Stop listening for broadcasts
		espconn_delete(&udp_broadcast_conn);

		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_TIMEOUT);
	}

	return; 
};
