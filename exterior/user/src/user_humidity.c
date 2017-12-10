// user_humidity.c
// Authors: Christian Auspland & Matthew Blanchard

#include "user_humidity.h"
#include "user_task.h"

float sensor_data_int = 0;
float sensor_data_ext = 0;
float threshold_humidity = 40;

void ICACHE_FLASH_ATTR user_read_humidity(void)
{
        uint8 status = 0;               // Status reported by humidity sensor
        uint8 read_byte = 0;            // Byte read from the humidity sensor
        uint16 humidity = 0;            // Humidity reading w/o calculations
        float adj_humidity = 0;         // Humidity reading after calculations

	PRINT_DEBUG(DEBUG_LOW, "reading humidity\r\nn");

        // Wake up the sensor by sending a measurement request. This consists of the slave's address
        // and a single 0 bit. 
        user_i2c_start_bit();
        if (user_i2c_write_byte((SENSOR_ADDR << 1) & 0xFE) == 1) {
                PRINT_DEBUG(DEBUG_ERR, "slave failed to initiate measurement\r\n");
        	user_i2c_stop_bit();
		return;
        };
        user_i2c_stop_bit();

	PRINT_DEBUG(DEBUG_LOW, "sent measure request\r\n");

        // The average measurement cycle takes 36.65ms. 100ms leaves a good amount
        // of leeway
        os_delay_us(50000);
        os_delay_us(50000);
        
        // Retrieve the data now that the measurement cycle has completed
        user_i2c_start_bit();
        if (user_i2c_write_byte((SENSOR_ADDR << 1) | 0x01) == 1) {
                PRINT_DEBUG(DEBUG_ERR, "slave failed to receive address\r\n");
        	user_i2c_stop_bit();
		return;      
        };


        read_byte = user_i2c_read_byte(0);               // Read upper byte
        status = read_byte >> 6;                         // Upper two bits are status
        humidity |= ((read_byte  & 0b00111111) << 8);    // Remainder of byte is upper 6 bits of humidity
        
        read_byte = user_i2c_read_byte(1);               // Read lower byte
        user_i2c_stop_bit();
        humidity |= read_byte;                           // Lower byte is lower 8 bits of humidity

        adj_humidity = ((float)humidity / (float)((1 << 14) - 2)) * 100;        // Calculate RH as defined by Honeywell

        PRINT_DEBUG(DEBUG_HIGH, "reading=%d, humidity=%d, status=%d\r\n", humidity, (uint32)adj_humidity, status);

        // Store humidity.
        sensor_data_ext = adj_humidity;

	TASK_RETURN(SIG_HUMIDITY, PAR_HUMIDITY_READ_DONE);	

        return;
};
