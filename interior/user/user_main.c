/* user_main.c
 * Description: main file for the interior sensor system software
 */

#include <user_interface.h>
#include <gpio.h>
#include <osapi.h>
#include <spi_flash.h>
#include <mem.h>

// Wifi modes
#define WIFI_STATION_MODE       1
#define WIFI_SOFTAP_MODE        2 

// Memory Addresses - the ESP8266 doesn't have dedicated EEPROM, however the SDK
// provides functions to write to flash memory, and guarantees that certain sectors
// are left available for user data. Addresses below are handpicked from those 
// sectors, starting from 0x0003C000. Flash is handled in 4KB sectors, numbered 
// starting from 0
#define USER_DATA_START_ADDR    0x0003C000
#define USER_DATA_START_SECT    USER_DATA_START_ADDR / (4 * 1024)
#define USER_DATA_END_ADDR      0x00040000
#define USER_DATA_END_SECT      USER_DATA_END_SECT / (4 * 1024)

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

}

// Function: user_connect()
// Desc: Attempts to connect to a wireless network with cached information. If 
//      no such cached information is available, passes off to user_apmode()

void ICACHE_FLASH_ATTR user_connect(void)
{
        struct station_config *saved_conn = (struct station_config *)os_zalloc(sizeof(struct station_config));
        uint8 flash_result = 0; 

        os_printf("init\r\n");

        // Set ESP8266 to station (client) mode
        wifi_set_opmode(WIFI_STATION_MODE);
        os_printf("opmode=station\r\n");

        // Pull station info (SSID/pass) from memory
        flash_result = spi_flash_read(USER_DATA_START_ADDR, (uint32 *)&saved_conn, sizeof(struct station_config));
        if (flash_result != SPI_FLASH_RESULT_OK) {
                /* ERROR */
        }
        os_printf("read user_data, result=%d\r\n", flash_result);
        os_printf("read_ssid=%s\r\n", saved_conn->ssid);
        os_printf("read_pass=%s\r\n", saved_conn->password);

        // Configure current station info with read saved station info
        if (wifi_station_set_config(saved_conn) != true) {
                os_printf("station config set failed\r\n"); 
        } else {
                os_printf("station config set succeeded\r\n");
        }

        // Attempt to connect to saved network
        if (wifi_station_connect() == true) {
                os_free(saved_conn);
                os_printf("connected, status=%d\r\n", wifi_station_get_connect_status());
        } else {
                os_free(saved_conn);
                os_printf("connection failed\r\n");
                user_apmode();     
        }

        os_printf("reached end of user code\r\n");
}

// Function: user_apmode()
// Desc: On failure to connect to a cached wireless network, switches the ESP8266
//      to AP mode, 

void ICACHE_FLASH_ATTR user_apmode(void)
{
        wifi_set_opmode_current(WIFI_SOFTAP_MODE);      // Set ESP8266 to AP mode
 
        os_printf("opmode=softap\r\n");

        if (!wifi_softap_set_config(&ap_config)) {      // Configure SoftAP settings
                os_printf("softap config failed\r\n");
        } else {
                os_printf("softap config succeeded\r\n");
        }


}

