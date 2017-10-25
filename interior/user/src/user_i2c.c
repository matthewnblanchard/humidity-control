// user_i2c.c
// Authors: Christian Auspland & Matthew Blanchard

#include <user_i2c.h>



void ICACHE_FLASH_ATTR user_i2c_init(void) 
{
        // Disable GPIO interrupts during initialization
        ETS_GPIO_INTR_DISABLE();

        // Set pin functions to GPIO 
        PIN_FUNC_SELECT(SDA_MUX, SDA_FUNC);
        PIN_FUNC_SELECT(SCL_MUX, SCL_FUNC);

        // Enable internal pullups on SDA/SCL
        // PIN_PULLUP_EN(SDA_MUX);
        // PIN_PULLUP_EN(SCL_MUX);

        // Set to open-drain
        GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(SDA_MUX)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(SDA_MUX))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));
        GPIO_REG_WRITE(GPIO_PIN_ADDR(GPIO_ID_PIN(SCL_MUX)), GPIO_REG_READ(GPIO_PIN_ADDR(GPIO_ID_PIN(SCL_MUX))) | GPIO_PIN_PAD_DRIVER_SET(GPIO_PAD_DRIVER_ENABLE));

        // Enable pins
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | SDA_BIT);
        GPIO_REG_WRITE(GPIO_ENABLE_ADDRESS, GPIO_REG_READ(GPIO_ENABLE_ADDRESS) | SCL_BIT);

        // GPIO initialization
        gpio_init();

        // Re-enable GPIO interrupts
        ETS_GPIO_INTR_ENABLE();

        // Both pins begin high
        user_i2c_sda_set(1);
        user_i2c_scl_set(1);

        return;
};

static void user_i2c_sda_set(bool state)
{
        // To drive the line low, pull the pin low (low output)
        // To "drive" the line high, let it float at Hi-Z (input)
        if (state == 0) {
                gpio_output_set(0, SDA_BIT, SDA_BIT, 0);
        } else {
                gpio_output_set(SDA_BIT, 0, SDA_BIT, 0);
        }

        return;
};

static void user_i2c_scl_set(bool state)
{
        // To drive the line low, pull the pin low (low output)
        // To "drive" the line high, let it float at Hi-Z (input)
        if (state == 0) {
                gpio_output_set(0, SCL_BIT, SCL_BIT, 0);
        } else {
                gpio_output_set(SCL_BIT, 0, SCL_BIT, 0);
        }

        return;
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

        return;
};

void ICACHE_FLASH_ATTR user_i2c_stop_bit(void)
{
        // An I2C stop bit starts with SDA low and SCL HIGH. It then
        // pulls SDA high while keeping SCL high
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);
        user_i2c_sda_set(1);
        os_delay_us(I2C_DELAY);

        return;
};

static bool user_i2c_scl_read(void)
{
        // Read and return SCL line
        return GPIO_INPUT_GET(GPIO_ID_PIN(SCL_PIN));
};

static bool user_i2c_sda_read(void)
{
        // Read and return SDA line
        return GPIO_INPUT_GET(GPIO_ID_PIN(SDA_PIN));
};

static uint8 user_i2c_read_bit(void)
{
        uint8 bit = 0;

        // To read a bit, SDA is first driven high. After a delay,
        // SCL is brought high. Data is read from the slave, then
        // the SCL line is brought low       
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);


        bit = user_i2c_sda_read();
        os_delay_us(I2C_DELAY);

        return bit;
};

static void user_i2c_write_bit(uint8 bit)
{
        // To write a bit, pull SCL low, write the desired bit to SDA,
        // then float SCL high delay and pull it back low
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
       
        os_delay_us(I2C_DELAY);

        // Write each bit from most significant to least
        for (i = 0; i < 8; i++) {
                user_i2c_write_bit((byte << i) & 0x80);
        }

        // Read/return ACK/NACK
        return user_i2c_recv_ack();
};

uint8 ICACHE_FLASH_ATTR user_i2c_read_byte(uint8 ack)
{
        int8 i = 0;    // Loop index
        uint8 byte = 0; // Read byte
        
        user_i2c_sda_set(1);

        // Read each bit from most significant to least
        for (i = 7; i >=0; i--) {
                byte |= user_i2c_read_bit() << i;
        }

        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);

        // Send an ACK or NACK
        user_i2c_send_ack(ack);

        return byte;
};

void ICACHE_FLASH_ATTR user_i2c_send_ack(uint8 ack_level)
{
        // To send an ACK, bring SDA low and pulse the clock.
        // To send a NACK, bring SDA high and pulse the clock

        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);

        user_i2c_sda_set(ack_level);

        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);

        user_i2c_sda_set(1);
        os_delay_us(I2C_DELAY);

        return;
};

uint8 ICACHE_FLASH_ATTR user_i2c_recv_ack(void)
{
        // To check for an ACK, bring SDA high, pulse the clock,
        // then check to see if the slave has brought SDA low

        uint8 result = 0;

        user_i2c_sda_set(1);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(1);
        os_delay_us(I2C_DELAY);

        result = user_i2c_sda_read();

        os_delay_us(I2C_DELAY);
        user_i2c_scl_set(0);
        os_delay_us(I2C_DELAY);
        user_i2c_sda_set(0);
        os_delay_us(I2C_DELAY);

        return result;
}
