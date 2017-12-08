// user_fan.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_fan.h"

// Variables
volatile bool drive_flag = 0;				// True if the fan should be driven
volatile sint32 drive_delay = SUPPLY_HALF_CYCLE / 2;	// Delay on triac pulse in us
volatile uint16 tach_cnt = 0;				// Count of tachometer pulses
//volatile uint32 last_time = 0;				// Time-keeping var for software debouncing
volatile sint32 desired_rpm = 2500;			// The desired RPM of the fan
volatile sint32 measured_rpm = 0;			// RPM measured by tachometer
volatile uint32 int_cnt = 0;

volatile uint32 gstatus = 0;
volatile uint32 gintr = 0;

void ICACHE_FLASH_ATTR user_fan_init(os_event_t *e)
{
	// Set GPIO ISR
	gpio_intr_handler_register(user_gpio_isr, 0);

	// Set up ZCD pin for interrupts
	//gpio_output_set(0, 0, 0, ZCD_BIT);
	gpio_pin_intr_state_set(GPIO_ID_PIN(ZCD_PIN), GPIO_PIN_INTR_POSEDGE);

	// Set up TACH pin for interrupts
	//gpio_output_set(0, 0, 0, TACH_BIT);
	gpio_pin_intr_state_set(GPIO_ID_PIN(TACH_PIN), GPIO_PIN_INTR_POSEDGE);

	return;

};

void user_gpio_isr(uint32 intr_mask, void *arg)
{
	static uint32 last_time = 0;	// Timer value for software debouncing

	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);	// Check which GPIO(s) have an interrupt queued

	//gstatus |= gpio_status;
	//gintr |= intr_mask;
	//int_cnt++;

	// Check if a ZCD interrupt occured
	if ((intr_mask & (ZCD_BIT)) && drive_flag) {			// Pulse the triac with appropriate delay if drive flag is set
		//int_cnt++;
		hw_timer_arm(drive_delay);
	};

	// Check if a tachometer interrupt occured
	if (intr_mask & (TACH_BIT)) {
		uint32 cur_time = system_get_time();	// Use the system time register for software debouncing
		if ((cur_time - last_time) > 1500) {
			tach_cnt++;
			last_time = cur_time;
		};
	};

	gpio_intr_ack(intr_mask);
	//GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, intr_mask);	// Ack interrupts
        //gpio_pin_intr_state_set(GPIO_ID_PIN(ZCD_PIN), GPIO_PIN_INTR_POSEDGE);   // Rising edge triggers

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
	float freq = 0;					// Calculated frequency
	static sint32 last_rpm = -3100;			// Last measured RPM

	// Calculate RPM
	freq = ((float)tach_cnt * 1000) / TACH_PERIOD;	// Convert to pulses/second
	measured_rpm = (freq * 60) / TACH_BLADE_N;	// Convert to rotations per minute

	if (measured_rpm > FAN_RPM_MIN) {
		if (((measured_rpm - last_rpm) < 100) && ((measured_rpm - last_rpm) > -100)) {
			drive_delay += (FEEDBACK_GAIN * (measured_rpm - desired_rpm));
			drive_delay = (drive_delay > DELAY_BOUNDH) ? DELAY_BOUNDH : drive_delay;
			drive_delay = (drive_delay < DELAY_BOUNDL) ? DELAY_BOUNDL : drive_delay;	
		}
		last_rpm = measured_rpm; 
	};

	PRINT_DEBUG(DEBUG_HIGH, "int_cnt=%d, tach_cnt=%d, rpm=%d, freq=%d, drive_flag=%d, drive_delay=%d\r\n",int_cnt, tach_cnt, measured_rpm, (uint32)freq, drive_flag, drive_delay);
//	PRINT_DEBUG(DEBUG_HIGH, "int_cnt=%d, gstatus=%x, gintr=%x\r\n", int_cnt, gstatus, gintr);	

	// Reset pulse count
	tach_cnt = 0;
	return;
};

