// user_fan.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_fan.h"

volatile bool drive_flag = 1;
volatile uint16 intr_cnt = 0;
volatile uint32 drive_delay = 150;
volatile uint16 tach_cnt = 0;
volatile uint16 rpm = 0;
volatile uint32 last_time = 0;

void user_gpio_isr(uint32 intr_mask, void *arg)
{
	gpio_intr_ack(intr_mask);	// ACK interrupt

	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);

	// Check if ZCD interrupt occured
	if ((gpio_status & (ZCD_BIT)) & drive_flag) {
		hw_timer_arm(drive_delay);
	};
	if (gpio_status & (TACH_BIT)) {
		uint32 cur_time = system_get_time();
		if (cur_time - last_time > 100) {
			tach_cnt++;
			last_time = cur_time;
		};
	};

	//GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status);
        return;
};

void user_fire_triac(void)
{
	// Pulse the triac
	gpio_output_set(TRIAC_BIT, 0, TRIAC_BIT, 0);
	os_delay_us(TRIAC_PULSE_PERIOD);
	gpio_output_set(0, TRIAC_BIT, TRIAC_BIT, 0);

	return;	
};

void ICACHE_FLASH_ATTR user_tach_calc(void)
{
	float freq = 0;
	freq = ((float)tach_cnt * 1000) / TACH_PERIOD;
	rpm = (freq * 60) / TACH_BLADE_N;
	os_printf("tach_cnt=%d, rpm=%d, freq=%d\r\n", tach_cnt, rpm, (uint32)freq); 
	tach_cnt = 0;
	return;
};

