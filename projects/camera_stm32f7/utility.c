#include "utility.h"
#include "sys_init.h" //Needed because of g_clock_mhz

// Storage for our monotonic system clock.
// Note that it needs to be volatile since we're modifying it from an interrupt.
static volatile uint64_t _millis = 0;


/*!
 * @brief prepares i2c peripheral for transfer of data 
 *
 * @param[in] i2c           I2Cx peripheral that we will use
 * @param[in] addr          Slave address that we will be communicating with
 * @param[in] dir           If zero then we are reading, 
 *                          if one then we are writing
 * @param[in] num_bytes     That we will write/read
 *
 * @note                    This is here because of refactoring
 */
void i2c_prepare(uint32_t i2c, uint8_t addr, uint8_t dir, uint8_t num_bytes)
{
    i2c_set_7bit_address(i2c, addr);
    
    if(dir)
    {
        //writing 
        i2c_set_write_transfer_dir(i2c);
    }
    else
    {
        //reading
        i2c_set_read_transfer_dir(i2c);
    }

    i2c_set_bytes_to_transfer(i2c, num_bytes);

    if(dir)
    {
        //writing 
        i2c_enable_autoend(i2c);
        i2c_send_start(i2c);
    }
    else
    {
        //reading
        i2c_send_start(i2c);
        // important to do it afterwards to do a proper repeated start!
        i2c_enable_autoend(i2c);
    }
}

/*!
 * @brief Writes one byte of data to specified address over i2c
 *
 * @param[in] addr  Address that we will write to
 * @param[in] data  One byte that we will write
 *
 * @return          if everything ok return true, otherwise false
 */
bool i2c_write(uint8_t addr, uint8_t data)
{
    i2c_prepare(I2C1, addr, I2C_WRITE_DIR, 1);

    bool wait = true;
    while (wait)
    {
        if (i2c_transmit_int_status(I2C1)) 
        {
            wait = false;
        }
        //blocks until ack is received, or it timeouts
        if(!wait_for_ack(I2C_TIMEOUT))
        {
            return false; 
        }
    }
    i2c_send_data(I2C1, data);
    return true;
}

/*!
 * @brief Writes an array of bytes of data to specified address over i2c
 *
 * @param[in] addr          Address that we will write to
 * @param[in] data          Reference to array of data that we will write
 * @param[in] num_bytes     Number of bytes that we will write 
 *
 * @return true, if everything ok, otherwise false
 */
bool i2c_write_array(uint8_t addr, uint8_t * data, uint8_t num_bytes)
{
    i2c_prepare(I2C1, addr, I2C_WRITE_DIR, num_bytes);

    while (num_bytes--) 
    {
        bool wait = true;
        while (wait) 
        {
            if (i2c_transmit_int_status(I2C1)) 
            {
                wait = false;
            }
            //blocks until ack is received, or it timeouts
            if(!wait_for_ack(I2C_TIMEOUT))
            {
                return false; 
            }
        }
        i2c_send_data(I2C1, *data++);
    }
    return true;
}

/*!
 * @brief Writes one word of data to specified address over i2c
 *
 * @param[in] addr  Address that we will write to
 * @param[in] data  One word that we will write
 *
 * @return true, if everything ok, otherwise false
 */
bool i2c_write16(uint8_t addr, uint16_t data)
{
    uint8_t separated_bytes[2] = {(uint8_t) (data >> 8) & 0xFF, 
                                 (uint8_t) data & 0xFF};

    i2c_prepare(I2C1, addr, I2C_WRITE_DIR, 2);

    for(int i = 0; i < 2; i++)
    {
        bool wait = true;
        while (wait) 
        {
            //printf("We loop here\n");
            if (i2c_transmit_int_status(I2C1)) 
            {
                wait = false;
            }
            //blocks until ack is received, or it timeouts
            if(!wait_for_ack(I2C_TIMEOUT))
            {
                return false; 
            }
        }
        i2c_send_data(I2C1, separated_bytes[i]);
    }
    return true;
}

/*!
 * @brief Writes an array of bytes of data to specified address over i2c
 *
 * @param[in] addr          Address that we will write to
 * @param[in] data          Reference to array of data that we will write
 * @param[in] num_words     Number of words that we will write 
 *
 * @return true, if everything ok, otherwise false
 */
bool i2c_write16_array(uint8_t addr, uint16_t * data, uint8_t num_words)
{
    i2c_prepare(I2C1, addr, I2C_WRITE_DIR, 2 * num_words);

    bool high_byte = true;
    uint16_t num_bytes = 2 * num_words;

    while (num_bytes--) 
    {
        bool wait = true;
        while (wait) 
        {
            if (i2c_transmit_int_status(I2C1)) 
            {
                wait = false;
            }
            //blocks until ack is received, or it timeouts
            if(!wait_for_ack(I2C_TIMEOUT))
            {
                return false; 
            }
        }
        if (high_byte)
        {
            //time to send high byte of the data element
            uint8_t byte = (uint8_t) (*data >> 8) & 0xFF;
            //send it
            i2c_send_data(I2C1, byte);
            high_byte = false;
        }
        else
        {
            //time to send low byte of the data element
            uint8_t byte = (uint8_t) (*data & 0xFF);
            i2c_send_data(I2C1, byte);
            data++;                     // Increment pointer to next element
            high_byte = true;
        }
    }
    return true;
}

/*!
 * @brief Reads one byte over i2c
 *
 * @param[in] addr  Address that we will read from
 * @param[in] data  Copy by reference, where we will copy read byte
 *
 * @return true, if everything ok, otherwise false
 */
bool i2c_read(uint8_t addr, uint8_t * data)
{
    i2c_prepare(I2C1, addr, I2C_READ_DIR, 1);

    if(!wait_for_empty_data_reg(I2C_TIMEOUT))
    {
        return false; 
    }
    
    *data = i2c_get_data(I2C1);

    return true; 
}

/*!
 * @brief Reads one word over i2c
 *
 * @param[in] addr  Address that we will read from
 * @param[in] data  Copy by reference, where we will copy read word
 *
 * @return true, if everything ok, otherwise false
 */
bool i2c_read16(uint8_t addr, uint16_t * data)
{
    uint16_t separated_bytes[2] = {0,0};

    i2c_prepare(I2C1, addr, I2C_READ_DIR, 2);

    for (uint8_t i = 0; i < 2; i++) 
    {
        if(!wait_for_empty_data_reg(I2C_TIMEOUT))
        {
            return false; 
        }
        separated_bytes[i] = i2c_get_data(I2C1);
    }
    
    //printf("high separated byte is %d\n", separated_bytes[0]);
    //printf("low separated byte is %d\n", separated_bytes[1]);

    //Join bytes together
    *data =(uint16_t)(separated_bytes[0] << 8) | separated_bytes[1];

    return true; 
}

/*!
 * @brief Reads number of bytes over i2c
 *
 * @param[in] addr          Address that we will read from
 * @param[in] data          Reference to array of data that we will receive
 * @param[in] num_bytes     Number of bytes that we expect
 */
bool i2c_read_array(uint8_t addr, uint8_t * data, uint8_t num_bytes)
{
    i2c_prepare(I2C1, addr, I2C_READ_DIR, num_bytes);

    for (uint8_t i = 0; i < num_bytes; i++) 
    {
        if(!wait_for_empty_data_reg(I2C_TIMEOUT))
        {
            return false; 
        }
        data[i] = i2c_get_data(I2C1);
    }
    return true; 
}

/*!
 * @brief Reads number of words over i2c
 *
 * @param[in] addr          Address that we will read from
 * @param[in] data          Reference to array of data that we will receive
 * @param[in] num_words     Number of words that we expect
 */
bool i2c_read16_array(uint8_t addr, uint16_t * data, uint8_t num_words)
{
    uint16_t high = 0;
    uint16_t low = 0;

    i2c_prepare(I2C1, addr, I2C_READ_DIR, 2 * num_words);

    bool high_byte = true;

    for (uint8_t i = 0; i < (2 * num_words); i++) 
    {
        if(!wait_for_empty_data_reg(I2C_TIMEOUT))
        {
            return false; 
        }

        if (high_byte)
        {
            //time to read high byte of the data element
            high = i2c_get_data(I2C1);
            high_byte = false;
        }
        else
        {
            //time to send low byte of the data element
            low = i2c_get_data(I2C1);
            *data++ = (uint16_t) ((high << 8) | low);
            high_byte = true;
        }
    }
    return true; 
}

/*!
 * @brief Waits for ACK bit until timeout
 *
 * @param[in]   timeout in milliseconds
 *
 * @return True if ack was received, false if timeout was reached
 */
bool wait_for_ack(uint32_t timeout)
{
    const uint64_t until = millis() + timeout;

    while (i2c_nack(I2C1))
    {
        if (millis() > until)
        {
            return false;
        }
    }
    return true;
}

/*!
 * @brief Waits for empty data reg until timeout
 *
 * @param[in]   timeout in milliseconds
 *
 * @return True if reg was cleaned, false if timeout was reached
 */
bool wait_for_empty_data_reg(uint32_t timeout)
{
    const uint64_t until = millis() + timeout;

    while (i2c_received_data(I2C1) == 0)
    {
        if (millis() > until)
        {
            return false;
        }
    }

    return true;

}

/*!
 * @brief Returns how long microcontroller has been running in microseconds
 *
 * @return time alive in milliseconds
 * @note This value is incremented in sys_tick_handler
 */
uint64_t millis()
{
    return _millis;
}

/*!
 * @brief Returns how long microcontroller has been running in microseconds
 *
 * @return time alive in microseconds
 *
 * @note    Explanation on implementation, we first get ms and turn them in us,
 *          then we get number of cycles left in systick timer, and turn that 
 *          into us. Last part might not make sense, but only because it was 
 *          simplified to avoid unnecessary math operations. 
 *
 *          Original equation was:
 *          number_of_cycles_in_ms = rcc_ahb_frequency/1000;
 *          cycles_in_systick = systick_get_value();
 *          time_in_us = (number_of_cycles_in_ms - cycles_in_systick)/(rcc_ahb_frequency/1000000);
 *          return (millis() * 1000) + time_in_us;
 */
uint64_t micros()
{
    return (millis() * 1000) + (1000 - (systick_get_value() / g_clock_mhz));
}

/*!
 * @brief This is our interrupt handler for the systick reload interrupt.
 *
 * @param[in] duration      in milliseconds
 *
 * @note    The full list of interrupt services routines that can be 
 *          implemented is listed in 
 *          libopencm3/include/libopencm3/stm32/f0/nvic.h
 */
void sys_tick_handler(void)
{
    // Increment our monotonic clock
    _millis++;
}

/*!
 * @brief Delay for a real number of milliseconds
 *
 * @param[in] duration      in milliseconds
 *
 * @note Blocks for specified duration
 */
void delay(uint64_t duration)
{
    const uint64_t until = millis() + duration;
    while (millis() < until);
}

/*!
 * @brief Delay for a real number of microseconds
 *
 * @param[in] duration      in microseconds
 *
 * @note Blocks for specified duration
 */
void delay_us(uint64_t duration)
{
    const uint64_t until = micros() + duration;
    while (micros() < until);
}
