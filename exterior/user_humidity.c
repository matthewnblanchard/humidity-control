// user_humidity.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_humidity.h"

uint16 data_index_ext = 0;

void ICACHE_FLASH_ATTR user_read_humidity(void)
{
	uint8 status = 0;		// Status read by humidity sensor
	uint8 read_byte = 0;		// Byte read from the humidity sensor
	uint16 humidity = 0;		// Humidity reading w/o calculations
	char *test = "Test send data";
	float adj_humidity = 0;	
	float *hp;

	// Wake up the sensor by sending a measurement request. This consists
	// of the slaves address and a single 0 bit.
	user_i2c_start_bit();
	if (user_i2c_write_byte((SENSOR_ADDR << 1) | 0x00) == 1) {
		os_printf("slave failed to initiate measurement\r\n");

	};
	user_i2c_stop_bit();

	// The average measurement cycle takes 36.65ms. 100ms leaves a 
	// good amount of leeway
	os_delay_us(50000);
	os_delay_us(50000);
	// Retrieve the data now that the measurement cycle has completed
	
	user_i2c_start_bit();
	if (user_i2c_write_byte((SENSOR_ADDR << 1) | 0x01) == 1) {
		os_printf("slave failed to receive address\r\n");
	};


	read_byte = user_i2c_read_byte(0);	// Read upper byte
	status = read_byte >> 6;		// Upper two bits are status
	
	// Remainder of byte is upper 6 bits of humidity
	humidity |= ((read_byte & 0b00111111) << 8);

	read_byte = user_i2c_read_byte(1);	// Read lower byte
	user_i2c_stop_bit();
	// Lower byte is lower 8 bits of humidity
	humidity |= read_byte;
	
	// Calculate RH as defined by Honeywell
	adj_humidity = ((float)humidity / (float)((1 << 14) - 2)) * 100;

	os_printf("reading=%d, humidity=%d, status=%d\r\n", humidity, (uint32)adj_humidity, status);
	
	// Store humidity. Rotate to bottom of buffer if full
	sensor_data_ext[data_index_ext] = adj_humidity;

	// Wrap around if buffer is full
	(data_index_ext >= SENSOR_BUFFER_SIZE -1) ? (data_index_ext = 0) : (data_index_ext++);

	hp = &adj_humidity;
	
	os_printf("Sending humidity data to interior\r\n");
	os_printf("adjusted humidity: %d\r\n", (uint32)*hp);	
	// Send newly read humidity data to the TCP connected system.
	if (espconn_send(&tcp_connect_conn, (uint8 *)hp, sizeof(uint16)) != 0) {
		os_printf("humidity data send failed\r\n");
	}
}


