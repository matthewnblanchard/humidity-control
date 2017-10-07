/* user_main.c
 * Description: main file for the interior sensor system software
 */

#include <user_interface.h>
#include <gpio.h>
#include <osapi.h>

// Wifi modes
#define WIFI_STATION_MODE       1
#define WIFI_SOFTAP_MODE        2 

// Wifi SoftAP mode configuration
struct softap_config ap_config = {
        "HBFC/D Wireless Setup",        // SSID
        "",                             // Password
        21,                             // SSID length
        1,                              // Channel
        AUTH_OPEN,                      // Authentication mode
        false,                          // SSID hidden
        1,                              // Maximum connection #
        100                             // Beacon broadcast interval
};

// Function prototypes
void ICACHE_FLASH_ATTR user_init(void);
void ICACHE_FLASH_ATTR user_connect(void);
void ICACHE_FLASH_ATTR user_apmode(void);

// Task: user_init()
// Desc: Initialization. ESP8266 hooks into this function after boot.
//      ICACHE_FLASH_ATTR flag causes the function to be saved to flash
//      rather than IRAM, of which there is limited space.

void ICACHE_FLASH_ATTR user_init(void)
{
        // Set baud rate of UART0 to 115200 for serial debugging 
        // (default is nonstandard 74880) 
        uart_div_modify(0, UART_CLK_FREQ / 115200);

        // Register the wifi connection function for callback after system
        // system initializes. 
        system_init_done_cb(user_connect);      

        return;
}

// Task: user_connect()
// Desc: Attempts to connect to a wireless network with cached information. If 
//      no such cached information is available, passes off to user_apmode()

void ICACHE_FLASH_ATTR user_connect(void)
{
        os_printf("System initialized\r\n");

        wifi_set_opmode(WIFI_STATION_MODE);     // Set ESP8266 to station (client) mode

        if (wifi_station_connect()) {   // Atempt to connect to cached wireless network
                /* PLACEHOLDER */
        } else {
                user_apmode();     
        }

        return;
}

// Task: user_apmode()
// Desc: On failure to connect to a cached wireless network, switches the ESP8266
//      to AP mode, 

void ICACHE_FLASH_ATTR user_apmode(void)
{
        wifi_set_opmode_current(WIFI_SOFTAP_MODE);      // Set ESP8266 to AP mode
        
        if (!wifi_softap_set_config(&ap_config)) {      // Configure SoftAP settings
                /* ERROR */
        }

        return;

}

