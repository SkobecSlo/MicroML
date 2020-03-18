#include "flir.h"
#include "utility.h"

static uint8_t last_flir_error = LEP_OK;

/*!
 * @brief Sends get command to FLIR module
 *
 * @param[in] cmd_code      Command code to be send, use the ones 
 *                          defined in flir_defines.h
 * @param[in] data_words    Copy by reference, result will be copied into it
 * @param[in] num_words     Num of words that will be read
 *
 * @return  Returns true if everything is ok, otherwise false
 *
 * @note    Procedure implemented as written in Lepton Software Interface 
 *          Description Document rev200.pdf, page 11
 */
bool get_flir_command(uint16_t cmd_code, uint16_t * data_words, uint8_t num_words)
{
    // Read command status register
    // If not BUSY, write number of data words to read into DATA length reg.
    if(wait_busy_bit(FLIR_BUSY_TIMEOUT))
    {
        // write command ID to command reg
        // Read command status register
        if (write_register(LEP_I2C_COMMAND_REG, cmd_code)) 
        {
            // Successful wait for Flir to process this
            if(wait_busy_bit(FLIR_BUSY_TIMEOUT))
            {
                // If no timeout, read DATA from DATA regs
                if(read_data_register(data_words, num_words))
                {
                    return true; 
                }
            }
        }
    }
    // Something failed
    return false;
}

/*!
 * @brief Sends set command to FLIR module
 *
 * @param[in] cmd_code      Command code to be send, use the ones 
 *                          defined in flir_defines.h
 * @param[in] data_words    Copy by reference, result will be copied into it
 * @param[in] num_words     Num of words that will be read
 *
 * @return  Returns true if everything is ok, otherwise false
 *
 * @note    Procedure implemented as written in Lepton Software Interface 
 *          Description Document rev200.pdf, page 12
 */
bool set_flir_command(uint16_t cmd_code, uint16_t * data_words, uint8_t num_words)
{
    // Read command status register
    // If not BUSY, write number of data words to read into DATA length reg.
    if (wait_busy_bit(FLIR_BUSY_TIMEOUT))
    {
        // write command ID to command reg
        // Read command status register
        if (write_command_register(cmd_code, data_words, num_words) == 0) 
        {
            // Successful wait for Flir to process this
            if (wait_busy_bit(FLIR_BUSY_TIMEOUT))
            {
                return true; 
            }
        }
    }

    // Something failed
    return false;
}

/*!
 * @brief Function will convert command ID and type into one cmd_code,
 *        which will be fed into get_flir_command() or set_flir_command()
 *
 * @param[in] cmd_id        Like LEP_CID_VID_POLARITY_SELECT
 * @param[in] cmd_type      Like LEP_I2C_COMMAND_TYPE_GET
 *
 * @return cmd_code
 *
 * @note 
 */
uint16_t command_code(uint16_t cmd_id, uint16_t cmd_type) 
{
    return (cmd_id & LEP_I2C_COMMAND_MODULE_ID_BIT_MASK) | 
           (cmd_id & LEP_I2C_COMMAND_ID_BIT_MASK) | 
           (cmd_type & LEP_I2C_COMMAND_TYPE_BIT_MASK);
}

/*!
 * @brief Blocking function, will wait until FLIR camera is ready to
 * receive commands
 *
 * @param[in] timeout How long will function pool for BUSY bit in milliseconds
 *
 * @return  Returns true if BUSY bit was cleared or false if timeout was reached
 */
bool wait_busy_bit(uint16_t timeout)
{
    uint16_t status;

    if (!read_register(LEP_I2C_STATUS_REG, &status))
    {
        // Something went wrong
        return false;
    }

    if (!(status & LEP_I2C_STATUS_BUSY_BIT_MASK))
    {
        // Busy bit was cleared
        last_flir_error = (uint8_t)((status & LEP_I2C_STATUS_ERROR_CODE_BIT_MASK) >> LEP_I2C_STATUS_ERROR_CODE_BIT_SHIFT);
        return true;
    }

    // Get current time and add to it timeout value
    uint32_t end_time = flir_millis() + (uint32_t) timeout;

    // Loop as long busy bit is set and timeout isn't reached
    while ((status & LEP_I2C_STATUS_BUSY_BIT_MASK) && flir_millis() < end_time)
    {
        flir_delay(1);

        if(!read_register(LEP_I2C_STATUS_REG, &status))
        {
            // Something went wrong
            return false;
        }
    }

    if (!(status & LEP_I2C_STATUS_BUSY_BIT_MASK))
    {
        // Busy bit was cleared
        last_flir_error = (uint8_t)((status & LEP_I2C_STATUS_ERROR_CODE_BIT_MASK) >> LEP_I2C_STATUS_ERROR_CODE_BIT_SHIFT);
        return true;
    }
    else
    {
        // Busy bit was still set after timeout
        printf("Fail 3\n");
        return false;
    }
}

/*!
 * @brief Returns static variable that will show last error code given from FLIR
 *
 * @return error code
 *
 * @note Check LEP_RESULT enum for possible values
 */
LEP_RESULT get_last_flir_result()
{
    return (LEP_RESULT) last_flir_error;
}

//I2C commands, i2c peripheral should be initialized before using flir.h

/*!
 * @brief Writes two bytes into FLIR register
 *
 * @param[in] reg_address   Register address that will be written
 * @param[in] value         Value that will be written in register  
 *
 * @return true, if everything ok, otherwise false
 */
static bool write_register(uint16_t reg_address, uint16_t value)
{
    uint16_t temp_buf[2] = {reg_address, value};
    if (!i2c_write16_array(LEP_I2C_DEVICE_ADDRESS, temp_buf, 2))
    {
        return false;
    }

    return true;
}

/*!
 * @brief Read FLIR register
 *
 * @param[in] reg_address  
 * @param[in] value         Passed by reference, will hold read value after
 *                          function call
 *
 * @return true, if everything ok, otherwise false
 */
static bool read_register(uint16_t reg_address, uint16_t * value)
{
    if (!i2c_write16(LEP_I2C_DEVICE_ADDRESS, reg_address))
    {
        return false;
    }

    return i2c_read16(LEP_I2C_DEVICE_ADDRESS, value);
}

/*!
 * @brief Functions writes into command register of FLIR camera
 *
 * @param[in] cmd_code      Command code that will be written
 * @param[in] data_words      
 * @param[in] num_words
 *
 * @return 0:   success
 *         1:   data too long to fit in transmit buffer
 *         2:   received NACK on transmit of address
 *         3:   received NACK on transmit of data
 *         4:   other error 
 */
//static bool write_command_register(uint16_t cmd_code, uint16_t * data_words, uint16_t num_words)
//{
//    if (data_words && num_words)
//    {
//        i2c_begin_transmission(LEP_I2C_DEVICE_ADDRESS);
//        i2c_write16(LEP_I2C_DATA_LENGTH_REG);
//        i2c_write16(num_words);
//
//        uint8_t status = i2c_end_transmission();
//
//        if (status)
//        {
//            return status;
//        }
//
//        uint16_t reg_address = num_words <= 16 ? LEP_I2C_DATA_0_REG : LEP_I2C_DATA_BUFFER;
//
//        i2c_begin_transmission(LEP_I2C_DEVICE_ADDRESS);
//        i2c_write16(reg_address);
//
//        while (num_words-- > 0)
//        {
//            i2c_write16(*data_words++);
//        }
//
//        status = i2c_end_transmission();
//
//        if (status)
//        {
//            return status;
//        }
//    }
//
//    i2c_begin_transmission(LEP_I2C_DEVICE_ADDRESS);
//    i2c_write16(LEP_I2C_COMMAND_REG);
//    i2c_write16(cmd_code);
//    return i2c_end_transmission();
//}

/*!
 * @brief Functions reads DATA register of FLIR camera
 *
 * @param[in] read_words  
 * @param[in] max_length    That can be read in words  
 *
 * @return true, if everything ok, otherwise false
 */
static bool read_data_register(uint16_t * read_words, uint8_t max_length)
{
    // Point to data length reg
    if(!i2c_write16(LEP_I2C_DEVICE_ADDRESS, LEP_I2C_DATA_LENGTH_REG))
    {
        // Something went wrong
        return false;
    }

    // Get data length
    // Now something weird is happening here. Here we are going to read DATA 
    // length registers, which should tell us how many WORDS of data are we 
    // expecting (according to datasheet) but is actually returning number 
    // of BYTES that we will have to read. For example when getting Customer
    // Serial number we are expecting DATA length to throw us number 16 as 
    // per datasheet, BUT we get 32. So it seems that we will be using number 
    // of bytes instead.  
    uint16_t num_bytes = 0;
    if (!i2c_read16(LEP_I2C_DEVICE_ADDRESS, &num_bytes))
    {
        // Something went wrong
        return false;
    }
    
    // Error handling
    if (0 == num_bytes)
    {
        return false;
    }
    
    // Error handling
    if ((max_length * 2) != num_bytes)
    {
        return false;
    }


    if (!i2c_read16_array(LEP_I2C_DEVICE_ADDRESS, read_words, max_length))
    {
        return false;
    }

    return true;
}

// Utility functions, hardware depended

/*!
 * @brief Function will delay processor for specified time
 *
 * @param[in] delay In milliseconds  
 *
 * @note Hardware depended implementation
 */
static void flir_delay(uint32_t delay_in_ms)
{
    delay(delay_in_ms);
}

/*!
 * @brief Get value how long is processor running in milliseconds
 *
 * @return time in milliseconds
 *
 * @note Hardware depended implementation
 */
static uint32_t flir_millis()
{
    return millis();
}