/*
 * This file is part of the XXX distribution (https://github.com/xxxx or http://xxx.github.io).
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

#include "drivers/i2cbus.h"

#include "drivers/clogger.h"
#include <cstring>
#include <stdexcept>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

/**
 * @brief Initialize an i2c device
 *
 * @param[in] i2c_bus The linux device name for the i2c bus including file path
 */
I2CBus::I2CBus(std::string i2c_bus) {
    // Set the device name
    this->i2c_bus = i2c_bus;

    // Try to open the device
    fd = open(i2c_bus.c_str(), O_RDWR | O_NONBLOCK, 0);
    if(fd < 0) {
        throw std::runtime_error("Could not open " + i2c_bus + " (" + strerror(errno) + ")");
    }

    CLOGGER_INFO("Opened " << i2c_bus);
}

/**
 * @brief Close the I2C bus
 *
 * Close the file pointer to the i2c bus
 */
I2CBus::~I2CBus(void) {
    assert(fd > 0);
    close(fd);

    CLOGGER_INFO("Closed " << i2c_bus);
}

/**
 * @brief Set the target i2c address
 *
 * @param[in] address The 7-bit address (or 10-bit address)
 */
void I2CBus::setAddress(uint16_t address) {
    assert(fd >= 0);

    // Set the address
    if (ioctl(fd, I2C_SLAVE, (address >> 1)) < 0) {
        throw std::runtime_error("Could not set slave address of " + i2c_bus + " with slave id " + std::to_string(address) + " (" + strerror(errno) + ")");
    }

    current_address = address;
    //SPDLOG_DEBUG("Set i2c address to " + std::to_string(address));
}

/**
 * @brief Get the currently configured target address
 *
 * @return The current configured target i2c address
 */
uint16_t I2CBus::getAddress(void) {
    return current_address;
}

/**
 * @brief Transmit a single byte to the current configured i2c address
 *
 * @param[in] byte The byte to transmit
 * @return If the bytes is send successfully
 */
bool I2CBus::transmit(uint8_t byte) {
    assert(fd >= 0);

    // Write a single byte to the device
    if(write(fd, &byte, 1) != 1) {
        throw std::runtime_error("Could not transmit byte to " + i2c_bus + " (" + strerror(errno) + ")");
        return false;
    }

    return true;
}

/**
 * @brief Transmit a single byte to a specified i2c address
 *
 * @param[in] address The target i2c address
 * @param[in] byte The byte to transmit
 * @return If the byte is send successfully
 */
bool I2CBus::transmit(uint16_t address, uint8_t byte) {
    // Set the target address
    this->setAddress(address);

    // transmit the byte
    return this->transmit(byte);
}

/**
 * @brief Transmit multiple bytes to the current configured i2c address
 *
 * @param[in] bytes The bytes to transmit
 * @param[in] length The amount of bytes to transmit
 * @return If the bytes are send successfully
 */
bool I2CBus::transmit(uint8_t *bytes, uint32_t length) {
    assert(fd >= 0);

    // Write multiple bytes to a device
    if(write(fd, &bytes, length) != (int32_t)length) {
        throw std::runtime_error(std::string("Could not transmit multiple bytes to (") + strerror(errno) + ")");
        return false;
    }

    return true;
}

/**
 * @brief Transmit multiple bytes to a specified i2c address
 *
 * @param[in] address The target i2c address
 * @param[in] bytes The bytes to transmit
 * @param[in] length The amount of bytes to transmit
 * @return If the bytes are send successfully
 */
bool I2CBus::transmit(uint16_t address, uint8_t *bytes, uint32_t length) {
    // Set the target address
    this->setAddress(address);

    // Transmit the bytes
    return this->transmit(bytes, length);
}

/**
 * @brief Receive a single byte from the current configured i2c address
 *
 * This function is non-blocking and will return true if a byte is read or not.
 * @param[out] byte The pointer to the received byte
 * @return If there was a byte received or not
 */
bool I2CBus::receive(uint8_t *byte) {
    assert(fd >= 0);

    // Read a single byte from the device
    ssize_t bytes_read = read(fd, byte, 1);

    // Check if error occured
    if(bytes_read < 0 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
        throw std::runtime_error("Could not read a single byte from " + i2c_bus + " (" + strerror(errno) + ")");
    }
    // Check if no bytes are read
    else if(bytes_read < 1) {
        return false;
    }

    return true;
}

/**
 * @brief Receive a single byte from a specified i2c address
 *
 * This function is non-blocking and will return true if a byte is read or not.
 * @param[in] address The target i2c address
 * @param[out] byte The pointer to the received byte
 * @return If there was a byte received or not
 */
bool I2CBus::receive(uint16_t address, uint8_t *byte) {
    // Set the target address
    this->setAddress(address);

    // Receive the byte
    return this->receive(byte);
}

/**
 * @brief Transmit multiple bytes to the current configured i2c address
 *
 * This function is non-blocking and the input length is the maximum amount of bytes which
 * will be read from the device. It will return true if any bytes are read and the amount of
 * bytes which are read are returned in the length.
 * @param[out] bytes The pointer to the received bytes
 * @param[in,out] length The amount of bytes to receive maximum and returns the amount of bytes read
 * @return If there are any bytes received successfully
 */
bool I2CBus::receive(uint8_t *bytes, uint32_t *length) {
    assert(fd >= 0);

    // Read multiple bytes from a device
    ssize_t bytes_read = read(fd, bytes, *length);

    // Check if error occured
    if(bytes_read < 0 && (errno != EAGAIN && errno != EWOULDBLOCK)) {
        throw std::runtime_error("Could not read a single byte from " + i2c_bus + " (" + strerror(errno) + ")");
    }
    // Check if no bytes are read
    else if(bytes_read < 1) {
        return false;
    }

    return true;
}

/**
 * @brief Receive multiple bytes from a specified i2c address
 *
 * This function is non-blocking and the input length is the maximum amount of bytes which
 * will be read from the device. It will return true if any bytes are read and the amount of
 * bytes which are read are returned in the length.
 * @param[in] address The target i2c address to read from
 * @param[in,out] bytes The amount of bytes to receive maximum and returns the amount of bytes read
 * @param[in] length The amount of bytes to receive
 * @return If the bytes are received successfully
 */
bool I2CBus::receive(uint16_t address, uint8_t *bytes, uint32_t *length) {
    // Set the target address
    this->setAddress(address);

    // Receive the bytes
    return this->receive(bytes, length);
}

/**
 * @brief Transmit and receive multiple bytes
 *
 * This function is blocking and two different lengths for writing and receiving can
 * be given. Both the input and ouput are read/written from the same buffer.
 * @param[in,out] bytes The output and input bytes to write/receive
 * @param[in] write_length The amount of bytes to write
 * @param[in] receive_length The amount of bytes to receive
 * @return If the bytes are send and received
 */
bool I2CBus::transceive(uint8_t *bytes, uint32_t write_length, uint32_t receive_length) {
    struct i2c_msg trx_msgs[2];
    struct i2c_rdwr_ioctl_data trx_data;
    trx_data.msgs = trx_msgs;
    trx_data.nmsgs = 2;

    // Setup the write message
    trx_msgs[0].addr = current_address >> 1;
    trx_msgs[0].flags = 0;
    trx_msgs[0].len = write_length;
    trx_msgs[0].buf = bytes;

    // Setup the receive message
    trx_msgs[1].addr = current_address >> 1;
    trx_msgs[1].flags = I2C_M_RD;
    trx_msgs[1].len = receive_length;
    trx_msgs[1].buf = bytes;

    // Execute the transceive
    if (ioctl(fd, I2C_RDWR, &trx_data) < 0) {
        throw std::runtime_error("Could not do a transceive at " + i2c_bus + " I2C_RDWR (" + strerror(errno) + ")");
    }

    return true;
}

/**
 * @brief Transmit and receive multiple bytes to a specific address
 *
 * This function is blocking and two different lengths for writing and receiving can
 * be given. Both the input and ouput are read/written from the same buffer.
 * @param[in] address The target i2c address
 * @param[in,out] bytes The output and input bytes to write/receive
 * @param[in] write_length The amount of bytes to write
 * @param[in] receive_length The amount of bytes to receive
 * @return If the bytes are send and received
 */
bool I2CBus::transceive(uint16_t address, uint8_t *bytes, uint32_t write_length, uint32_t receive_length) {
    // Set the target address
    this->setAddress(address);

    // Receive the byte
    return this->transceive(bytes, write_length, receive_length);
}
