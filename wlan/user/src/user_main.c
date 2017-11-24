// user_main.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: Main control file for the test Wireless
// Local Area Network (WLAN)

#include <user_interface.h>
#include <osapi.h>
#include <mem.h>
#include "user_task.h"
#include "user_ap.h"

// Function prototypes
void ICACHE_FLASH_ATTR user_init(void);				// First step initialization function. Handoff from bootloader.
void ICACHE_FLASH_ATTR user_task_init(void);			// Task initialization. Hands off to the control task.
void ICACHE_FLASH_ATTR user_control_task(os_event_t *e);	// Main control task

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

		// Control entry point. The interior begins by initializing AP mode.
		case SIG_CONTROL | PAR_CONTROL_START:
			TASK_START(user_apmode_init, 0, 0);
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

		/* --------------- */
		/* AP Mode Signals */
		/* --------------- */
		
		// Once AP mode initializtion is complete, we're done
		case SIG_APMODE | PAR_APMODE_SETUP_COMPLETE:
			break;

		// Error cases
		case SIG_APMODE | PAR_APMODE_DHCP_CONFIG_FAILURE:	// Failed to configure DHCP/IP settings
		case SIG_APMODE | PAR_APMODE_MODE_CONFIG_FAILURE:	// Failed to configure AP mode
		case SIG_APMODE | PAR_APMODE_AP_MODE_FAILURE:		// Failed to enter AP mode
			os_printf("RESPONSE: FATAL!\r\n"); 
			TASK_RETURN(SIG_CONTROL, PAR_CONTROL_ERR_FATAL);
			break;	

			
	};
	return;
};
