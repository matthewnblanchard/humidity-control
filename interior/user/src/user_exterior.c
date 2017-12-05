// user_exterior.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_exterior.h"

// Discovery key details
static const uint8 const *discovery_recv_key = "hbfcd_exterior_confirm";
static const uint16 discovery_recv_keylen = 22;

// Exterior connection flag
static bool ext_conn_flag = false;

//struct mdns_info mdns_config;	// mDNS configuration
// mDNS connection control structures
//struct espconn udp_mdns_conn;
//struct _esp_udp udp_mdns_proto;		

// UDP broadcast connections for discovery
static struct espconn udp_broadcast_conn;
static struct _esp_udp udp_broadcast_proto;

// TCP connections for interior - exterior connection
static struct espconn tcp_espconnect_conn;
static struct _esp_tcp tcp_espconnect_proto;

// Static function prototypes
static void ICACHE_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length);
static void ICACHE_FLASH_ATTR user_espconnect_connect_cb(void *arg);
static void ICACHE_FLASH_ATTR user_espconnect_recv_cb(void *arg, char *pusrdata, unsigned short length);
static void ICACHE_FLASH_ATTR user_espconnect_sent_cb(void *arg);
static void ICACHE_FLASH_ATTR user_espconnect_recon_cb(void *arg, sint8 err);
static void ICACHE_FLASH_ATTR user_espconnect_discon_cb(void *arg);

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
	sint8 result = 0;	// API call results

	// Listen on UDP port BROADCAST_PORT
	os_memset(&udp_broadcast_conn, 0, sizeof(udp_broadcast_conn));
	os_memset(&udp_broadcast_proto, 0, sizeof(udp_broadcast_proto));
	udp_broadcast_proto.local_port = BROADCAST_PORT;	
	udp_broadcast_proto.remote_port = BROADCAST_PORT;
	os_memset(udp_broadcast_proto.remote_ip, 0xFF, 4);		// Global broadcasts --> 255.255.255.255
	udp_broadcast_conn.type = ESPCONN_UDP;				// Discovery packets are UDP
	udp_broadcast_conn.proto.udp = &udp_broadcast_proto;
	udp_broadcast_conn.recv_callback = user_broadcast_recv_cb;	

	// Begin listening
        result = espconn_create(&udp_broadcast_conn);
        if (result != 0) {
                PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to listen for broadcast, result=%d\r\n", result);
		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_LISTEN_FAILURE);
		return;
        } else {
                PRINT_DEBUG(DEBUG_LOW, "listening on udp %d\r\n", BROADCAST_PORT);
        }

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_CONFIG_COMPLETE);
	return;
};

void ICACHE_FLASH_ATTR user_broadcast_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	// Expecting discovery data in the format key=xxxxx,ip=xxx.xxx.xxx.xxx.
	// Note the ending '.' and that the number of x's between each set of points can be 1-3
	
	struct espconn *client_conn = arg;	// Grab connection info
	char *p1 = NULL;			// Data pointer 1 (for navigation)
	char *p2 = NULL;			// Data pointer 2 (for navigation)
	uint32 octet = 0;			// IP octet value
	uint8 i = 0;				// Loop index

	PRINT_DEBUG(DEBUG_HIGH, "recevied data (udp %d): data=%s\r\n)", BROADCAST_PORT, pusrdata);

	// Check if the connecting client provided the discovery key
	p1 = (uint8 *)os_strstr(pusrdata, discovery_recv_key);
	if (p1 != NULL) {
	
		PRINT_DEBUG(DEBUG_HIGH, "discovery key match detected, from ip=%d.%d.%d.%d\r\n",
                	client_conn->proto.tcp->remote_ip[0],
                	client_conn->proto.tcp->remote_ip[1],
                	client_conn->proto.tcp->remote_ip[2],
                	client_conn->proto.tcp->remote_ip[3]);

		// Skip to end of key, then to start of IP    V
		p1 += discovery_recv_keylen;  // key=xxxxxxxxx&ip=xxx.xxx.xxx.xxx.
		p1 += 4;		      //                  ^
	
		// If the exterior is not yet connected, retrieve the exterior's IP from the discovery packet
		// and apply it to the TCP connection control structure	
		if (ext_conn_flag == false) {

			// Clear exterior TCP control structures in preparation for the new data
               	 	os_memset(&tcp_espconnect_conn, 0, sizeof(tcp_espconnect_conn));
                	os_memset(&tcp_espconnect_proto, 0, sizeof(tcp_espconnect_proto));

			// Iterate through each octet and convert	
			for (i = 0; i < 4; i++) {

				// Look for next decimal point		    p1-v  v-p2
				p2 = (uint8 *)os_strstr(p1, ".");	// ... xxx.xxx ...
			
				// If a decimal point isn't found, the ip is malformed
				if (p2 == NULL) {	
					PRINT_DEBUG(DEBUG_ERR, "ERROR: received malformed discovery ip\r\n");
					TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_MALFORMED);
					return;
				}

				// Convert and save octet
				octet = user_atoi(p1, p2 - p1);
				tcp_espconnect_proto.remote_ip[i] = octet;
			
				// Move first pointer up and repeat
				p1 = p2;	//			     p2-vv-p1 
				p1++;		// Discard decimal point ... xxx.xxx ...
			}
	
			PRINT_DEBUG(DEBUG_HIGH, "discovery ip=%d.%d.%d.%d\r\n",
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
        sint8 result = 0;	// API call result

	// Set connection flag
	ext_conn_flag = true;

	// Stop listening for broadcasts
	espconn_delete(&udp_broadcast_conn);
        
        // Complete the TCP connection information, which should now contain the IP of the exterior system
	tcp_espconnect_conn.type = ESPCONN_TCP;
        tcp_espconnect_proto.remote_port = ESP_CONNECT_PORT;
        tcp_espconnect_proto.local_port = ESP_CONNECT_PORT;
       	tcp_espconnect_conn.proto.tcp = &tcp_espconnect_proto;
	espconn_regist_connectcb(&tcp_espconnect_conn, user_espconnect_connect_cb);

	// Attempt to connect to the exterior system
	result = espconn_connect(&tcp_espconnect_conn);
        if (result < 0) {
                PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to connect to exterior, error=%d\r\n", result);
                TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_CONN_FAILED);
		return; 
        } else {
		PRINT_DEBUG(DEBUG_LOW, "attempted to connect to exterior, result=%d\r\n", result);
	}

	// Set the TCP timeout interval to 1800 seconds.
	espconn_regist_time(&tcp_espconnect_conn, 1800, 0);

	return;
};

void ICACHE_FLASH_ATTR user_espconnect_connect_cb(void *arg)
{
        struct espconn *client_conn = arg;      // Retrieve connection structure

        PRINT_DEBUG(DEBUG_LOW, "connected to exterior system\r\n");

        // Register callbacks for connected client
        espconn_regist_recvcb(client_conn, user_espconnect_recv_cb);
        espconn_regist_reconcb(client_conn, user_espconnect_recon_cb);
        espconn_regist_disconcb(client_conn, user_espconnect_discon_cb);
        espconn_regist_sentcb(client_conn, user_espconnect_sent_cb);

	TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_CONNECTED);
	return;
};

void ICACHE_FLASH_ATTR user_espconnect_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *ext_conn = arg;	// Grab control structure
	float *humidity_ext = (float *)os_zalloc(sizeof(float));         // Exterior humidity

	//PRINT_DEBUG(DEBUG_HIGH, "received from exterior: data=%s\r\n", pusrdata);

        // Check packet size. If it's less than EXT_PACKET_SIZE then the packet must be malformed, discard
        if (length != EXT_PACKET_SIZE) {
                PRINT_DEBUG(DEBUG_ERR, "received malformed exterior packet\r\n");
		os_free(humidity_ext);
                return;
        }

        // Retreive exterior humidity from packet
        os_memcpy(humidity_ext, &pusrdata[0], 4);

        // Store the exterior humidity
        sensor_data_ext = *humidity_ext;
	
	PRINT_DEBUG(DEBUG_HIGH, "received humidity=%d from exterior\r\n", (uint32)(*humidity_ext));

	// Send an ACK to the exterior system
        // espconn_send(ext_conn, "ack", 3); 

	os_free(humidity_ext);
        return;
};

void ICACHE_FLASH_ATTR user_espconnect_sent_cb(void *arg)
{
        PRINT_DEBUG(DEBUG_LOW, "data sent to exterior\r\n");       
        return;
};

void ICACHE_FLASH_ATTR user_espconnect_recon_cb(void *arg, sint8 err)
{
        PRINT_DEBUG(DEBUG_ERR, "an error occured in connection with exterior, code=%d\r\n", err);
        return;
};

void ICACHE_FLASH_ATTR user_espconnect_discon_cb(void *arg)
{
        PRINT_DEBUG(DEBUG_LOW, "exterior system disconnected\r\n");
        return;
};

void ICACHE_FLASH_ATTR user_ext_timeout(void)
{
	// Run only if the exterior is not yet connected
	if (ext_conn_flag == false) {
		
		PRINT_DEBUG(DEBUG_LOW, "maximum exterior connection wait time elapsed\r\n");

		// Stop listening for broadcasts
		espconn_delete(&udp_broadcast_conn);

		// Disconnect from AP
		wifi_station_disconnect();

		TASK_RETURN(SIG_DISCOVERY, PAR_DISCOVERY_TIMEOUT);
	}

	return; 
};
