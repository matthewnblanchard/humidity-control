// user_humidity.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_humidity.h"

// Humidity data initializations
float sensor_data_int = 0;
float sensor_data_ext = 0;
float threshold_humidity = 40;

// Static function prototypes
static void ICACHE_FLASH_ATTR user_humidity_cmp(void);

void ICACHE_FLASH_ATTR user_read_humidity(void)
{
        uint8 status = 0;               // Status reported by humidity sensor
        uint8 read_byte = 0;            // Byte read from the humidity sensor
        uint16 humidity = 0;            // Humidity reading w/o calculations
        float adj_humidity = 0;         // Humidity reading after calculations

        // Wake up the sensor by sending a measurement request. This consists of the slave's address
        // and a single 0 bit. 
        user_i2c_start_bit();
        if (user_i2c_write_byte((SENSOR_ADDR << 1) & 0xFE) == 1) {
                PRINT_DEBUG(DEBUG_ERR, "slave failed to initiate measurement\r\n");
        	user_i2c_stop_bit();
		return;
        };
        user_i2c_stop_bit();

        // The average measurement cycle takes 36.65ms. 100ms leaves a good amount
        // of leeway
        os_delay_us(50000);
        os_delay_us(50000);
        
        // Retrieve the data now that the measurement cycle has completed. The data is sent in two bytes
	//         Byte 1                Byte 0
	// | 15 14 13 12 11 10 9 8 | 7 6 5 4 3 2 1 0| 
	//   ^  ^  ^                               ^ 
	// STATUS  HUMIDITY DATA -------------------
        user_i2c_start_bit();
        if (user_i2c_write_byte((SENSOR_ADDR << 1) | 0x01) == 1) {
                PRINT_DEBUG(DEBUG_ERR, "slave failed to receive address\r\n");
        	user_i2c_stop_bit();
		return;      
        };

        read_byte = user_i2c_read_byte(0);               // Read upper byte and send ACK (indicates more data is desired)
        status = read_byte >> 6;                         // Upper two bits are status
        humidity |= ((read_byte  & 0b00111111) << 8);    // Remainder of byte is upper 6 bits of humidity
        
        read_byte = user_i2c_read_byte(1);               // Read lower byte and send NACK (indicates no more data is desired)
        user_i2c_stop_bit();
        humidity |= read_byte;                           // Lower byte is lower 8 bits of humidity

	ETS_GPIO_INTR_ENABLE();
	gpio_intr_handler_register(user_gpio_isr, 0);

	// The formula for the conversion from the received integer "humidity count" to a floating point %RH is as follows:
	//        Humidity Count
	// %RH =  -------------- * 100%
	//	    (2^14) - 2
        adj_humidity = ((float)humidity / (float)((1 << 14) - 2)) * 100;        // Calculate RH as defined by Honeywell

	// Print results
        PRINT_DEBUG(DEBUG_HIGH, "reading=%d, humidity=%d, status=%d\r\n", humidity, (uint32)adj_humidity, status);

        // Store humidity.
        sensor_data_int = adj_humidity;

	// Compare interior/exterior humidities
	user_humidity_cmp();

	PRINT_DEBUG(DEBUG_HIGH, "ext_humidity=%d\r\n", (uint32)sensor_data_ext);
        return;
};

void ICACHE_FLASH_ATTR user_humidity_cmp(void)
{

	// Don't try to push the humidity below the threshold
	if (sensor_data_int <= threshold_humidity) {
		drive_flag = false;

	// Drive the humidity down if the interior is above the exterior
	} else if (sensor_data_int > sensor_data_ext) {
		drive_flag = true;

	// Otherise, do not try to drive the humidity down
	} else {
		drive_flag = false;
	}
	
	return;
};
