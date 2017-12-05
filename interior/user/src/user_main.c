// user_main.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: main file for the interior sensor system software

#include <user_interface.h>
#include <osapi.h>
#include <gpio.h>
#include "user_task.h"
#include "user_network.h"
#include "user_humidity.h"
#include "user_i2c.h"
#include "user_fan.h"
#include "user_captive.h"
#include "user_exterior.h"

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
        PRINT_DEBUG(DEBUG_LOW, "user code entered\r\n");

        // Set baud rate of UART0 to 115200 for serial debugging 
        // (default is nonstandard 74880) 
        uart_div_modify(0, UART_CLK_FREQ / 115200);

        PRINT_DEBUG(DEBUG_LOW, "UART speed set to 115200\r\n");

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
	/* ==================== */
	/* Master Control Block */
	/* ==================== */
	switch(e->sig | e->par) {

		/* ----------------------- */
		/* General Control Signals */
		/* ----------------------- */

		// Control entry point. The interior begins by scanning for AP's using the
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
			PRINT_DEBUG(DEBUG_LOW, os_printf("rebooting system in 5 seconds\r\n"));
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
			PRINT_DEBUG(DEBUG_LOW, "waiting for IP ...\r\n");
			os_timer_setfn(&timer_ipcheck, user_check_ip, NULL); 	// Check for an IP every second			
			os_timer_arm(&timer_ipcheck, 1000, true);
			break;    

		// If the system does not find an AP with it's saved SSID/pass, it will enter
		// AP mode and serve a configuration webpage, where a user can enter a new SSID/pass
		case SIG_AP_SCAN | PAR_AP_SCAN_NOAP:
			PRINT_DEBUG(DEBUG_LOW, "switching to configuration mode\r\n");
			TASK_START(user_apmode_init, 0, 0);
			break;
 
		// Error cases:
		case SIG_AP_SCAN | PAR_AP_SCAN_FAILED_CONNECT:       // Failed to attempt to connect to an SSID   
		case SIG_AP_SCAN | PAR_AP_SCAN_FAILED_CONFIG:        // Failed to configure station mode  
		case SIG_AP_SCAN | PAR_AP_SCAN_FAILED_SCAN:          // Failed to begin AP scanning task 
		case SIG_AP_SCAN | PAR_AP_SCAN_FLASH_FAILURE:        // Failed to read data from flash memory   
		case SIG_AP_SCAN | PAR_AP_SCAN_STATION_MODE_FAILURE: // Failed to change the wifi mode to station
			PRINT_DEBUG(DEBUG_ERR, "RESPONSE: FATAL!\r\n");
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
			break;
			
		/* ------------------ */
		/* IP Waiting Signals */
		/* ------------------ */
		
		// Once the system has obtained an IP, disable the IP checking timer and initialize the
		// exterior connection configuration
		case SIG_IP_WAIT | PAR_IP_WAIT_GOTIP:
			os_timer_disarm(&timer_ipcheck);            
			TASK_START(user_broadcast_init, 0, 0);
			break;

		// Error case. Failed to check IP info. In theory it's still there, so ignore this
		case SIG_IP_WAIT | PAR_IP_WAIT_CHECK_FAILURE:
			PRINT_DEBUG(DEBUG_ERR, "RESPONSE: ignoring\r\n");
			break; 

		/* ------------ */
		/* mDNS Signals */
		/* ------------ */	
		/*		
		// PLACEHOLDER
		case SIG_MDNS | PAR_MDNS_CONFIG_COMPLETE:

			break; 	

		// Error cases:
		case SIG_MDNS | PAR_MDNS_INIT_FAILURE:
			os_printf("RESPONSE: FATAL!\r\n"); 
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
			break;	
		*/

		/* ----------------- */
		/* Discovery Signals */
		/* ----------------- */

		// Once discovery via udp broadcast is configured, initialize a timeout timer,
		// which will cause the system to fall back to configuration mode to re-sync
		// with the exterior system if the exterior system fails to discover it
		case SIG_DISCOVERY | PAR_DISCOVERY_CONFIG_COMPLETE:
			os_timer_setfn(&timer_extcon, user_ext_timeout, NULL);	
			os_timer_arm(&timer_extcon, EXT_WAIT_TIME, false);
			break;    
		
		// If the discovery times out, clear the config and reboot to get back to config mode
		case SIG_DISCOVERY | PAR_DISCOVERY_TIMEOUT:
			PRINT_DEBUG(DEBUG_LOW, "erasing config\r\n");

                	sint8 flash_result = FLASH_ERASE(USER_DATA_START_SECT);
                	if (flash_result != SPI_FLASH_RESULT_OK) {
                        	PRINT_DEBUG(DEBUG_ERR, "ERROR: flash erase failed\r\n");
				TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
                        	break;
                	}

			system_restart();
			break;
		
		// If a discovery packet is received, proceed to connect to the found system
		case SIG_DISCOVERY | PAR_DISCOVERY_FOUND:
			os_timer_disarm(&timer_extcon);
			TASK_START(user_espconnect_init, 0, 0);
			break;

		// Once the system is connected to the exterior, initialize the humidity readings and tachometer, then start the webserver
		case SIG_DISCOVERY | PAR_DISCOVERY_CONNECTED:
			PRINT_DEBUG(DEBUG_LOW, "initiating humidity readings\r\n");
			os_timer_setfn(&timer_humidity, user_read_humidity, NULL);		// Initialize humidity readings
			os_timer_arm(&timer_humidity, HUMIDITY_READ_INTERVAL, true);
			os_timer_setfn(&timer_tachometer, user_tach_calc, NULL);		// Initialize tachometer readings
			os_timer_arm(&timer_tachometer, TACH_PERIOD, true);
			TASK_START(user_front_init, 0, 0);
			break;

		// Error cases:
		case SIG_DISCOVERY | PAR_DISCOVERY_CONN_FAILED:		// If the connection failed, begin the discovery process again
			TASK_START(user_broadcast_init, 0, 0);
			break;	
		case SIG_DISCOVERY | PAR_DISCOVERY_MALFORMED:		// Received a malformed discovery packet. Keep going.
			break;
		case SIG_DISCOVERY | PAR_DISCOVERY_LISTEN_FAILURE:	// Failed to start listening for discovery packets
			PRINT_DEBUG(DEBUG_ERR, "RESPONSE: FATAL!\r\n");
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
			break; 
		
		/* ----------------- */
		/* Webserver Signals */
		/* ----------------- */
		
		// Error cases:
		case SIG_WEB | PAR_WEB_INIT_FAILURE:
			PRINT_DEBUG(DEBUG_ERR, "RESPONSE: FATAL!\r\n");
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
			break; 
		
		/* --------------- */
		/* AP Mode Signals */
		/* --------------- */

		// Once AP mode is configured and the system is serving the config webpage/listening for the exterior
		// system, setup is complete. Control drops off and waits for notification from the aforementioned
		// connections that either a user has entered an SSID/password or something has gone wrong.
		case SIG_APMODE | PAR_APMODE_SETUP_COMPLETE:
			PRINT_DEBUG(DEBUG_LOW, "apmode setup completed\r\n");
			break;

		// Once the system has received/saved an SSID/pass from a user, continually attempt to send
		// the credentials to the exterior system until acknowledgement is received from it.
		case SIG_APMODE | PAR_APMODE_CONFIG_RECV:
			PRINT_DEBUG(DEBUG_LOW, "WiFi details obtained from user\r\n");
			os_timer_setfn(&timer_extfwd, user_ext_send_cred, NULL);
			os_timer_arm(&timer_extfwd, 1000, true);
			break;	
		
		// Once the exterior has accepted wifi credentials, perform AP mode cleanup and switch back
		// to station mode
		case SIG_APMODE | PAR_APMODE_EXT_ACCEPT:
			PRINT_DEBUG(DEBUG_LOW, "exterior has accepted WiFi credentials\r\n");
			os_timer_disarm(&timer_extfwd);
			TASK_START(user_apmode_cleanup, 0, 0);
			break;

		// Once cleanup is complete, launch a new AP scan
		case SIG_APMODE | PAR_APMODE_CLEANUP_COMPLETE:
			PRINT_DEBUG(DEBUG_LOW, "AP mode cleanup completed\r\n");
			TASK_START(user_scan, 0, 0);
			return;

		// Error cases:
		case SIG_APMODE | PAR_APMODE_SEND_FAILURE:		// Failed to send data to the exterior. Ignore, it will try again
			PRINT_DEBUG(DEBUG_ERR, "RESPONSE: ignoring\r\n");
			break;
		case SIG_APMODE | PAR_APMODE_FLASH_FAILURE:		// Failed to erase flash data
		case SIG_APMODE | PAR_APMODE_EXT_INIT_FAILURE:		// Failed to open port to listen for exterior system
		case SIG_APMODE | PAR_APMODE_WEB_INIT_FAILURE:		// Failed to open webserver
		case SIG_APMODE | PAR_APMODE_DHCP_CONFIG_FAILURE:	// Failed to configure DHCP/IP settings
		case SIG_APMODE | PAR_APMODE_MODE_CONFIG_FAILURE:	// Failed to configure AP mode
		case SIG_APMODE | PAR_APMODE_AP_MODE_FAILURE:		// Failed to enter AP mode
			PRINT_DEBUG(DEBUG_ERR, "RESPONSE: FATAL!\r\n"); 
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
        PIN_FUNC_SELECT(TRIAC_MUX, TRIAC_FUNC);
        PIN_FUNC_SELECT(ZCD_MUX, ZCD_FUNC);
	PIN_FUNC_SELECT(TACH_MUX, TACH_FUNC);

        // Set I2C pins to open-drain
        GPIO_REG_WRITE(
		GPIO_PIN_ADDR(GPIO_ID_PIN(SDA_PIN)), 
		GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(SDA_PIN))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));
        GPIO_REG_WRITE(
		GPIO_PIN_ADDR(GPIO_ID_PIN(SCL_PIN)), 
		GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(SCL_PIN))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));

        // Enable pins
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | SDA_BIT);
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | SCL_BIT);
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | TRIAC_BIT);
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | ZCD_BIT);
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | TACH_BIT);

	// Initialize HW timer
	hw_timer_init(FRC1_SOURCE, 0);
	hw_timer_set_func(user_fire_triac); 
	
	// Initialize ZCD
	gpio_output_set(0, 0, 0, ZCD_BIT);     					// Set ZCD pin as input
        gpio_intr_handler_register(user_gpio_isr, 0);   			// Register GPIO ISR
        gpio_pin_intr_state_set(GPIO_ID_PIN(ZCD_PIN), GPIO_PIN_INTR_NEGEDGE);   // Rising edge triggers

	// Initialize tachometer interrupt
	gpio_output_set(0, 0, 0, TACH_BIT);					// Set Tachometer as input
        gpio_pin_intr_state_set(GPIO_ID_PIN(TACH_PIN), GPIO_PIN_INTR_POSEDGE);  // Rising edge triggers

        // GPIO initialization
        gpio_init();

	// Start I2C pins high
        gpio_output_set(SDA_BIT, 0, SDA_BIT, 0);
       	gpio_output_set(SCL_BIT, 0, SCL_BIT, 0);

        // Re-enable GPIO interrupts
        ETS_GPIO_INTR_ENABLE();

        return;
};
