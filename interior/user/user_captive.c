// user_captive.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_captive.h"

struct softap_config ap_config = {      // SoftAP configuration
        "HBFC/D Wireless Setup",        // SSID
        "pass",                         // Password
        22,                             // SSID length
        1,                              // Channel
        AUTH_OPEN,                      // Authentication mode
        false,                          // SSID hidden
        2,                              // Maximum connection #
        1000                            // Beacon broadcast interval
};

void ICACHE_FLASH_ATTR user_apmode_init(os_event_t *e)
{
        // DHCP lease range
        struct dhcps_lease ip_range;
        IP4_ADDR(&(ip_range.start_ip), 192, 168, 0, 2);     // Leases start at 192.168.0.2
        IP4_ADDR(&(ip_range.end_ip), 192, 168, 0, 3);       // Leases end at 192.168.0.3

        // AP IP info - 192.168.0.1/29
        struct ip_info ip_config;
        IP4_ADDR(&(ip_config.ip), 192, 168, 0, 1);
        IP4_ADDR(&(ip_config.gw), 192, 168, 0, 1);
        IP4_ADDR(&(ip_config.netmask), 255, 255, 255 , 248);

        sint8 result = 0;       // Function call return values

        // Set ESP8266 to AP mode
        if (wifi_set_opmode_current(SOFTAP_MODE) == false) {
                os_printf("failed to set to ap mode\r\n");
                CALL_ERROR(ERR_FATAL); 
        }

        // Configure AP settings (SSID/pass, etc)
        if (wifi_softap_set_config_current(&ap_config) == false) {
                os_printf("softap config failed\r\n");
                CALL_ERROR(ERR_FATAL);
        }

        // Stop DHCP service for configuration
        if (wifi_softap_dhcps_stop() == false) {
                os_printf("failed to stop DHCP service\r\n");
                CALL_ERROR(ERR_FATAL);
        }

        // Set SoftAP IP info
        if (wifi_set_ip_info(SOFTAP_IF, &ip_config) == false) {
                os_printf("ip info configuration failed\r\n");
                CALL_ERROR(ERR_FATAL);
        }

        // Set DHCP lease range
        if (wifi_softap_set_dhcps_lease(&ip_range) == false) {
                os_printf("failed to set dhcp lease range\r\n");
                CALL_ERROR(ERR_FATAL);
        }

        // Start DHCP service
        if (wifi_softap_dhcps_start() == false) {
                os_printf("failed to start dhcp service\r\n");
                CALL_ERROR(ERR_FATAL);
        }

        // Set up TCP server on port 80
        os_memset(&tcp_captive_conn, 0, sizeof(tcp_captive_conn));      // Clear connection settings
        os_memset(&tcp_captive_proto, 0, sizeof(tcp_captive_proto));
        tcp_captive_proto.local_port = HTTP_PORT;                       // Listen for HTTP traffic
        tcp_captive_conn.type = ESPCONN_TCP;                            // TCP protocol
        tcp_captive_conn.proto.tcp = &tcp_captive_proto;

        // Register callbacks for the TCP server
        result = espconn_regist_connectcb(&tcp_captive_conn, user_captive_connect_cb);
        if (result < 0) {
                os_printf("failed to register connect callback, error=%d\r\n", result);
                CALL_ERROR(ERR_FATAL);
        }

        // Start listening
        result = espconn_accept(&tcp_captive_conn);      
        if (result < 0) {
                os_printf("failed to start tcp server, error=%d\r\n", result);
                CALL_ERROR(ERR_FATAL);
        }
};

void ICACHE_FLASH_ATTR user_captive_connect_cb(void *arg)
{
        os_printf("client connected to the captive portal\r\n");
};
