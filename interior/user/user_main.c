// user_main.c
// Authors: Christian Auspland & Matthew Blanchard
// Description: main file for the interior sensor system software

#include <user_interface.h>
#include <osapi.h>
#include "user_network.h"

// Function prototypes
void ICACHE_FLASH_ATTR user_init(void);

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

