// user_i2c.h
// Authors: Christian Auspland & Matthew Blanchard
// Desc: Contains software i2c functions & definitions. The ESP8266 lacks i2c
// 	hardware, bit-banging: is necessary.

#ifndef USER_I2C_H
#define USER_I2C_H

#include <user_interface.h>
#include <osapi.h>
#include <gpio.h>
#include <eagle_soc.h>
#include <ets_sys.h>

// I2C pin definitions - THESE SHOULD NOT BE CHANGED
#define SCL_PIN 4
#define SCL_BIT BIT4
#define SCL_MUX PERIPHS_IO_MUX_GPIO4_U
#define SCL_FUNC FUNC_GPIO4
#define SDA_PIN 5
#define SDA_BIT BIT5
#define SDA_MUX PERIPHS_IO_MUX_GPIO5_U
#define SDA_FUNC FUNC_GPIO5

// Delay time in microseconds
#define I2C_DELAY 3

// void ICACHE_FLASH_ATTR user_i2c_start_bit(void);
// Desc: Sends an I2C start bit
// Args:
// 	None
// Returns:
// 	Nothing
void ICACHE_FLASH_ATTR user_i2c_start_bit(void);

// void ICACHE_FLASH_ATTR user_i2c_stop_bit(void);
// Desc: Sends an I2C stop bit
// Args:
// 	None
// Returns:
// 	Nothing
void ICACHE_FLASH_ATTR user_i2c_stop_bit(void);

// uint8 ICACHE_FLASH_ATTR user_i2c_write_byte(uint8 bit);
// Desc:
// 	Writes a single byte to the slave device,
// 	and checks for an ACK/NACK
// Args:
// 	uint8 byte: Byte to be written
// Return:
// 	True, if a NACK was received
// 	False, if an ACK was received
uint8 ICACHE_FLASH_ATTR user_i2c_write_byte(uint8 byte);

// uint8 ICACHCE_FLASH_ATTR user_i2c_read_byte(uint8 byte);
// Desc:
// 	Reads a single byte from the slave device,
// 	and sends an ACK/NACK.
// Args:
// 	uint9 ack: 0 to send an ACK, 1 to send a NACK
// Returns:
// 	The byte which was read
uint8 ICACHE_FLASH_ATTR user_i2c_read_byte(uint8 ack);

// void ICACHE_FLASH_ATTR user_i2c_send_ack(uint9 ack_level);
// Desc:
// 	Sends an ACK/NACK to the slave device. An argument
// 	of '0' indicates an ACK, and '1' indicates a NACK
// Args:
// 	uint8 ack_level: Ack level (ACK/NACK)
// Returns:
// 	Nothing
void ICACHE_FLASH_ATTR user_i2c_send_ack(uint8 ack_level);

// void ICACHE_FLASH_ATTR user_i2c_recv_ack(void);
// Desc:
// 	Checks for an ACK/NACK from the slave device
// Args:
// 	None
// Returns:
// 	0, if an ACK was received
// 	1, if a NACK was received
uint8 ICACHE_FLASH_ATTR user_i2c_recv_ack(void);

#endif
