// user_connect.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_connect.h"

void ICACHE_FLASH_ATTR udp_broadcast()
{
	char buffer[40] = {"Test data"};	// Broadcasted data doesn't matter
	char macaddr[6];
	struct ip_info ipsend;
	
	//Finish UDP setup and send
	const char udp_ip[4] = {255, 255, 255, 255};

	// Reset appropriate parameters and send UDP broadcast packet
	os_memcpy(udp_broadcast_conn.proto.udp->remote_ip, udp_ip, 4);
	udp_broadcast_conn.proto.udp->remote_port = UDP_DISCOVERY_PORT;	
	wifi_get_macaddr(STATION_IF, macaddr);
	espconn_send(&udp_broadcast_conn, buffer, os_strlen(buffer));
}

void ICACHE_FLASH_ATTR user_tcp_accept()
{
	os_printf("Entered user_tcp_accept()\r\n");
	server_counter = 0;
	sint8 result = 0;
	//Prepare TCP server on port 4000
	os_memset(&tcp_connect_conn, 0, sizeof(tcp_connect_conn));
	os_memset(&tcp_connect_proto, 0, sizeof(tcp_connect_proto));
	tcp_connect_proto.local_port = ESPCONNECT;
	tcp_connect_conn.type = ESPCONN_TCP;
	tcp_connect_conn.state = ESPCONN_NONE;
	tcp_connect_conn.proto.tcp = &tcp_connect_proto;
	
	// Register Callbacks for the TCP server
	result = espconn_regist_connectcb(&tcp_connect_conn, user_tcp_accept_cb);
	if (result < 0) {
		os_printf("failed to register tcp connection callback\r\n");
		CALL_ERROR(ERR_FATAL);
	}
	
	os_printf("Starting TCP server\r\n");
	// Start TCP server
	result = espconn_accept(&tcp_connect_conn);
	if (result < 0) {
		os_printf("failed to start tcp server\r\n");
		CALL_ERROR(ERR_FATAL);
	}
	
	os_printf("Entering timer loop\r\n");	
	// Wait for 30 seconds for the interior to connect to the server.
        os_timer_disarm(&timer_1);
	os_timer_setfn(&timer_1, (os_timer_func_t *)tcp_timer_cb, NULL);
        os_timer_arm(&timer_1, 1000, 1);	
	
	os_printf("The Interior did not hear the broadcast\r\n");
}

void ICACHE_FLASH_ATTR user_tcp_connect()
{
	
	sint8 result = 0;	// Function call return values
	
	// Set up TCP connection control structures
	os_memset(&tcp_connect_conn, 0, sizeof(tcp_connect_conn));
	os_memset(&tcp_connect_proto, 0, sizeof(tcp_connect_proto));
	tcp_connect_proto.local_port = HTTP_PORT;
	tcp_connect_conn.type = ESPCONN_TCP;
	tcp_connect_conn.state = ESPCONN_NONE;
	tcp_connect_conn.proto.tcp = &tcp_connect_proto;

	
	
	// Regist callback for TCP connection
	result = espconn_regist_connectcb(&tcp_connect_conn, user_tcp_connect_cb);
	if (result < 0){
		os_printf("failed to register connect callback, error=%d\n\n", result);
		CALL_ERROR(ERR_FATAL);
	}

	os_printf("Starting search for interior...\r\n");
	// Attempt to connect
	result = espconn_connect(&tcp_connect_conn);
	if (result < 0) {
		os_printf("failed to start tcp server, error=%d\r\n", result);
		CALL_ERROR(ERR_FATAL);
	}
	os_printf("does it go here?");
}

void ICACHE_FLASH_ATTR tcp_timer_cb()
{
	server_counter++;
	os_printf("Waiting for interior connection...\r\n");
	os_printf("%d seconds spent waiting\r\n", server_counter);
	if (server_counter > 30){
		if (system_os_task(user_scan_post, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH) == false){
			os_printf("failed to initialize user scan task\r\n");
			CALL_ERROR(ERR_FATAL);
		}
		if (system_os_post(USER_TASK_PRIO_1, 1, 0) == false) {
			os_printf("failed to call user_scan\r\n");
			CALL_ERROR(ERR_FATAL);
		}
		os_timer_disarm(&timer_1);
	}	
}

void ICACHE_FLASH_ATTR udp_broadcast_cb(void *arg)
{
        struct espconn *rec_conn = arg;         // Pull espconn from passed args
        //remot_info *info = NULL;                // Connection info

        os_printf("data sent\r\n");

	os_printf("deleting UDP transmission data\r\n");
	if (espconn_delete(&udp_broadcast_conn) < 0) {
		os_printf("Failed to delete UDP broadcast transmission\r\n");
		CALL_ERROR(ERR_FATAL);
	}



	os_timer_disarm(&timer_1);
	os_timer_setfn(&timer_1, (os_timer_func_t *)user_tcp_accept, NULL);
	os_timer_arm(&timer_1, 1000, 0);
	
}

void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg)
{
	os_printf("tcp connection established\r\n");
	struct espconn *client_conn = arg;

	os_timer_disarm(&timer_1);

	// Register callbacks for the connected client
	espconn_regist_recvcb(client_conn, user_tcp_recv_cb);
	espconn_regist_reconcb(client_conn, user_tcp_recon_cb);
	espconn_regist_disconcb(client_conn, user_tcp_discon_cb);
	espconn_regist_sentcb(client_conn, user_tcp_sent_cb);
}

void ICACHE_FLASH_ATTR user_tcp_accept_cb(void *arg)
{
	os_printf("Connection made to exterior over port ESPCONN\r\n");
	struct espconn *client_conn = arg;

	os_timer_disarm(&timer_1);

	// Register callbacks for the connected client
	espconn_regist_recvcb(client_conn, user_tcp_accept_recv_cb);
	espconn_regist_reconcb(client_conn, user_tcp_recon_cb);
	espconn_regist_disconcb(client_conn, user_tcp_discon_cb);
	espconn_regist_sentcb(client_conn, user_tcp_sent_cb);

}

void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *client_conn = arg;	// Pull client connection
	sint8 flash_result = 0;			// Result of flash operation
	char *p1;				// Char pointer for string manipulation
	char *p2;
	char *ssid;				// User sent SSID
	uint8 ssid_len;				// Length of the SSID
	char *pass;				// User sent password
	uint8 pass_len;				// Length of the password

	os_printf("received data from client\r\n");
	os_printf("data=%s\r\n", pusrdata);

	// Search for form data
	p1 = (uint8 *)os_strstr(pusrdata, "ssid=");	//Locate SSID
	p1 = p1 + 5;				// Move past "ssid=" to the actual ssid
	p2 = (uint8 *)os_strstr(p1, "\r\n");	// Search for &, which separates values
	ssid_len = (uint8)((uint32)p2 - (uint32)p1);	//Calculate SSID length
	ssid = p1;				// Save location of SSID
	
	p1 = (char *)os_strstr(pusrdata, "pass=");	// Locate password
	p1 = p1 + 5;				// Move past "pass=" to the actual pass
	pass_len = os_strlen(p1);		// Calculate the password length
	pass = p1;
	
	os_printf("ssid read: %s\r\n", ssid);
	os_printf("pass read: %d\r\n", pass);

	// Store retrieved data in flash
	struct user_data_station_config post_config;
	os_memcpy(post_config.config.ssid, ssid, ssid_len);
	os_memcpy(post_config.config.password, pass, pass_len);
	post_config.config.ssid[ssid_len] = '\0';
	post_config.config.password[pass_len] = '\0';


	os_printf("Writing new data to flash\r\n");

	flash_result = spi_flash_erase_sector(USER_DATA_START_SECT);
	if (flash_result != SPI_FLASH_RESULT_OK) {
		os_printf("flash erase failed!\r\n");
		CALL_ERROR(ERR_FATAL);
		return;
	}
	
	flash_result = spi_flash_write(
			USER_DATA_START_ADDR,
			(uint32 *)&post_config,
			sizeof(struct user_data_station_config)
	);

	if (flash_result != SPI_FLASH_RESULT_OK) {
		os_printf("flash write failed\r\n");
		CALL_ERROR(ERR_FATAL);
		return;
	}
	
	os_printf("deleting tcp transmission...\r\n");
	if (espconn_delete(&tcp_connect_conn) < 0) {
		os_printf("failed to delete tcp transmission\r\n");
		CALL_ERROR(ERR_FATAL);
	}

	os_printf("reentering AP scan");

	if (system_os_task(user_scan, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH) == false) {
		os_printf("failed to initialize user_scan task\r\n");
		CALL_ERROR(ERR_FATAL);
	}
	
	if (system_os_post(USER_TASK_PRIO_1, 1, 0) == false) {
		os_printf("failed to call user_scan\r\n");
		CALL_ERROR(ERR_FATAL);
	}
}

void ICACHE_FLASH_ATTR user_tcp_accept_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *client_conn = arg;	// Pull Client connection
	char *p;				// Character pointer for manipulation
	int *n;					// Integer pointer for manipulation
	int delay;				// Client sent frequency data
	uint8 delay_len;			// Length of delay data
	
	//threshold_humidity = 10;

	os_printf("received data from client\r\n");
	os_printf("data=%s\r\n", pusrdata);

	//Search for form data
	p = (uint8 *)os_strstr(pusrdata, "sensor_read_delay=");
	p = p + 18;
	/*n = p;
	delay = *n;

	os_printf("delay read: %d\r\n", delay);
	*/
	// Allocate memory for humidity sensor data buffer
	sensor_data_ext = (float *)os_zalloc(SENSOR_BUFFER_SIZE * sizeof(float));
	os_printf("humidity memory allocated\r\n");
	
	//os_printf("threshold humidity:%lf\r\n", threshold_humidity);

	// Register humidity reading timer
	os_timer_setfn(&timer_humidity, (os_timer_func_t *)user_read_humidity, NULL);	
	os_timer_arm(&timer_humidity, 3000, true);
	os_printf("commencing humidity readings\r\n");
}

void ICACHE_FLASH_ATTR user_tcp_recon_cb(void *arg, sint8 err)
{
	os_printf("tcp connection error occured\r\n");
}

void ICACHE_FLASH_ATTR user_tcp_discon_cb(void *arg)
{
	os_printf("tcp connection disconnected\r\n");
}

void ICACHE_FLASH_ATTR user_tcp_sent_cb(void *arg)
{
	os_printf("Data sent to server\r\n");
}

