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

	// Attempt to connect
	result = espconn_connect(&tcp_connect_conn);
	if (result < 0) {
		os_printf("failed to start tcp server, error=%d\r\n", result);
		CALL_ERROR(ERR_FATAL);
	}
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

void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg)
{
	os_printf("tcp connection established");
	struct espconn *client_conn = arg;

	// Register callbacks for the connected client
	espconn_regist_recvcb(client_conn, user_tcp_recv_cb);
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
	p1 = (char *)os_strstr(pusrdata, "ssid=");	//Locate SSID
	p1 = p1 + 5;				// Move past "ssid=" to the actual ssid
	p2 = os_strchr(p1, "\r\n");		// Search for &, which separates values
	ssid_len = (uint8)((uint32)p2 - (uint32)p1);	//Calculate SSID length
	ssid = p1;				// Save location of SSID
	
	p1 = (char *)os_strstr(pusrdata, "pass=");	// Locate password
	p1 = p1 + 5;				// Move past "pass=" to the actual pass
	pass_len = os_strlen(p1);		// Calculate the password length
	pass = p1;

	// Store retrieved data in flash
	struct user_data_station_config post_config;
	os_memcpy(post_config.config.ssid, ssid, ssid_len);
	os_memcpy(post_config.config.password, pass, pass_len);
	post_config.config.ssid[ssid_len] = '\0';
	post_config.config.password[pass_len] = '\0';

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

	if (system_os_task(user_scan, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH) == false) {
		os_printf("failed to initialize user_scan task\r\n");
		CALL_ERROR(ERR_FATAL);
	}
	
	if (system_os_post(USER_TASK_PRIO_1, 1, 0) == false) {
		os_printf("failed to call user_scan\r\n");
		CALL_ERROR(ERR_FATAL);
	}
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

