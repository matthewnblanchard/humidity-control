// user_network.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_network.h"

void ICACHE_FLASH_ATTR user_scan(os_event_t *e)
{
        struct scan_config ap_scan_config;              // AP scanning config
        struct user_data_station_config  saved_conn;    // Retrieved station config
        uint8 flash_result = 0;                         // Result of flash operation 

	int_scan = 0;
        /* -------------- */
        /* TEMPORARY CODE */
        /* -------------- */
        flash_result = spi_flash_erase_sector(USER_DATA_START_SECT);
        if (flash_result != SPI_FLASH_RESULT_OK) {
                os_printf("flash erase failed\r\n");
                CALL_ERROR(ERR_FATAL);
                return;
        }
        struct user_data_station_config test_config = {
                "TESTTTTTT",
                "purplefinch654",
                0x00
        };
        flash_result = spi_flash_write(
                USER_DATA_START_ADDR,
                (uint32 *)&test_config,
                sizeof(struct user_data_station_config)
        );
        if (flash_result != SPI_FLASH_RESULT_OK) {
                os_printf("flash write failed\r\n");
                CALL_ERROR(ERR_FATAL);
                return;
        }
        /* ------------------ */
        /* TEMPORARY CODE END */
        /* ------------------ */

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
	char ssid[32] = "HBFC/D Wireless Setup";
	char password[64] = "pass";
	 struct scan_config ap_scan_config;	// AP scanning config

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
			os_printf("found AP: %s\r\n", scan_results->ssid);
                        scan_results = scan_results->next.stqe_next;            // Move to next found AP
                }               
        }

        // If an AP was found, connect to it
        if (ap_count != 0) {
                best_bssid = best_ap->bssid;
                os_printf("found %d APs, best_rssi=%d\r\n", ap_count, best_rssi);
                os_printf("bssid=%x:%x:%x:%x:%x:%x\r\n",        // BSSI MAC in hex
                        best_bssid[0],best_bssid[1],best_bssid[2],best_bssid[3],
                        best_bssid[4],best_bssid[5]);

                // Save wifi configurations
                if (wifi_station_set_config(&client_config) == false) {          // Set client config (SSID/pass)
                        os_printf("station_config failed\r\n");
                        CALL_ERROR(ERR_FATAL);
                        return;
                }

                // Attempt to connect, check for obtained IP every second until an IP has been received
                if (wifi_station_connect() == true) {
                        os_printf("attempting to connect ...\r\n");
                        os_timer_setfn(&timer_1, user_check_ip, NULL);  // Set timer callback function
                        os_timer_arm(&timer_1, 1000, true);             // Check for an IP every 1000 ms
                } else {
                        os_printf("connection attempt failed \r\n");
                        client_config.bssid_set = 0;
			
			//configs ap_scan_config for the interior SSID and password instead of
			//flash stored SSID and password	
			os_memcpy(&client_config.ssid, ssid, 32);
			os_memcpy(&client_config.password, password, 64);
			os_memset(&ap_scan_config, 0, sizeof(ap_scan_config));
			ap_scan_config.ssid = station_conn.ssid;
			
			//starts a new scan
			if(wifi_station_scan(&ap_scan_config, user_scan_done) != true) {
				os_printf("AP scan has failed\r\n");
				CALL_ERROR(ERR_FATAL);
			}
                        return;
                }
        } else {
                os_printf("no valid APs found with saved SSID\r\n");    // Switch to AP mode
		client_config.bssid_set = 0;
		//lets the scan function know it's looking for interior SSID
		int_scan = 1;	

		//configs ap_scan_config for the interior SSID and password instead of 
		//flash stored SSID and password
		os_memcpy(&client_config.ssid, ssid, 32);
		os_memcpy(&client_config.password, password, 64);
		os_memset(&ap_scan_config, 0, sizeof(ap_scan_config));
		ap_scan_config.ssid = station_conn.ssid;
		
		//starts new scan
		if (wifi_station_scan(&ap_scan_config, user_scan_done) != true) {
			os_printf("AP scan has failed\r\n");
			CALL_ERROR(ERR_FATAL);
		}
	}
};

void ICACHE_FLASH_ATTR user_check_ip(void)
{
        struct ip_info *ip = (struct ip_info *)os_zalloc(sizeof(struct ip_info));
	const char udp_broadcast_ip[4] = {255,255,255,255};
        uint8 status = wifi_station_get_connect_status();       // Check connection status
	//uint8 int_scan = 0;

        os_printf("connecting ... status=%d\r\n", status);

        //Check if the ESP8266 has received an IP through DHCP
        if (status == STATION_GOT_IP) {
                os_printf("ip received\r\n");
                os_timer_disarm(&timer_1);                      // Stop checking the connection
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

                        if(int_scan == 0){
				// Set up UDP listening connection
                        	os_printf("Entering UDP block\r\n");
				os_memset(&udp_broadcast_conn,
					       	0,
					       	sizeof(udp_broadcast_conn));        // Clear control structure
                        	
				os_memset(&udp_broadcast_proto, 
						0, 
						sizeof(udp_broadcast_proto));       // Clear protocol structure
                       	
		       		udp_broadcast_conn.type = ESPCONN_UDP;              // UDP protocol
                       	
				//commented out because not sure if necessary
				// udp_listen_conn.state = ESPCONN_NONE;            // UDP lacks state info, so no state
                        
				udp_broadcast_conn.proto.udp = (esp_udp *)
					os_zalloc(sizeof(esp_udp)); // Point to protocol info
                        
				//udp_listen_conn.recv_callback = udp_listen_cb;    // Callback function on received data
                        
				udp_broadcast_proto.local_port = UDP_DISCOVERY_PORT;// Local port
			
				//copy broadcast address to control structure
				os_memcpy(udp_broadcast_conn.proto.udp->remote_ip, 
						udp_broadcast_ip, 
						4);
			
				espconn_regist_sentcb(&udp_broadcast_conn, 
						udp_broadcast_cb);	//register send packet callback

				os_printf("configured udp listening\r\n");
                        	if (espconn_create(&udp_broadcast_conn) < 0) {
                        	        os_printf("failed to start broadcasting\r\n");
                        	} else {
                        	        os_printf("started broadcasting\r\n");
					udp_broadcast(); //send udp data
                        	}
			} else if( int_scan == 1){
				//TCP connection stuff goes here
				os_printf("Entering TCP block");
				user_tcp_connect();
			}	
                } else {
                        os_printf("failed tp check ip\r\n");
                }
        }
        os_free(ip);
};
