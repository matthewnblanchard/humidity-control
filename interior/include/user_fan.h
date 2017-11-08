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

#define TRIAC_PULSE_PERIOD	100	// Pulse length in us for driving the triac
// Fan supply definitions
#define FAN_RPM_MAX		3100							// Exhaust fan maximum RPM
#define SUPPLY_PERIOD 		16667 							// Fan supply period in us
#define SUPPLY_HALF_CYCLE	SUPPLY_PERIOD / 2					// Fan supply half cycle period
#define CALC_DELAY(X)		(SUPPLY_HALF_CYCLE * (FAN_RPM_MAX - X)) / FAN_RPM_MAX 	// Calculate delay required for RPM X
#define SPEED_DELAY(X)		CALC_DELAY(X) > 150 ? CALC_DELAY(X) : 150		// Minimum delay of 150 us

#define TRIAC_PIN	12	// GPIO pin connected to the fan triac
#define TRIAC_BIT       BIT12
#define TRIAC_MUX       PERIPHS_IO_MUX_MTDI_U
#define TRIAC_FUNC      FUNC_GPIO12
#define ZCD_PIN	        13	// GPIO pin connected to the ZCD
#define ZCD_BIT         BIT13
#define ZCD_MUX         PERIPHS_IO_MUX_MTCK_U
#define ZCD_FUNC        FUNC_GPIO13
extern bool drive_flag;		// Flag to indicate if fan should be driven
extern uint32 drive_delay;	// Triac delay in us
extern uint16 intr_cnt;

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

#endif /* _USER_FAN_H */
