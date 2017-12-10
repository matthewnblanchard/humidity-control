// user_captive.c

#include "user_captive.h"

// espconn structs - these are control structures for TCP/UDP connections
struct espconn tcp_connect_conn;
struct _esp_tcp tcp_connect_proto;

// Interior config SSID
static struct station_config interior_ssid = {
	"HBFC/D Wireless Setup",	// SSID
	"",				// Password
	false,				// Connect by BSSID flag
	""				// BSSID
};

void ICACHE_FLASH_ATTR user_config_assoc_init(os_event_t *e)
{
	// Ensure the system is in station mode
        if (wifi_set_opmode_current(STATION_MODE) != true) {
		PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to set wifi mode to station\r\n");
		TASK_RETURN(SIG_CONFIG, PAR_CONFIG_STATION_MODE_FAILURE);
		return;
	};

	// Configure the system to connect to the interior's SSID
	if (wifi_station_set_config(&interior_ssid) != true) {
		PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to configure station mode\r\n");
		TASK_RETURN(SIG_CONFIG, PAR_CONFIG_CONFIG_FAILURE);
		return;
	};		
	
	TASK_RETURN(SIG_CONFIG, PAR_CONFIG_ASSOC_INIT);
	return;
};

void ICACHE_FLASH_ATTR user_config_assoc(void)
{
	if (wifi_station_connect() == true) {
		PRINT_DEBUG(DEBUG_LOW, "connected to interior\r\n");
		TASK_RETURN(SIG_CONFIG, PAR_CONFIG_ASSOC);
		return;
	};
};

void ICACHE_FLASH_ATTR user_config_connect_init(os_event_t *e)
{
	
	sint8 result = 0;	// Function call return values
	
	// Set up TCP connection control structures
	os_memset(&tcp_connect_conn, 0, sizeof(tcp_connect_conn));
	os_memset(&tcp_connect_proto, 0, sizeof(tcp_connect_proto));
	tcp_connect_proto.remote_port = CONFIG_PORT;
	tcp_connect_proto.local_port = CONFIG_PORT;
	tcp_connect_proto.remote_ip[0] = 192;		// Address of interior on the HBFC/D Wireless Config Network
	tcp_connect_proto.remote_ip[1] = 168;
	tcp_connect_proto.remote_ip[2] = 0;
	tcp_connect_proto.remote_ip[3] = 1;
	tcp_connect_conn.type = ESPCONN_TCP;
	tcp_connect_conn.proto.tcp = &tcp_connect_proto;

	// Regist callback for TCP connection
	result = espconn_regist_connectcb(&tcp_connect_conn, user_tcp_connect_cb);
	if (result < 0){
		PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to register connect callback, error=%d\n\n", result);
		TASK_RETURN(SIG_CONFIG, PAR_CONFIG_SETUP_FAILED);
		return;
	}

	// Attempt to connect
	result = espconn_connect(&tcp_connect_conn);
	if (result < 0) {
		PRINT_DEBUG(DEBUG_ERR, "ERROR: failed to connect to interior, error=%d\r\n", result);
		TASK_RETURN(SIG_CONFIG, PAR_CONFIG_CONNECT_FAILED);
	}

	// Set TCP timeout interval to 1800 second
	espconn_regist_time(&tcp_connect_conn, 1800, 0);

	return;
};

void ICACHE_FLASH_ATTR user_tcp_connect_cb(void *arg)
{
	PRINT_DEBUG(DEBUG_LOW, "tcp connection established\r\n");
	struct espconn *client_conn = arg;

	// Register callbacks for the connected client
	espconn_regist_recvcb(client_conn, user_tcp_recv_cb);
	
	return;
};

void ICACHE_FLASH_ATTR user_tcp_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *client_conn = arg;		// Pull client connection
	struct user_data_station_config post_config;	// Flash storage structure
	sint8 flash_result = 0;				// Result of flash operation
	char *p1;					// Char pointer for string manipulation
	char *p2;
	char *ssid;				// User sent SSID
	uint8 ssid_len;				// Length of the SSID
	char *pass;				// User sent password
	uint8 pass_len;				// Length of the password

	PRINT_DEBUG(DEBUG_LOW, "received data from interior\r\n");
	PRINT_DEBUG(DEBUG_HIGH, "data=%s\r\n", pusrdata);

	// Search for form data
	p1 = (uint8 *)os_strstr(pusrdata, "ssid=");	//Locate SSID
	if (p1 == NULL) {
		PRINT_DEBUG(DEBUG_ERR, "ERROR: received malformed config packet\r\n");
		TASK_RETURN(SIG_CONFIG, PAR_CONFIG_MALFORMED);
		return;
	}

	p1 = p1 + 5;					// Move past "ssid=" to the actual ssid
	p2 = (uint8 *)os_strstr(p1, "&");		// Search for &, which separates values
	ssid_len = (uint8)((uint32)p2 - (uint32)p1);	// Calculate SSID length
	ssid = p1;					// Save location of SSID
	
	p1 = (char *)os_strstr(pusrdata, "pass=");	// Locate password
	p1 = p1 + 5;					// Move past "pass=" to the actual pass
	pass_len = os_strlen(p1);			// Calculate the password length
	pass = p1;
	
	PRINT_DEBUG(DEBUG_HIGH, "ssid read: %s\r\n", ssid);
	PRINT_DEBUG(DEBUG_HIGH, "pass read: %d\r\n", pass);

	// Store retrieved data in flash data structure
	os_memcpy(post_config.config.ssid, ssid, ssid_len);
	os_memcpy(post_config.config.password, pass, pass_len);
	post_config.config.ssid[ssid_len] = '\0';	// Add null terminators
	post_config.config.password[pass_len] = '\0';


	PRINT_DEBUG(DEBUG_LOW, "Writing new data to flash\r\n");

	flash_result = spi_flash_erase_sector(USER_DATA_START_SECT);
	if (flash_result != SPI_FLASH_RESULT_OK) {
		PRINT_DEBUG(DEBUG_ERR, "ERROR: flash erase failed!\r\n");
		TASK_RETURN(SIG_CONFIG, PAR_CONFIG_FLASH_FAILURE);
		return;
	}
	
	flash_result = spi_flash_write(
			USER_DATA_START_ADDR,
			(uint32 *)&post_config,
			sizeof(struct user_data_station_config)
	);

	if (flash_result != SPI_FLASH_RESULT_OK) {
		PRINT_DEBUG(DEBUG_ERR, "ERROR: flash write failed\r\n");
		TASK_RETURN(SIG_CONFIG, PAR_CONFIG_FLASH_FAILURE);
		return;
	};

	// Send accept packet
	espconn_send(&tcp_connect_conn, "accept", 6);

	TASK_RETURN(SIG_CONFIG, PAR_CONFIG_RECV);
	return;
	
}

void ICACHE_FLASH_ATTR user_config_cleanup(os_event_t *e)
{
	// Disconnect/delete config mode connections
	espconn_disconnect(&tcp_connect_conn);
	espconn_delete(&tcp_connect_conn);

	// Diconnect from HBFC/D Wireless Config network
	wifi_station_disconnect();
	
	TASK_RETURN(SIG_CONFIG, PAR_CONFIG_CLEANUP_COMPLETE);
	return;
};
