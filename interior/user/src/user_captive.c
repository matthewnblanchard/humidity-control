// user_captive.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_captive.h"

// AP configuration
struct softap_config ap_config = {      // SoftAP configuration
        "HBFC/D Wireless Setup",        // SSID
        "pass",                         // Password
        21,                             // SSID length
        1,                              // Channel
        AUTH_OPEN,                      // Authentication mode
        false,                          // SSID hidden
        2,                              // Maximum connection #
        1000                            // Beacon broadcast interval
};

// HTML for captive portal webpage
char *captive_page = {
        "HTTP/1.1 200 OK\r\n\
        Content-type: text/html\r\n\r\n\
        <html>\
        <head><title>HBFC/D Wireless Config</title></head>\
        <body>\
        <h1>\
                Humidity Based Fan Controller / Dehumidifier Wifi Configuration<br>\
        </h1>\
        <p>\
                The HBFC/D either couldn't find the SSID it has saved in memory,\
                or the password was incorrect. Please enter wifi credentials and\
                hit \"Submit\"<br>\
        </p>\
        <form id=\"wifi_config\" action=\"./submit\" method=\"post\">\
                Wifi Credentials:<br><br>\
                SSID:<br>\
                <input type=\"text\" name=\"ssid\"><br>\
                Password:<br>\
                <input type=\"password\" name=\"pass\"><br>\
                <input type=\"submit\" value=\"Submit\"><br>\
        </form>\
        </body>\
        </html>"
};

// HTML for exterior system wait page
char *wait_page = {
	"HTTP/1.1 200 OK\r\n\
	Content-type: text/html\r\n\r\n\
	<html>\
	<head><title>HBFC/D Wait</title></head>\
	<body>\
	<h>\
		Humidity Based Fan Controller / Dehumidifer Wait Page<br>\
	</h1>\
	<p>\
		The exterior sensor system has not yet connected. Please wait a few seconds\
		and refresh this page\
	</p>\
	</body>\
	</html>"
};

// HTML for form submission page
char *submit_page = {
        "HTTP/1.1 200 OK\r\n\
        Content-type: text/html\r\n\r\n\
        <html>\
        <head><title>HBFC/D Config Submission</title></head>\
        <body>\
        <p>\
        Wireless configuration submitted. The system will attempt to connect to the\
        given network.\
        </p>\
        </body>\
        </html>"
};

bool captive_ext_connect = 0;
struct espconn *ext_conn = NULL;

void ICACHE_FLASH_ATTR user_apmode_init(os_event_t *e)
{
	// Terminate existing connections on fallback
	//if (e->sig == SIG_EXT_ABORT) {
	//	espconn_disconnect(&tcp_espconnect_conn);
	//	espconn_delete(&tcp_espconnect_conn);
	//	espconn_delete(&udp_broadcast_conn);
	//}

        // DHCP lease range
        struct dhcps_lease ip_range;
        IP4_ADDR(&(ip_range.start_ip), 192, 168, 0, 2);     // Leases start at 192.168.0.2
        IP4_ADDR(&(ip_range.end_ip), 192, 168, 0, 10);       // Leases end at 192.168.0.10

        // AP IP info - 192.168.0.1/28
        struct ip_info ip_config;
        IP4_ADDR(&(ip_config.ip), 192, 168, 0, 1);
        IP4_ADDR(&(ip_config.gw), 192, 168, 0, 1);
        IP4_ADDR(&(ip_config.netmask), 255, 255, 255 , 240);

        sint8 result = 0;       // Function call return values

        // Set ESP8266 to AP mode
        if (wifi_set_opmode_current(SOFTAP_MODE) == false) {
                os_printf("ERROR: failed to set WiFi mode to AP\r\n");
                TASK_RETURN(SIG_APMODE, PAR_APMODE_AP_MODE_FAILURE);
		return; 
        }

        // Configure AP settings (SSID/pass, etc)
        if (wifi_softap_set_config_current(&ap_config) == false) {
                os_printf("ERROR: softap config failed\r\n");
                TASK_RETURN(SIG_APMODE, PAR_APMODE_MODE_CONFIG_FAILURE);
		return; 
        }

        // Stop DHCP service for configuration
        if (wifi_softap_dhcps_stop() == false) {
                os_printf("ERROR: failed to stop DHCP service\r\n");
                TASK_RETURN(SIG_APMODE, PAR_APMODE_DHCP_CONFIG_FAILURE);
		return; 
        }

        // Set SoftAP IP info
        if (wifi_set_ip_info(SOFTAP_IF, &ip_config) == false) {
                os_printf("ERROR: ip info configuration failed\r\n");
                TASK_RETURN(SIG_APMODE, PAR_APMODE_DHCP_CONFIG_FAILURE);
		return; 
        }

        // Set DHCP lease range
        if (wifi_softap_set_dhcps_lease(&ip_range) == false) {
                os_printf("ERROR: failed to set dhcp lease range\r\n");
                TASK_RETURN(SIG_APMODE, PAR_APMODE_DHCP_CONFIG_FAILURE);
		return; 
        }

        // Start DHCP service
        if (wifi_softap_dhcps_start() == false) {
                os_printf("ERROR: failed to start dhcp service\r\n");
                TASK_RETURN(SIG_APMODE, PAR_APMODE_DHCP_CONFIG_FAILURE);
		return; 
        }

        // Set up TCP server on port 80
        os_memset(&tcp_captive_conn, 0, sizeof(tcp_captive_conn));      // Clear connection settings
        os_memset(&tcp_captive_proto, 0, sizeof(tcp_captive_proto));
        tcp_captive_proto.local_port = HTTP_PORT;                       // Listen for HTTP traffic
        tcp_captive_conn.type = ESPCONN_TCP;                            // TCP protocol
        tcp_captive_conn.state = ESPCONN_NONE;
        tcp_captive_conn.proto.tcp = &tcp_captive_proto;

        // Register callbacks for the TCP server
        result = espconn_regist_connectcb(&tcp_captive_conn, user_captive_connect_cb);
        if (result < 0) {
                os_printf("ERROR: failed to register connect callback, error=%d\r\n", result);
                TASK_RETURN(SIG_APMODE, PAR_APMODE_WEB_INIT_FAILURE);
		return; 
        }

        // Start listening
        result = espconn_accept(&tcp_captive_conn);      
        if (result < 0) {
                os_printf("ERROR: failed to start tcp server, error=%d\r\n", result);
                TASK_RETURN(SIG_APMODE, PAR_APMODE_WEB_INIT_FAILURE);
		return; 
        }

	// Set TCP timeout interval to 1 second
	espconn_regist_time(&tcp_captive_conn, 1, 0);

        // Set up TCP server on port 4000
        os_memset(&tcp_captive_ext_conn, 0, sizeof(tcp_captive_ext_conn));      // Clear connection settings
        os_memset(&tcp_captive_ext_proto, 0, sizeof(tcp_captive_ext_proto));
        tcp_captive_ext_proto.local_port = EXT_CONFIG_PORT;                     // Listen for TCP EXT_CONFIG_PORT traffic
        tcp_captive_ext_conn.type = ESPCONN_TCP;                            	// TCP protocol
        tcp_captive_ext_conn.state = ESPCONN_NONE;
        tcp_captive_ext_conn.proto.tcp = &tcp_captive_ext_proto;

        // Register callbacks for the TCP server
        result = espconn_regist_connectcb(&tcp_captive_ext_conn, user_captive_ext_connect_cb);
        if (result < 0) {
                os_printf("failed to register connect callback, error=%d\r\n", result);
                TASK_RETURN(SIG_APMODE, PAR_APMODE_EXT_INIT_FAILURE);
		return; 
        }

        // Start listening
        result = espconn_accept(&tcp_captive_ext_conn);      
        if (result < 0) {
                os_printf("failed to start tcp server, error=%d\r\n", result);
                TASK_RETURN(SIG_APMODE, PAR_APMODE_EXT_INIT_FAILURE);
		return; 
        }
	
	// Set TCP timeout interval to 30 minutes (1800 seconds)
	espconn_regist_time(&tcp_captive_ext_conn, 1800, 0);

        TASK_RETURN(SIG_APMODE, PAR_APMODE_SETUP_COMPLETE);
	return; 
};

void ICACHE_FLASH_ATTR user_captive_connect_cb(void *arg)
{
        os_printf("client connected to the captive portal\r\n");
        struct espconn *client_conn = arg;      // Grab connection control structure

        // Register callbacks for the connected client
        espconn_regist_recvcb(client_conn, user_captive_recv_cb);
        espconn_regist_reconcb(client_conn, user_captive_recon_cb);
        espconn_regist_disconcb(client_conn, user_captive_discon_cb);
        espconn_regist_sentcb(client_conn, user_captive_sent_cb);

	// Enable keep-alive
	espconn_set_opt(client_conn, ESPCONN_KEEPALIVE);		

	return;
};
        
void ICACHE_FLASH_ATTR user_captive_recon_cb(void *arg, sint8 err)
{
        os_printf("user connection error: %d\r\n", err);
	return;
};

void ICACHE_FLASH_ATTR user_captive_discon_cb(void *arg)
{
        os_printf("user disconnected\r\n");
};

void ICACHE_FLASH_ATTR user_captive_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
        struct espconn *client_conn = arg;                      // Pull client connection
        sint8 flash_result = 0;                                 // Result of flash operation
        uint8 *p1;                                              // Char pointers for string manipulations
        uint8 *p2;                              
        uint8 *ssid;                                            // User sent SSID
        uint8 ssid_len;                                         // Length of the SSID
        uint8 *pass;                                            // User sent password
        uint8 pass_len;                                         // Length of the password
	

        os_printf("received data from client\r\n");
        os_printf("data=%s\r\n", pusrdata);

        // Check if an HTTP GET request was received
        if (os_strncmp(pusrdata, "GET ", 4) == 0) {

                os_printf("http GET request detected\r\n");
	
		// Send the configuration page only if the exterior system is connected,
		// otherwise send the wait page
		if (captive_ext_connect == 1) {
                	os_printf("sending captive portal\r\n");
                	espconn_send(client_conn, captive_page, os_strlen(captive_page)); 
		} else {
			os_printf("sending wait page\r\n");
                	espconn_send(client_conn, wait_page, os_strlen(wait_page)); 
		}
        }

        // Check if we received an HTTP POST request
        else if (os_strncmp(pusrdata, "POST ", 5) == 0) {

		// Send submit page
                os_printf("user submitted config\r\n");
                espconn_send(client_conn, submit_page, os_strlen(submit_page));

                // Search for form data
                p1 = (char *)os_strstr(pusrdata, "ssid=");      // Locate SSID
                p1 = p1 + 5;                                    // Move past "ssid=" to the actual SSID
                p2 = os_strchr(p1, '&');                        // Search for &, which separates values
                ssid_len = (uint8)((uint32)p2 - (uint32)p1);    // Calculate SSID length
                ssid = p1;                                      // Save location of SSID

                p1 = (char *)os_strstr(pusrdata, "pass=");      // Locate password
                p1 = p1 + 5;                                    // Move past "pass=" to the actual password
                pass_len = os_strlen(p1);                       // Calculate password length.
                pass = p1;

                // Store retrieved data in flash     
                struct user_data_station_config post_config;
                os_memcpy(post_config.config.ssid, ssid, ssid_len);
                os_memcpy(post_config.config.password, pass, pass_len);
                post_config.config.ssid[ssid_len] = '\0';       // Append null terminators
                post_config.config.password[pass_len] = '\0';

                flash_result = spi_flash_erase_sector(USER_DATA_START_SECT);
                if (flash_result != SPI_FLASH_RESULT_OK) {
                        os_printf("ERROR: flash erase failed\r\n");
                        TASK_RETURN(SIG_APMODE, PAR_APMODE_FLASH_FAILURE);
                        return;
                }

                flash_result = spi_flash_write(
                        USER_DATA_START_ADDR,
                        (uint32 *)&post_config,
                        sizeof(struct user_data_station_config)
                );

                if (flash_result != SPI_FLASH_RESULT_OK) {
                        os_printf("ERROR: flash write failed\r\n");
                        TASK_RETURN(SIG_APMODE, PAR_APMODE_FLASH_FAILURE);
                        return;
                }
		
		TASK_RETURN(SIG_APMODE, PAR_APMODE_CONFIG_RECV);
		return;		
        } 
};

void ICACHE_FLASH_ATTR user_captive_sent_cb(void *arg)
{
        os_printf("sent data to client\r\n");

	// Disconnect after webpage is sent
};

void ICACHE_FLASH_ATTR user_captive_ext_connect_cb(void *arg)
{
        os_printf("exterior connected to the system\r\n");
        struct espconn *client_conn = arg;      // Grab connection control structure

	captive_ext_connect = 1;
	ext_conn = arg;

        // Register callbacks for the connected exterior system
        espconn_regist_recvcb(client_conn, user_captive_ext_recv_cb);
        espconn_regist_reconcb(client_conn, user_captive_ext_recon_cb);
        espconn_regist_disconcb(client_conn, user_captive_ext_discon_cb);
        espconn_regist_sentcb(client_conn, user_captive_ext_sent_cb);

	// Enable keep-alive
	espconn_set_opt(client_conn, ESPCONN_KEEPALIVE);		

	return;
};

void ICACHE_FLASH_ATTR user_captive_ext_recon_cb(void *arg, sint8 err)
{
	os_printf("exterior connection error, code=%d\r\n", err);

	return;
};

void ICACHE_FLASH_ATTR user_captive_ext_discon_cb(void *arg)
{
        os_printf("exterior disconnected\r\n");
	captive_ext_connect = 0;
	ext_conn = NULL;
};

void ICACHE_FLASH_ATTR user_captive_ext_recv_cb(void *arg, char *pusrdata, unsigned short length)
{
	struct espconn *conn = arg;

	os_printf("received packet from exterior system: %s\r\n", pusrdata);

	// Check if it's an accept packet from the exterior system, indicating it has received and
	// accepted WiFi configuration data
	if (os_strstr(pusrdata, "accept") != NULL) {
		TASK_RETURN(SIG_APMODE, PAR_APMODE_EXT_ACCEPT);
	};	
	
	return;
};

void ICACHE_FLASH_ATTR user_captive_ext_sent_cb(void *arg)
{
	os_printf("sent packet to exterior system\r\n");
};

void ICACHE_FLASH_ATTR user_ext_send_cred(void)
{
	uint8 buf[256];					// Data to send to the exterior system
        struct user_data_station_config *saved_conn;    // Retrieved station config
	uint8 flash_result = 0;				// Result of flash operation	
	sint16 data_len = 0;				// Length of data. Negative indicates an error
	sint8 send_result = 0;				// Result of data sending operating

	saved_conn = os_zalloc(sizeof(struct user_data_station_config));
	if (saved_conn == NULL) {
		os_printf("ERROR: failed to allocate memory for flash read\r\n");
		TASK_RETURN(SIG_APMODE, PAR_APMODE_FLASH_FAILURE);
	}

	// Check if the exterior system is available
	if (!captive_ext_connect) {
		return;
	}	

        // Pull station info (SSID/pass) from memory
        flash_result = spi_flash_read(
                USER_DATA_START_ADDR, 
                (uint32 *)saved_conn, 
                sizeof(struct user_data_station_config)
        );
        if (flash_result != SPI_FLASH_RESULT_OK) {
                os_printf("ERROR: flash read failed\r\n");
		TASK_RETURN(SIG_APMODE, PAR_APMODE_FLASH_FAILURE);
		os_free(saved_conn);
                return;
        }
	os_printf("read from flash\r\n");

	// Format SSID/Pass
	data_len = os_sprintf(buf, "ssid=%s&pass=%s", saved_conn->config.ssid, saved_conn->config.password);	

	os_printf("sprintf\r\n");
	
	// Send data to exterior system
        send_result = espconn_send(ext_conn, buf, data_len);
	if (send_result != 0) {
		os_printf("ERROR: failed to send data to exterior, code=%d\r\n", send_result);
		TASK_RETURN(SIG_APMODE, PAR_APMODE_SEND_FAILURE);
		os_free(saved_conn);
		return;
	}; 
	
	os_printf("sent data\r\n");
	os_free(saved_conn);
	return;
};

void ICACHE_FLASH_ATTR user_apmode_cleanup(os_event_t *e)
{
	sint8 result = 0;	// Function result

	// Disconnect remaining connections
	espconn_disconnect(&tcp_captive_conn);
	espconn_disconnect(&tcp_captive_ext_conn);

	// Stop listening for connections
	espconn_delete(&tcp_captive_conn);
	espconn_delete(&tcp_captive_ext_conn);

	TASK_RETURN(SIG_APMODE, PAR_APMODE_CLEANUP_COMPLETE);
	return;
};
