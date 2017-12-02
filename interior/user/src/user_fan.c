// user_fan.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_fan.h"

// Variables
volatile bool drive_flag = 1;				// True if the fan should be driven
volatile uint32 drive_delay = SUPPLY_HALF_CYCLE / 2;	// Delay on triac pulse in us
volatile uint16 tach_cnt = 0;				// Count of tachometer pulses
volatile uint32 last_time = 0;				// Time-keeping var for software debouncing
volatile uint16 desired_rpm = 2900;			// The desired RPM of the fan
volatile uint16 measured_rpm = 0;			// RPM measured by tachometer

// Static function prototypes
static void ICACHE_FLASH_ATTR user_tach_calc(void);

void user_gpio_isr(uint32 intr_mask, void *arg)
{
	gpio_intr_ack(intr_mask);					// ACK interrupt

	uint32 gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);	// Check which GPIO(s) have an interrupt queued

	// Check if a ZCD interrupt occured
	if ((gpio_status & (ZCD_BIT)) & drive_flag) {			// Pulse the triac with appropriate delay if drive flag is set
		hw_timer_arm(drive_delay);
	};

	// Check if a tachometer interrupt occured
	if (gpio_status & (TACH_BIT)) {
		uint32 cur_time = system_get_time();	// Use the system time register for software debouncing
		if (cur_time - last_time > 100) {
			tach_cnt++;
			last_time = cur_time;
		};
	};

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
	static uint32 delay_boundl = 0;			// Lower bound of pulse delay
	static uint32 delay_boundh = SUPPLY_HALF_CYCLE;	// Higher bound of pulse delay

	// Calculate RPM
	freq = ((float)tach_cnt * 1000) / TACH_PERIOD;	// Convert to pulses/second
	measured_rpm = (freq * 60) / TACH_BLADE_N;	// Convert to rotations per minute
	PRINT_DEBUG(DEBUG_HIGH, "tach_cnt=%d, rpm=%d, freq=%d\r\n", tach_cnt, measured_rpm, (uint32)freq); 

	// Adjust delay based on read RPM. If the RPM is too high, fire the triac later.
	// If it is too low, fire the triac sooner. Delay change is proportional to RPM difference
	if (measured_rpm != 0) {
		drive_delay += FEEDBACK_GAIN * (measured_rpm - desired_rpm);
		(drive_delay < delay_boundl) ? (drive_delay = delay_boundl) : 0;	// Bound limiting
		(drive_delay > delay_boundh) ? (drive_delay = delay_boundh) : 0;
	}
	
	// If the speed is 0 and the drive flag is set, the triac delay must be too low or too high (motor locks)
	// adjust the delay up or down accordingly and save 
	else if ((measured_rpm == 0) && (drive_flag)) {
		if (drive_delay >= (SUPPLY_HALF_CYCLE / 2)) {	// If the motor is locking on high delay, reduce delay and save
			drive_delay -= 50;
			delay_boundh = drive_delay; 	
		} else if (drive_delay < (SUPPLY_HALF_CYCLE / 2)) { // If the motor is locking on low delay, increase delay and save
			drive_delay += 50;
			delay_boundl = drive_delay;
		}
	};

	// Reset pulse count
	tach_cnt = 0;
	return;
};

