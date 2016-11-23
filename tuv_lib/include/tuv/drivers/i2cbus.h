/*
 * This file is part of the TUV library (https://github.com/tudelft/tudelft_vision).
 * Copyright (c) 2016 Freek van Tienen <freek.v.tienen@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DRIVERS_I2CBUS_H_
#define DRIVERS_I2CBUS_H_

#include <string>

/**
 * A linux based i2c driver
 * This class can transmit, receive and transceive on a linux i2c bus. All receive functions
 * are non-blocking and the last i2c target address is remembered for the next transmit, receive
 * or transceive action.
 */
class I2CBus {
  private:
    std::string i2c_bus;  ///< The device name including file path
    int fd;										///< The file pointer to the i2c device
    uint16_t current_address;	///< The current i2c address

  public:
    I2CBus(std::string i2c_bus);
    ~I2CBus(void);

    /* Operational functions */
    void setAddress(uint16_t address);			///< Set the target i2c address
    uint16_t getAddress(void);							///< get the target i2c address

    /* Transmit functions */
    bool transmit(uint8_t byte);										  ///< Transmit a byte
    bool transmit(uint16_t address, uint8_t byte);		///< Transmit a byte to a specific address
    bool transmit(uint8_t *bytes, uint32_t length);	  ///< Transmit multiple bytes
    bool transmit(uint16_t address, uint8_t *bytes, uint32_t length);  ///< Transmit multiple bytes to a specific address

    /* Receive functions */
    bool receive(uint8_t *byte);										  ///< Receive a single byte
    bool receive(uint16_t address, uint8_t *byte);		///< Receive a single byte from a specific address
    bool receive(uint8_t *bytes, uint32_t *length);		///< Receive multiple bytes
    bool receive(uint16_t address, uint8_t *bytes, uint32_t *length); ///< Receive multiple bytes from a specific address

    /* Transceive functions */
    bool transceive(uint8_t *bytes, uint32_t write_length, uint32_t receive_length); ///< Transceive multiple bytes
    bool transceive(uint16_t address, uint8_t *bytes, uint32_t write_length, uint32_t receive_length); ///< Transceive multiple bytes to a speicifc address
};

#endif /* DRIVERS_I2CBUS_H_ */
