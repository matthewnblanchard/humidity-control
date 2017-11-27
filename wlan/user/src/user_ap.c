// user_captive.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_ap.h"

// AP configuration
struct softap_config ap_config = {      // SoftAP configuration
        "hbfcd_test",                   // SSID
        "pass",                         // Password
        10,                             // SSID length
        1,                              // Channel
        AUTH_OPEN,                      // Authentication mode
        false,                          // SSID hidden
        5,                              // Maximum connection #
        1000                            // Beacon broadcast interval
};

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

        TASK_RETURN(SIG_APMODE, PAR_APMODE_SETUP_COMPLETE);
	return; 
};
