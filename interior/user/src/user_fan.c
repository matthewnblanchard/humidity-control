// user_fan.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_fan.h"

bool drive_flag = 0;
uint16 drive_rpm = FAN_RPM_MAX;

void ICACHE_FLASH_ATTR user_fan_init()
{
	// Set the ZCD pin as an input
	gpio_output_set(0, 0, 0, 0x01 << ZCD_GPIO);

	// Initialize HW timer
	hw_timer_init(FRC1_SOURCE, 1);
		
};

void user_gpio_isr(uint32 intr_mask, void *arg)
{
	// Check if ZCD interrupt occured
	if (intr_mask & (0x01 << ZCD_GPIO)) {

		// Arm the fan driving timer if necessary
		if (drive_flag) {
			hw_timer_arm(CALC_DELAY(drive_rpm));
		}	
		gpio_intr_ack((uint32)(0x01 << ZCD_GPIO));
	}
};

void ICACHE_FLASH_ATTR user_fire_triac(void)
{
	// Pulse the triac
	gpio_output_set(0x01 << TRIAC_GPIO, 0, 0x01 << TRIAC_GPIO, 0);
	os_delay_us(TRIAC_PULSE_PERIOD);
	gpio_output_set(0, 0x01 << TRIAC_GPIO, 0x01 << TRIAC_GPIO, 0);

	// Disarm the timer
	hw_timer_set_func(NULL); 

	return;	
};
