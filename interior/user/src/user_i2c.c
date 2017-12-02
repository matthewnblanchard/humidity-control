// user_i2c.c
// Authors: Christian Auspland & Matthew Blanchard

#include <user_i2c.h>

// Static function prototypes
static inline void ICACHE_FLASH_ATTR user_i2c_sda_set(bool state);
static inline void ICACHE_FLASH_ATTR user_i2c_scl_set(bool state);
static inline bool ICACHE_FLASH_ATTR user_i2c_scl_read(void);
static inline bool ICACHE_FLASH_ATTR user_i2c_sda_read(void);
static inline uint8 ICACHE_FLASH_ATTR user_i2c_read_bit(void);
static inline void ICACHE_FLASH_ATTR user_i2c_write_bit(uint8 bit);
static inline uint8 ICACHE_FLASH_ATTR user_i2c_recv_ack(void);
static inline void ICACHE_FLASH_ATTR user_i2c_send_ack(uint8 ack_level);

static inline void ICACHE_FLASH_ATTR user_i2c_sda_set(bool state)
{
        // To drive the line low, pull the pin low (low output)
        // To "drive" the line high, let it float at Hi-Z (input)
        if (state == 0) {
                gpio_output_set(0, SDA_BIT, SDA_BIT, 0);	// Output '0' on SDA pin 
        } else {
                gpio_output_set(SDA_BIT, 0, SDA_BIT, 0);	// Set SDA pin as input
        }

        return;
};

static inline void ICACHE_FLASH_ATTR user_i2c_scl_set(bool state)
{
        // To drive the line low, pull the pin low (low output)
        // To "drive" the line high, let it float at Hi-Z (input)
        if (state == 0) {
                gpio_output_set(0, SCL_BIT, SCL_BIT, 0);	// Output '0' on SCL pin
        } else {
                gpio_output_set(SCL_BIT, 0, SCL_BIT, 0);	// Set SCL pin as input
        }

        return;
};

static inline bool ICACHE_FLASH_ATTR user_i2c_scl_read(void)
{
        // Read and return SCL line
        return GPIO_INPUT_GET(GPIO_ID_PIN(SCL_PIN));
};

static inline bool ICACHE_FLASH_ATTR user_i2c_sda_read(void)
{
        // Read and return SDA line
        return GPIO_INPUT_GET(GPIO_ID_PIN(SDA_PIN));
};

void ICACHE_FLASH_ATTR user_i2c_start_bit(void)
{
        // An I2C start bit begins with both SDA & SCL high. It then
        // pulls SDA low while keeping SCL high
        user_i2c_sda_set(1);
        user_i2c_scl_set(1);

        os_delay_us(I2C_DELAY);
        user_i2c_sda_set(0);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);

        return;
};

void ICACHE_FLASH_ATTR user_i2c_stop_bit(void)
{
        // An I2C stop bit starts with SDA low and SCL LOW. It then
        // pulls SCL high followed by SDA
        user_i2c_scl_set(0);
        user_i2c_sda_set(0);

        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);
        user_i2c_sda_set(1);
        os_delay_us(I2C_DELAY);

        return;
};

static inline uint8 ICACHE_FLASH_ATTR user_i2c_read_bit(void)
{
        uint8 bit = 0;

        // To read a bit, SDA is first left floating (high). SCL is pulsed,
	// and SDA (controlled by the slave) is read during the pulse
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);

        bit = user_i2c_sda_read();
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);

        return bit;
};

static inline void ICACHE_FLASH_ATTR user_i2c_write_bit(uint8 bit)
{
        // To write a bit, set SDA to the desired level (Hi-Z for a '1' or drive low 
	// for a '0', then pulse SCL
        user_i2c_scl_set(0);
        user_i2c_sda_set(bit);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);        
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);        

        return;
};

uint8 ICACHE_FLASH_ATTR user_i2c_write_byte(uint8 byte)
{
        uint8 i = 0;    // Loop index
       
        // Write each bit from most significant to least
        for (i = 0; i < 8; i++) {
                user_i2c_write_bit((byte << i) & 0x80);
        }

        // Read/return ACK/NACK
        return user_i2c_recv_ack();
};

uint8 ICACHE_FLASH_ATTR user_i2c_read_byte(uint8 ack)
{
        int8 i = 0;     // Loop index
        uint8 byte = 0; // Read byte
        
        user_i2c_sda_set(1);	// Let SDA float

        // Read each bit from most significant to least
        for (i = 7; i >=0; i--) {
                byte |= user_i2c_read_bit() << i;
        }

        // Send an ACK or NACK
        user_i2c_send_ack(ack);

        return byte;
};

static inline void ICACHE_FLASH_ATTR user_i2c_send_ack(uint8 ack_level)
{
        // To send an ACK, bring SDA low and pulse the clock.
        // To send a NACK, bring SDA high and pulse the clock

        user_i2c_scl_set(0);
        user_i2c_sda_set(ack_level);

        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);

        return;
};

static inline  uint8 ICACHE_FLASH_ATTR user_i2c_recv_ack(void)
{
        // To check for an ACK, bring SDA high, pulse the clock,
        // then check to see if the slave has brought SDA low

        uint8 result = 0;

        user_i2c_sda_set(1);

        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);

        result = user_i2c_sda_read();

        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);

        return result;
}
