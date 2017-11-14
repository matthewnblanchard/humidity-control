// user_fan.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_fan.h"

volatile bool drive_flag = 1;
uint16 intr_cnt = 0;
volatile uint32 drive_delay = 150;

void ICACHE_FLASH_ATTR user_fan_init()
{
	// Initialize ZCD
	gpio_output_set(0, 0, 0, ZCD_BIT);     				// Set ZCD pin as input
        gpio_intr_handler_register(user_gpio_isr, 0);   		// Register GPIO ISR
        gpio_pin_intr_state_set(ZCD_BIT, GPIO_PIN_INTR_NEGEDGE);        // Falling edge triggers
        os_printf("zcd initialized\r\n");

        return;        
};

void user_gpio_isr(uint32 intr_mask, void *arg)
{
	gpio_intr_ack(intr_mask);	// ACK interrupt

	// Check if ZCD interrupt occured
	if (intr_mask & (ZCD_BIT)) {

		// Arm the fan driving timer if necessary
		if (drive_flag) {
			hw_timer_arm(drive_delay);
		}
	
                return;
	};
};

void user_fire_triac(void)
{
	// Pulse the triac
	gpio_output_set(TRIAC_BIT, 0, TRIAC_BIT, 0);
	os_delay_us(TRIAC_PULSE_PERIOD);
	gpio_output_set(0, TRIAC_BIT, TRIAC_BIT, 0);

	return;	
};
