// user_network.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_network.h"

void ICACHE_FLASH_ATTR user_scan(os_event_t *e)
{
        struct scan_config ap_scan_config;              // AP scanning config
        struct user_data_station_config  saved_conn;    // Retrieved station config
        uint8 flash_result = 0;                         // Result of flash operation 

        // Delete captive portal connections if task call came from it
        if (e->sig == SIG_CAPTIVE_DONE) {
                if (espconn_disconnect(&tcp_captive_conn) < 0) {
                        os_printf("failed to disconnect captive portal\r\n");
                } else {
                        os_printf("captive portal disconnected\r\n");
                }

                if (espconn_delete(&tcp_captive_conn) < 0) {
                        os_printf("failed to terminate captive portal\r\n");                
                } else {
                        os_printf("captive portal terminated\r\n");
                }
	}

        // Set ESP8266 to station (client) mode
        wifi_set_opmode_current(STATION_MODE);

        // Pull station info (SSID/pass) from memory
        flash_result = spi_flash_read(
                USER_DATA_START_ADDR, 
                (uint32 *)&saved_conn, 
                sizeof(struct user_data_station_config)
        );
        if (flash_result != SPI_FLASH_RESULT_OK) {
                os_printf("flash read failed\r\n");
                CALL_ERROR(ERR_FATAL);
                return;
        }
        os_printf("read user_data, result=%d\r\n", flash_result);
        os_printf("read_ssid=%s\r\n", saved_conn.config.ssid);
        os_printf("read_pass=%s\r\n", saved_conn.config.password);
     
        // Save retrieved station configuration for later use 
        client_config = saved_conn.config;

        // Check for AP's broadcasting the saved SSID
        os_memset(&ap_scan_config, 0, sizeof(ap_scan_config));  // Clear scan config
        ap_scan_config.ssid = saved_conn.config.ssid;           // Scan based on SSID only
        os_printf("scanning for APs\r\n");
        if (wifi_station_scan(&ap_scan_config, user_scan_done) != true) {
                os_printf("AP scan failed\r\n");
                CALL_ERROR(ERR_FATAL);
                return;
        }
};

static void ICACHE_FLASH_ATTR user_scan_done(void *arg, STATUS status)
{
        sint8 best_rssi = -128;         // Best RSSI from found APs
        uint8 *best_bssid;              // BSSID of best AP
        struct bss_info *best_ap;       // AP with best RSSI 
        uint8 ap_count = 0;             // Number of APs found

        os_printf("AP scan succeeded, status=%d\r\n", status);
        
        // Check AP info for valid APs
        if (status == OK) {
                struct bss_info *scan_results = (struct bss_info *)arg;         // Copy BSS info from arguments

                // Follow the queue of found APs to the end, comparing RSSI's for the best connection
                while (scan_results != NULL) {                          
                        ap_count++;
                        if (scan_results->rssi > best_rssi) {                   // Check if RSSI beats the old best
                                best_rssi = scan_results->rssi;
                                best_ap = scan_results;
                        }
                        scan_results = scan_results->next.stqe_next;            // Move to next found AP
                }               
        }

        // If an AP was found, connect to it
        if (ap_count != 0) {
                best_bssid = best_ap->bssid;
                os_printf("found %d APs, best_rssi=%d\r\n", ap_count, best_rssi);
                os_printf("bssid=%x:%x:%x:%x:%x:%x\r\n", MAC2STR(best_bssid));

                // Connect
                if (wifi_station_set_config(&client_config) == false) {          // Set client config (SSID/pass)
                        os_printf("station_config failed\r\n");
                        CALL_ERROR(ERR_FATAL);
                        return;
                }

                // Attempt to connect, check for obtained IP every second until an IP has been received
                if (wifi_station_connect() == true) {
                        os_printf("attempting to connect ...\r\n");
                        os_timer_setfn(&timer_ip, user_check_ip, NULL);  // Set timer callback function
                        os_timer_arm(&timer_ip, 1000, true);             // Check for an IP every 1000 ms
                } else {
                       os_printf("connection attempt failed \r\n");
                        CALL_ERROR(ERR_FATAL);
                        return;
                }
        } else {
                os_printf("no valid APs found with saved SSID\r\n");    // Switch to AP mode
                system_os_task(user_apmode_init, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH);
                system_os_post(USER_TASK_PRIO_1, 0, 0); 
        }
};

void ICACHE_FLASH_ATTR user_check_ip(void)
{
        struct ip_info *ip = (struct ip_info *)os_zalloc(sizeof(struct ip_info));

	// Check connection status
        uint8 status = wifi_station_get_connect_status();
        os_printf("connecting ... status=%d\r\n", status);

        // Check if the ESP8266 has received an IP through DHCP
        if (status == STATION_GOT_IP) {
                os_printf("ip received\r\n");
                os_timer_disarm(&timer_ip);                     // Stop checking the connection
                if (wifi_get_ip_info(STATION_IF, ip) == true) { // Check ip info on station interface
                        os_printf("ip=%d.%d.%d.%d\r\n",
                                IP_OCTET(ip->ip.addr,0),IP_OCTET(ip->ip.addr,1),
                                IP_OCTET(ip->ip.addr,2),IP_OCTET(ip->ip.addr,3)); 
                        os_printf("netmask=%d.%d.%d.%d\r\n",
                                IP_OCTET(ip->netmask.addr,0),IP_OCTET(ip->netmask.addr,1),
                                IP_OCTET(ip->netmask.addr,2),IP_OCTET(ip->netmask.addr,3)); 
                        os_printf("gw=%d.%d.%d.%d\r\n",
                                IP_OCTET(ip->gw.addr,0),IP_OCTET(ip->gw.addr,1),
                                IP_OCTET(ip->gw.addr,2),IP_OCTET(ip->gw.addr,3)); 

                	system_os_task(user_force_solo, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH);
                	//system_os_task(user_broadcast_init, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH);
                	system_os_post(USER_TASK_PRIO_1, 0, 0); 

                } else {
                        os_printf("failed tp check ip\r\n");
                }
        }
        os_free(ip);
};

void ICACHE_FLASH_ATTR user_force_solo(os_event_t *e)
{
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

	// Initate tacometer
	tach_cnt = 0;
        os_timer_setfn(&tach_t, user_tach_calc, NULL);
        os_timer_arm(&tach_t, TACH_PERIOD, true);
        //os_timer_setfn(&debounce_t, user_smooth_tach, NULL);
        //os_timer_arm(&debounce_t, 1, true);

	return;
};
