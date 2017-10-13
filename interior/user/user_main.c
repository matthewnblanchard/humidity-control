/* user_main.c
 * Description: main file for the interior sensor system software
 */

#include <user_interface.h>
#include <gpio.h>
#include <osapi.h>
#include <spi_flash.h>
#include <mem.h>
#include "user_flash.h"

// IP Macros
#define IP_OCTET(ip, oct) (ip >> (8 * oct)) % 256

// Global Configurations (must be accessible from callback functions)
struct softap_config ap_config = {      // SoftAP configuration
        "HBFC/D Wireless Setup",        // SSID
        "",                             // Password
        21,                             // SSID length
        1,                              // Channel
        AUTH_OPEN,                      // Authentication mode
        false,                          // SSID hidden
        1,                              // Maximum connection #
        100                             // Beacon broadcast interval
};
struct station_config client_config;    // Station configuration

// Software Timers
os_timer_t timer_1;

// Function prototypes
void ICACHE_FLASH_ATTR user_init(void);
void ICACHE_FLASH_ATTR user_scan(void);
static void ICACHE_FLASH_ATTR user_scan_done(void *arg, STATUS status);
void ICACHE_FLASH_ATTR user_check_ip(void);
void ICACHE_FLASH_ATTR user_apmode(void);

// User Task: user_init()
// Desc: Initialization. ESP8266 hooks into this function after boot.
//      ICACHE_FLASH_ATTR flag causes the function to be saved to flash
//      rather than IRAM, of which there is limited space.
void ICACHE_FLASH_ATTR user_init(void)
{
        os_printf("user code entered\r\n");

        // Set baud rate of UART0 to 115200 for serial debugging 
        // (default is nonstandard 74880) 
        uart_div_modify(0, UART_CLK_FREQ / 115200);

        os_printf("UART speed set to 115200\r\n");

        // Register the wifi connection function for callback after system
        // system initializes. 
        system_init_done_cb(user_scan);      
}

// Callback Function: user_scan_done(void *arg, STATUS status)
// Desc: Callback once AP scan is complete. Interprets AP scan results
//      and hands off to another function accordingly.
// Args:
//      void *arg: Information on AP's found by wifi_station_scan
//      STATUS status:  get status
static void ICACHE_FLASH_ATTR user_scan_done(void *arg, STATUS status)
{
        sint8 best_rssi = -128;         // Best RSSI from found APs
        uint8 *best_bssid;              // BSSID of best AP
        struct bss_info *best_ap;       // AP with best RSSI 
        uint8 ap_count = 0;             // Number of APs found

        os_printf("AP scan succeeded, status=%d\r\n", status);
        
        // Check AP info for valid APs
        if (status == OK) {
                struct bss_info *scan_results = (struct bss_info *)arg;
                while (scan_results != NULL) {
                        ap_count++;
                        if (scan_results->rssi > best_rssi) {    // Check if RSSI beats the old best
                                best_rssi = scan_results->rssi;
                                best_ap = scan_results;
                        }
                        scan_results = scan_results->next.stqe_next;   // Move to next found AP
                }               
        }

        // If an AP was found, connect to it
        if (ap_count != 0) {
                best_bssid = best_ap->bssid;
                os_printf("found %d APs, best_rssi=%d\r\n", ap_count, best_rssi);
                os_printf("bssid=%x:%x:%x:%x:%x:%x\r\n",
                        best_bssid[0],best_bssid[1],best_bssid[2],best_bssid[3],best_bssid[4],best_bssid[5]);

                // Connect
                if (wifi_station_set_config(&client_config) == true) {
                        os_printf("station_config set\r\n");
                        if (wifi_station_connect() == true) {
                                os_printf("attempting to connect ...\r\n");
                                os_timer_setfn(&timer_1, user_check_ip, NULL);  // Set timer callback function
                                os_timer_arm(&timer_1, 1000, true);             // Check for an IP every 1000 ms
                        } else {
                                os_printf("connection attempt failed \r\n");
                        }
                } else {
                        os_printf("station_config failed\r\n");
                }
        } else {
                os_printf("no valid APs found with saved SSID\r\n");
                user_apmode();          // Create network for user to reach the ESP8266
        }
}

// Callback Function: user_scan()
// Desc: Callback once system initialization is complete. Pulls saved connection 
//      info from flash and checks for APs braodcasting that SSID. 
void ICACHE_FLASH_ATTR user_scan(void)
{
        os_printf("entered user_scan\r\n");
        struct scan_config ap_scan_config;
        struct user_data_station_config  saved_conn;
        uint8 flash_result = 0; 

        os_printf("init\r\n");

        /* -------------- */
        /* TEMPORARY CODE */
        /* -------------- */
        flash_result = spi_flash_erase_sector(USER_DATA_START_SECT);
        if (flash_result != SPI_FLASH_RESULT_OK) {
                os_printf("flash erase failed\r\n");
        }
        struct user_data_station_config test_config = {
                "Soochboys",
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
        }
        /* ------------------ */
        /* TEMPORARY CODE END */
        /* ------------------ */

        // Set ESP8266 to station (client) mode
        wifi_set_opmode(STATION_MODE);
        os_printf("opmode=station\r\n");

        // Pull station info (SSID/pass) from memory
        flash_result = spi_flash_read(
                USER_DATA_START_ADDR, 
                (uint32 *)&saved_conn, 
                sizeof(struct user_data_station_config)
        );
        if (flash_result != SPI_FLASH_RESULT_OK) {
                os_printf("flash read failed\r\n");
        }
        os_printf("read user_data, result=%d\r\n", flash_result);
        os_printf("read_ssid=%s\r\n", saved_conn.config.ssid);
        os_printf("read_pass=%s\r\n", saved_conn.config.password);
        
        client_config = saved_conn.config;    // Save retrieved station configuration for later use

        os_printf("saved client info\r\n");

        // Check for AP's broadcasting the saved SSID
        os_memset(&ap_scan_config, 0, sizeof(ap_scan_config));
        ap_scan_config.ssid = saved_conn.config.ssid;

        os_printf("scanning ...\r\n");

        if (wifi_station_scan(&ap_scan_config, user_scan_done) != true) {
                os_printf("AP scan failed\r\n");
        }
        os_printf("dropped to end\r\n");
}

// Callback Function: user_check_ip()
// Desc: Called every 1000ms to check if the ESP8266 has received an IP. Disarms timer
//      And moves forward when an IP is obtained

void ICACHE_FLASH_ATTR user_check_ip(void)
{
        struct ip_info *ip = (struct ip_info *)os_zalloc(sizeof(struct ip_info));
        uint8 status = wifi_station_get_connect_status();       // Check connection status

        os_printf("connecting ... status=%d\r\n", status);

        // Check if the ESP8266 has received an IP through DHCP
        if (status == STATION_GOT_IP) {
                os_printf("ip received\r\n");
                os_timer_disarm(&timer_1);      // Stop checking the connection
                if (wifi_get_ip_info(STATION_IF, ip) == true) { // Check ip info on station interface
                        os_printf("ip=%d.%d.%d.%d\r\n",
                        IP_OCTET(ip->ip.addr,3),IP_OCTET(ip->ip.addr,2),
                        IP_OCTET(ip->ip.addr,1),IP_OCTET(ip->ip.addr,0)); 
                        os_printf("netmask=%d.%d.%d.%d\r\n",
                        IP_OCTET(ip->netmask.addr,3),IP_OCTET(ip->netmask.addr,2),
                        IP_OCTET(ip->netmask.addr,1),IP_OCTET(ip->netmask.addr,0)); 
                        os_printf("gw=%d.%d.%d.%d\r\n",
                        IP_OCTET(ip->gw.addr,3),IP_OCTET(ip->gw.addr,2),
                        IP_OCTET(ip->gw.addr,1),IP_OCTET(ip->gw.addr,0)); 
                } else {
                        os_printf("failed tp check ip\r\n");
                }
        }
        os_free(ip);
}

// Application Function: user_apmode()
// Desc: On failure to locate the saved wireless network, switches the ESP8266
//      to AP mode. 

void ICACHE_FLASH_ATTR user_apmode(void)
{
        wifi_set_opmode_current(SOFTAP_MODE);      // Set ESP8266 to AP mode
 
        os_printf("opmode=softap\r\n");

        if (!wifi_softap_set_config(&ap_config)) {      // Configure SoftAP settings
                os_printf("softap config failed\r\n");
        } else {
                os_printf("softap config succeeded\r\n");
        }


}
