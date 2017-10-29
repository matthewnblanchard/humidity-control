// user_fan.h
// Authors: Christian Auspland & Matthew Blanchard
// Description: Contains functions/definitions pertaining to
//	the exhaust fan

#ifndef _USER_FAN_H
#define _USER_FAN_H

#include <user_interface.h>
#include <osapi.h>
#include <gpio.h>

#define TRIAC_PULSE_PERIOD	3	// Pulse length in us for driving the triac
// Fan supply definitions
#define FAN_RPM_MAX		3100					// Exhaust fan maximum RPM
#define SUPPLY_PERIOD 		16667 					// Fan supply period in us
#define SUPPLY_HALF_CYCLE	SUPPLY_PERIOD / 2			// Fan supply half cycle period
#define CALC_DELAY(X)		(SUPPLY_HALF_CYCLE * X) / FAN_RPM_MAX 	// Calculate delay required for RPM X

#define TRIAC_GPIO	6	// GPIO pin connected to the fan triac
#define ZCD_GPIO	7	// GPIO pin connected to the ZCD

extern bool drive_flag;		// Flag to indicate if fan should be driven
extern uint16 drive_rpm;	// Desired fan drive speed

/* MISSING DECLARATIONS FROM HW_TIMER */
typedef enum {
    FRC1_SOURCE = 0,
    NMI_SOURCE = 1,
} FRC1_TIMER_SOURCE_TYPE;

void ICACHE_FLASH_ATTR hw_timer_arm (u32 val);
void ICACHE_FLASH_ATTR hw_timer_set_func(void (* user_hw_timer_cb_set)(void));
void ICACHE_FLASH_ATTR hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req);
/* ---------------------------------- */

// Application Function: user_fan_init()
// Desc: Initializes the GPIO pins and timers
//	necessary to drive the fan 
void ICACHE_FLASH_ATTR user_fan_init();

// Interrupt Service Routine: user_gpio_isr(uint32 intr_mask, void *arg)
// Desc: ISR called when a GPIO interrupt happens
// Args:
//	uint32 intr_mask: Interrupt flag mask
//	void *arg:	Arguments
void user_gpio_isr(uint32 intr_mask, void *arg);

// Callback Function: user_fire_triac(void)
// Desc: Timer callback which fires the triac after
//	the timer delay
void ICACHE_FLASH_ATTR user_fire_triac(void);

#endif /* _USER_FAN_H */
