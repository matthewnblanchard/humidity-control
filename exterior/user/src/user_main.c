// user_main.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: main file for the exterior sensor system software

#include <user_interface.h>
#include <osapi.h>
#include <gpio.h>
#include "user_task.h"
#include "user_network.h"
#include "user_humidity.h"
#include "user_i2c.h"
#include "user_captive.h"
#include "user_discover.h"
#include "user_connect.h"

// Function prototypes
void ICACHE_FLASH_ATTR user_init(void);				// First step initialization function. Handoff from bootloader.
void ICACHE_FLASH_ATTR user_task_init(void);			// Task initialization. Hands off to the control task.
void ICACHE_FLASH_ATTR user_control_task(os_event_t *e);	// Main control task. Schedules all other user tasks.
void ICACHE_FLASH_ATTR user_gpio_init(void);			// Performs GPIO initialization

// User Task: user_init()
// Desc: Initialization. The ESP8266 hooks into this function after boot.
//      ICACHE_FLASH_ATTR flag causes the function to be saved to flash
//      rather than IRAM, in which there is limited space.
void ICACHE_FLASH_ATTR user_init(void)
{
        os_printf("user code entered\r\n");

        // Set baud rate of UART0 to 115200 for serial debugging 
        // (default is nonstandard 74880) 
        uart_div_modify(0, UART_CLK_FREQ / 115200);

        os_printf("UART speed set to 115200\r\n");

        // Initialize GPIO interfaces
        user_gpio_init();

        // Once the system has finished initializing,
        //      continue initializing user tasks
        system_init_done_cb(user_task_init);      
};

// Callback Function: user_task_init()
// Desc: Initializes data structures for posting tasks to the system
void ICACHE_FLASH_ATTR user_task_init(void)
{
        // Allocate space for message queues
        user_msg_queue_0 = (os_event_t *)os_malloc(sizeof(os_event_t) * MSG_QUEUE_LENGTH);
        user_msg_queue_1 = (os_event_t *)os_malloc(sizeof(os_event_t) * MSG_QUEUE_LENGTH);
        user_msg_queue_2 = (os_event_t *)os_malloc(sizeof(os_event_t) * MSG_QUEUE_LENGTH);

        // Register control task and begin control
        system_os_task(user_control_task, USER_TASK_PRIO_2, user_msg_queue_2, MSG_QUEUE_LENGTH);
	TASK_RETURN(SIG_CONTROL, PAR_CONTROL_START);

	return;
};

// User Task: user_control_task(os_event_t *e)
// Desc: Maximum priority control task which calls other tasks
//	based on the results of previous tasks
void ICACHE_FLASH_ATTR user_control_task(os_event_t *e)
{
	// Control flags
	static bool config_mode = false;

	/* ==================== */
	/* Master Control Block */
	/* ==================== */
	switch(e->sig | e->par) {

		/* ----------------------- */
		/* General Control Signals */
		/* ----------------------- */

		// Control entry point. The exterior begins by scanning for AP's using the
		// SSID/password it has saved in memory.
		case SIG_CONTROL | PAR_CONTROL_START:
			TASK_START(user_scan, 0, 0);
			break;

		// Deadloop of task calls while waiting for the system to restart
		case SIG_CONTROL | PAR_CONTROL_ERR_DEADLOOP:
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_DEADLOOP);
			break;

		// Fatal error case. The control function calls itself with this sig/par
		// if it is passed an error it cannot recover from. The system is restarted
		// after a 5 second delay
		case SIG_CONTROL | PAR_CONTROL_ERR_FATAL:
			os_printf("rebooting system in 5 seconds\r\n");
			os_timer_setfn(&timer_reboot, system_restart, NULL);
			os_timer_arm(&timer_reboot, 5000, false);
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_DEADLOOP);
			break;

		/* ------------------- */
		/* AP Scanning Signals */
		/* ------------------- */

		// Once the system has found and associated to an AP, it waits to receive an IP
		// address
		case SIG_AP_SCAN | PAR_AP_SCAN_CONNECTED:
			os_timer_setfn(&timer_ipcheck, user_check_ip, NULL); // Check for an IP every second			
			os_timer_arm(&timer_ipcheck, 1000, true);
			break;    

		// If the system does not find an AP with it's saved SSID/pass, it will enter
		// AP mode and serve a configuration webpage, where a user can enter a new SSID/pass
		case SIG_AP_SCAN | PAR_AP_SCAN_NOAP:
			config_mode = true;
			os_printf("switching to configuration mode\r\n");
			TASK_START(user_config_assoc_init, 0, 0);
			break;
 
		// Error cases:
		case SIG_AP_SCAN | PAR_AP_SCAN_FAILED_CONNECT:       // Failed to attempt to connect to an SSID   
		case SIG_AP_SCAN | PAR_AP_SCAN_FAILED_CONFIG:        // Failed to configure station mode  
		case SIG_AP_SCAN | PAR_AP_SCAN_FAILED_SCAN:          // Failed to begin AP scanning task 
		case SIG_AP_SCAN | PAR_AP_SCAN_FLASH_FAILURE:        // Failed to read data from flash memory   
		case SIG_AP_SCAN | PAR_AP_SCAN_STATION_MODE_FAILURE: // Failed to change the wifi mode to station
			os_printf("RESPONSE: FATAL!\r\n");
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
			break;
			
		/* ------------------ */
		/* IP Waiting Signals */
		/* ------------------ */
		
		// Once the system has obtained an IP, disable the IP checking timer and initialize the
		// exterior connection configuration
		case SIG_IP_WAIT | PAR_IP_WAIT_GOTIP:
			os_timer_disarm(&timer_ipcheck);
			if (config_mode) {
				TASK_START(user_config_connect_init, 0, 0);
			} else {
				TASK_START(user_int_connect_init, 0, 0);
			}
			break;

		// Error case. Failed to check IP info. In theory it's still there, so ignore this
		case SIG_IP_WAIT | PAR_IP_WAIT_CHECK_FAILURE:
			os_printf("RESPONSE: ignoring\r\n");
			break; 

		/* ----------------- */
		/* Discovery Signals */
		/* ----------------- */
		
		// Once the system has started to listen for the interior, configure it to broadcast
		// discovery packets
		case SIG_DISCOVERY | PAR_DISCOVERY_LISTEN_INIT:
			TASK_START(user_broadcast_init, 0, 0);
			break;	

		// Once discovery via udp broadcast is configured, begin broadcasting the discovery
		// regularly
		case SIG_DISCOVERY | PAR_DISCOVERY_CONFIG_COMPLETE:
			os_timer_setfn(&timer_intcon, user_send_broadcast, NULL);	
			os_timer_arm(&timer_intcon, BROADCAST_PERIOD, true);
			break;    
		
		// If the discovery times out, go back to configuration mode
		case SIG_DISCOVERY | PAR_DISCOVERY_TIMEOUT:
			os_printf("discovery timed out\r\n");
			config_mode = true;
			os_timer_disarm(&timer_intcon);
			TASK_START(user_int_connect_cleanup, 0, 0);
			break;
		case SIG_DISCOVERY | PAR_DISCOVERY_INT_CLEANUP:
			TASK_START(user_broadcast_stop, 0, 0);
			break;
		case SIG_DISCOVERY | PAR_DISCOVERY_BROADCAST_CLEANUP:
			TASK_START(user_config_assoc_init, 0, 0);
			break;
					
		// Once the interior is connected to the exterior, initialize the humidity readings
		case SIG_DISCOVERY | PAR_DISCOVERY_CONNECTED:
			os_printf("interior connected");
			break;

		// Error cases:
		case SIG_DISCOVERY | PAR_DISCOVERY_BROADCAST_FAILURE:	// Broadcast failed to send this time. Continue
			os_printf("RESPONSE: ignoring\r\n");
			break;
		case SIG_DISCOVERY | PAR_DISCOVERY_LISTEN_FAILURE:	// Failed to start listening for the interior with TCP
		case SIG_DISCOVERY | PAR_DISCOVERY_OPEN_FAILURE:	// Failed to start listening for discovery packets
			os_printf("RESPONSE: FATAL!\r\n");
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
			break; 

		/* ------------------- */
		/* Config Mode Signals */
		/* ------------------- */

		// Once associate mode is initiated, attempt to connect to the interior every second until sucessful
		case SIG_CONFIG | PAR_CONFIG_ASSOC_INIT:
			os_timer_setfn(&timer_assoc, user_config_assoc, NULL);	
			os_timer_arm(&timer_assoc, 1000, true);
			break;

		// Once the system has sucessfully associated, wait until an IP has been received
		case SIG_CONFIG | PAR_CONFIG_ASSOC:
			os_timer_disarm(&timer_assoc);
			os_timer_setfn(&timer_ipcheck, user_check_ip, NULL); // Check for an IP every second			
			os_timer_arm(&timer_ipcheck, 1000, true);
			break;    
	
		// Once the system has received WiFi creds, cleanup config mode connections
		case SIG_CONFIG | PAR_CONFIG_RECV:
			config_mode = false;
			TASK_START(user_config_cleanup, 0, 0);
			break;
		
		// Once cleanup is complete, relaunch the AP scan
		case SIG_CONFIG | PAR_CONFIG_CLEANUP_COMPLETE:
			os_printf("re-entering AP scan\r\n");
			TASK_START(user_scan, 0, 0);	
			break;

		// Error cases:
		case SIG_CONFIG | PAR_CONFIG_MALFORMED:		// Received a malformed config packet
			os_printf("RESPONSE: ignoring\r\n");
			break;
		case SIG_CONFIG | PAR_CONFIG_STATION_MODE_FAILURE:	// Failed to set system to station mode
		case SIG_CONFIG | PAR_CONFIG_CONFIG_FAILURE:	// Failed to configure system to connect to interior SSID
		case SIG_CONFIG | PAR_CONFIG_FLASH_FAILURE:	// Failed to save config to flash
		case SIG_CONFIG | PAR_CONFIG_CONNECT_FAILED:	// Failed to connect to interior via TCP 
		case SIG_CONFIG | PAR_CONFIG_SETUP_FAILED:	// Failed to setup TCP connection to interior
			os_printf("RESPONSE: FATAL!\r\n");
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
			break; 
			

		};	

	return;
};

// Application Function: user_gpio_init(void)
// Desc: Initializes the GPIO pins
void ICACHE_FLASH_ATTR user_gpio_init(void)
{
        // Disable GPIO interrupts during initialization
        ETS_GPIO_INTR_DISABLE();

        // Set pin functions to GPIO 
        PIN_FUNC_SELECT(SDA_MUX, SDA_FUNC);
        PIN_FUNC_SELECT(SCL_MUX, SCL_FUNC);

        // Set I2C pins to open-drain
        GPIO_REG_WRITE(
		GPIO_PIN_ADDR(GPIO_ID_PIN(SDA_MUX)), 
		GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(SDA_MUX))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));
        GPIO_REG_WRITE(
		GPIO_PIN_ADDR(GPIO_ID_PIN(SCL_MUX)), 
		GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(SCL_MUX))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));

        // Enable pins
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | SDA_BIT);
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | SCL_BIT);

        // GPIO initialization
        gpio_init();

        // Re-enable GPIO interrupts
        ETS_GPIO_INTR_ENABLE();

        return;
};
