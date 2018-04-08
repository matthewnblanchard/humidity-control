// user_fan.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_fan.h"

// Variables
volatile bool drive_flag = false;				// True if the fan should be driven
volatile sint32 desired_delay = SUPPLY_HALF_CYCLE / 2;
volatile sint32 drive_delay = SUPPLY_HALF_CYCLE / 2;	// Delay on triac pulse in us
volatile uint16 tach_cnt = 0;				// Count of tachometer pulses
volatile sint32 desired_rpm = 2500;			// The desired RPM of the fan
volatile sint32 measured_rpm = 0;			// RPM measured by tachometer
volatile bool fan_on = true;

void user_gpio_isr(uint32 intr_mask, void *arg)
{
	static uint32 last_time = 0;	// Timer value for software debouncing

	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);	// Check which GPIO(s) have an interrupt queued

	// Check if a ZCD interrupt occured
	if ((intr_mask & (ZCD_BIT)) && drive_flag) {			// Pulse the triac with appropriate delay if drive flag is set
		hw_timer_arm(drive_delay);
	};

	// Check if a tachometer interrupt occured
	if (intr_mask & (TACH_BIT)) {
		uint32 cur_time = system_get_time();	// Use the system time register for software debouncing
		if ((cur_time - last_time) > 1000) {	// 1ms debounce
			tach_cnt++;
			last_time = cur_time;
		};
	};

	gpio_intr_ack(intr_mask);

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
	freq = ((float)tach_cnt * 1000) / (TACH_PERIOD * 2);	// Convert to pulses/second. Two tach counts are generated per pulse
	measured_rpm = (freq * 60) / TACH_BLADE_N;		// Convert to rotations per minute

	// When the fan is running (RPM reading above minimum), adjust triac drive delay as the measured/desired RPM error changes
	if (measured_rpm > 250) {
		if (((measured_rpm - last_rpm) < 100) && ((measured_rpm - last_rpm) > -100)) {		// Allow up to 100 RPM error
			drive_delay += (FEEDBACK_GAIN * (measured_rpm - desired_rpm));			// Proportional control
			drive_delay = (drive_delay > DELAY_BOUNDH) ? DELAY_BOUNDH : drive_delay;	// Keep drive delay within bounds
			drive_delay = (drive_delay < DELAY_BOUNDL) ? DELAY_BOUNDL : drive_delay;
		}
		last_rpm = measured_rpm;
	};

	PRINT_DEBUG(DEBUG_HIGH, "tach_cnt=%d, rpm=%d, freq=%d, drive_flag=%d, drive_delay=%d\r\n", tach_cnt, measured_rpm, (uint32)freq, drive_flag, drive_delay);

	// Reset pulse count
	tach_cnt = 0;
	return;
};
