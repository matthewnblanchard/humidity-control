// user_main.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: main file for the interior sensor system software

#include <user_interface.h>
#include <osapi.h>
#include "user_network.h"
#include "user_task.h"

// Function prototypes
void ICACHE_FLASH_ATTR user_init(void);
void ICACHE_FLASH_ATTR user_task_init(void);

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

        // Once the system has finished initializing,
        //      continue initializing user tasks
        system_init_done_cb(user_task_init);      
}

// Callback Function: user_task

void ICACHE_FLASH_ATTR user_task_init(void)
{
        // Allocate space for message queues
        user_msg_queue_0 = (os_event_t *)os_malloc(sizeof(os_event_t) * MSG_QUEUE_LENGTH);
        user_msg_queue_1 = (os_event_t *)os_malloc(sizeof(os_event_t) * MSG_QUEUE_LENGTH);
        user_msg_queue_2 = (os_event_t *)os_malloc(sizeof(os_event_t) * MSG_QUEUE_LENGTH);

        // Register error task
        system_os_task(user_task_error, USER_TASK_PRIO_2, user_msg_queue_2, MSG_QUEUE_LENGTH);

        // Register AP scanning task for station mode initialization, then handoff to it
        if (system_os_task(user_scan, USER_TASK_PRIO_1, user_msg_queue_1, MSG_QUEUE_LENGTH) == false) {
                os_printf("failed to initialize user_scan task\r\n");
                CALL_ERROR(ERR_FATAL);
        }
        if (system_os_post(USER_TASK_PRIO_1, 0, 0) == false) {
                os_printf("failed to call user_scan\r\n");
                CALL_ERROR(ERR_FATAL);
        }
}
