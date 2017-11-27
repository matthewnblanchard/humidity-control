// user_fan.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Contains functions/definitions pertaining to
//	the exhaust fan

#ifndef _USER_FAN_H
#define _USER_FAN_H

#include <user_interface.h>
#include <osapi.h>
#include <gpio.h>
#include <eagle_soc.h>

// Fan supply definitions
#define TRIAC_PULSE_PERIOD	100			// Pulse length in us for driving the triac
#define FAN_RPM_MAX		3100			// Exhaust fan maximum RPM
#define SUPPLY_PERIOD 		16667 			// Fan supply period in us
#define SUPPLY_HALF_CYCLE	SUPPLY_PERIOD / 2	// Fan supply half cycle period

// Macros to calculate TRIAC delay
#define CALC_DELAY(X)		X //((-0.01541*(X)*(X)*(X)) + (0.02207*(X)*(X)) - (0.01175*(X)) + 0.006587)
#define SPEED_DELAY(X)		X //CALC_DELAY((X)/FAN_RPM_MAX) > 150 ? CALC_DELAY((X)/FAN_RPM_MAX) : 150	// Minimum delay of 150 u

// Fan/ZCD GPIO definitions
#define TRIAC_PIN	12	// GPIO pin connected to the fan triac
#define TRIAC_BIT       BIT12
#define TRIAC_MUX       PERIPHS_IO_MUX_MTDI_U
#define TRIAC_FUNC      FUNC_GPIO12
#define ZCD_PIN	        13	// GPIO pin connected to the ZCD
#define ZCD_BIT         BIT13
#define ZCD_MUX         PERIPHS_IO_MUX_MTCK_U
#define ZCD_FUNC        FUNC_GPIO13
#define TACH_PIN	14	// GPIO pin connected to the tachometer
#define TACH_BIT	BIT14
#define TACH_MUX	PERIPHS_IO_MUX_MTMS_U
#define TACH_FUNC	FUNC_GPIO14

// Tachometer definitions
#define TACH_PERIOD 500		// Tach calculaiton period in ms
#define TACH_BLADE_N 7 		// Number of fan blades with reflective tape (# of pulses per revolution)
#define DEBOUNCE_CYCLES 50

// Fan driving variables
extern volatile bool drive_flag;		// Flag to indicate if fan should be driven
extern volatile uint32 drive_delay;		// Triac delay in us
extern volatile uint16 intr_cnt;
extern volatile uint16 tach_cnt;
extern volatile uint16 rpm;
os_timer_t tach_t;
os_timer_t debounce_t;

/* MISSING DECLARATIONS FROM HW_TIMER */
typedef enum {
    FRC1_SOURCE = 0,
    NMI_SOURCE = 1,
} FRC1_TIMER_SOURCE_TYPE;

void hw_timer_arm (u32 val);
void ICACHE_FLASH_ATTR hw_timer_set_func(void (* user_hw_timer_cb_set)(void));
void ICACHE_FLASH_ATTR hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req);
/* ---------------------------------- */

// Interrupt Service Routine: user_gpio_isr(uint32 intr_mask, void *arg)
// Desc: ISR called when a GPIO interrupt happens
// Args:
//	uint32 intr_mask: Interrupt flag mask
//	void *arg:	Arguments
void user_gpio_isr(uint32 intr_mask, void *arg);

// Callback Function: user_fire_triac(void)
// Desc: Timer callback which fires the triac after
//	the timer delay
void user_fire_triac(void);

// Callback Function: user_tach_calc
// Desc: Calculates the current fan RPM based on the number of
//	tachometer pulses
void ICACHE_FLASH_ATTR user_tach_calc(void);

#endif /* _USER_FAN_H */
