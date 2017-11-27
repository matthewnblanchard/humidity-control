// user_network.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_network.h"

void ICACHE_FLASH_ATTR user_scan(os_event_t *e)
{
        struct scan_config ap_scan_config;              // AP scanning config
        struct user_data_station_config saved_conn;     // Retrieved station config
        uint8 flash_result = 0;                         // Result of flash operation 

        // Set ESP8266 to station (client) mode
        if (wifi_set_opmode_current(STATION_MODE) != true) {
		os_printf("ERROR: failed to set wifi mode to station\r\n");
		TASK_RETURN(SIG_AP_SCAN, PAR_AP_SCAN_STATION_MODE_FAILURE);
		return;
	};

        // Pull station info (SSID/pass) from memory
        flash_result = spi_flash_read(
                USER_DATA_START_ADDR, 
                (uint32 *)&saved_conn, 
                sizeof(struct user_data_station_config)
        );
        if (flash_result != SPI_FLASH_RESULT_OK) {
                os_printf("ERROR: flash read failed\r\n");
		TASK_RETURN(SIG_AP_SCAN, PAR_AP_SCAN_FLASH_FAILURE);
                return;
        }

        os_printf("read flash, result=%d\r\n", flash_result);
        os_printf("read_ssid=%s\r\n", saved_conn.config.ssid);
        os_printf("read_pass=%s\r\n", saved_conn.config.password);
     
        // Save retrieved station configuration for later use 
        client_config = saved_conn.config;

        // Check for AP's broadcasting the saved SSID
        os_memset(&ap_scan_config, 0, sizeof(ap_scan_config));  // Clear scan config
        ap_scan_config.ssid = saved_conn.config.ssid;           // Scan based on SSID only
        os_printf("scanning for APs ...\r\n");
        if (wifi_station_scan(&ap_scan_config, user_scan_done) != true) {
                os_printf("ERROR: Failed to begin AP scan\r\n");
		TASK_RETURN(SIG_AP_SCAN, PAR_AP_SCAN_FAILED_SCAN);
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
                        os_printf("ERROR: failed to set station config\r\n");
			TASK_RETURN(SIG_AP_SCAN, PAR_AP_SCAN_FAILED_CONFIG);
                        return;
                }

                // Attempt to connect, check for obtained IP every second until an IP has been received
                if (wifi_station_connect() == true) {
                        os_printf("connecting ...\r\n");
			TASK_RETURN(SIG_AP_SCAN, PAR_AP_SCAN_CONNECTED);
                } else {
                       	os_printf("ERROR: connection attempt failed\r\n");
			TASK_RETURN(SIG_AP_SCAN, PAR_AP_SCAN_FAILED_CONFIG);
                        return;
                }
        } else {
                os_printf("no valid APs found with saved SSID\r\n");    // Switch to AP mode
		TASK_RETURN(SIG_AP_SCAN, PAR_AP_SCAN_NOAP);
		return;
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
		TASK_RETURN(SIG_IP_WAIT, PAR_IP_WAIT_GOTIP);
                if (wifi_get_ip_info(STATION_IF, ip) == true) { // Check ip info on station interface
                        os_printf("ip=%d.%d.%d.%d\r\n", IP2STR(ip->ip.addr));
                        os_printf("netmask=%d.%d.%d.%d\r\n", IP2STR(ip->ip.addr));
                        os_printf("gw=%d.%d.%d.%d\r\n", IP2STR(ip->ip.addr));

                } else {
                        os_printf("ERROR: failed to check ip\r\n");
			TASK_RETURN(SIG_IP_WAIT, PAR_IP_WAIT_CHECK_FAILURE);
                }
        }
        os_free(ip);
	return;
};
