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

#include "drivers/mt9f002.h"

#include "drivers/mt9f002_regs.h"
#include <stdexcept>
#include <assert.h>
#include <math.h>
#include <algorithm>
#include <unistd.h>

/**
 * @brief Initialize the MT9F002 CMOS chip
 *
 * This will initialize the MT9F002 CMOS chip and set the default configuration. This default
 * configuration inclused exposure, fps etc.
 * @param[in] i2c_bus The i2c bus which the MT9F002 CMOS chip is connected to
 * @param[in] interface The type of interface the MT9F002 data lines are connected to
 * @param[in] pll_config The PLL configuration values for the MT9F002 chip
 */
MT9F002::MT9F002(I2CBus *i2c_bus, enum interfaces interface, struct pll_config_t pll_config) {
    // Save the i2c bus, interface and pll config
    this->i2c_bus = i2c_bus;
    this->interface = interface;
    this->pll_config = pll_config;

    // Default values
    target_exposure = 50;
    target_fps = 15;

    // Default configuration
    res_config.offset_x       = 114;
    res_config.offset_y       = 106;
    res_config.output_width   = 1088;
    res_config.output_height  = 720;
    res_config.output_scaler  = 1.0;
    res_config.sensor_width   = 1088;
    res_config.sensor_height  = 720;

    blank_config.min_line_blanking_pck              = 1316;
    blank_config.min_line_length_pck                = 1032;
    blank_config.min_line_fifo_pck                  = 60;
    blank_config.fine_integration_time_min          = 1032;
    blank_config.fine_integration_time_max_margin   = 1316;

    gain_config.red        = 2.0;
    gain_config.green1     = 2.0;
    gain_config.blue       = 2.0;
    gain_config.green2     = 2.0;

    //FIXME: Power reset??

    // Software reset
    writeRegister(MT9F002_SOFTWARE_RESET, 1, 1);
    usleep(500000); // FIXME: Wait for 500ms, non busy-waiting and arch indep
    writeRegister(MT9F002_SOFTWARE_RESET, 0, 1);

    // Based on the interface configure stage 1
    if(interface == MIPI || interface == HiSPi) {
        mipiHispiStage1();
    } else {
        parallelStage1();
    }

    // Write the PLL based on Input clock and wanted clock frequency
    writePll();

    // Based on the interface configure stage 2
    if(interface == MIPI || interface == HiSPi) {
        mipiHispiStage2();
    } else {
        parallelStage2();
    }

    // Write the resolution information
    writeResolution();

    // Write blanking (based on FPS)
    writeBlanking();

    // Write exposure (based on target_exposure)
    writeExposure();

    // Write gains for the different pixel colors
    writeGains();

    // Based on the interface configure stage 3
    if(interface == MIPI || interface == HiSPi) {
        mipiHispiStage3();
    }

    // Turn the stream on
    writeRegister(MT9F002_MODE_SELECT, 0x01, 1);
}

/**
 * @brief Write to a register
 *
 * This will write an integer value to a register of a certain length
 * @param[in] address The address to write to
 * @param[in] value The value to write to the register
 * @param[in] length The length of the value in bytes to write (1, 2 or 4)
 */
void MT9F002::writeRegister(uint16_t address, uint32_t value, uint8_t length) {
    uint8_t bytes[length+2];
    assert(this->i2c_bus != NULL);
    assert(length == 1 || length == 2 || length == 4);

    // Set the output address
    bytes[0] = address >> 8;
    bytes[1] = address & 0xFF;

    // Fix sigdness based on length
    if(length == 1) {
        bytes[2] = value & 0xFF;
    } else if(length == 2) {
        bytes[2] = (value >> 8) & 0xFF;
        bytes[3] = value & 0xFF;
    } else if(length == 4) {
        bytes[2] = (value >> 24) & 0xFF;
        bytes[3] = (value >> 16) & 0xFF;
        bytes[4] = (value >> 8) & 0xFF;
        bytes[5] = value & 0xFF;
    }

    // Transmit the buffer
    this->i2c_bus->transmit(MT9F002_ADDRESS, bytes, length+2);
}

/**
 * @brief Read from a register
 *
 * This will read from a register and return the value as an integer.
 * @param[in] address The address to read fromr
 * @param[in] length The amount of bytes to read
 * @return The register value
 */
uint32_t MT9F002::readRegister(uint16_t address, uint8_t length) {
    uint8_t bytes[length+2];
    assert(this->i2c_bus != NULL);
    assert(length <= 4);

    // Set the address
    bytes[0] = address >> 8;
    bytes[1] = address & 0xFF;

    // Transmit the buffer and receive back
    this->i2c_bus->transceive(MT9F002_ADDRESS, bytes, 2, length);

    /* Fix sigdness */
    uint32_t ret = 0;
    for(uint8_t i =0; i < length; i++) {
        ret |= bytes[length-i-1] << (8*i);
    }
    return ret;
}

/**
 * @brief Configuration stage 1 for both MIPI and HiSPi interface
 *
 * This will do the first step of configuring the MT9F002 for both the
 * MIPI and the HiSPi interface.
 */
void MT9F002::mipiHispiStage1(void) {
    writeRegister(MT9F002_RESET_REGISTER, 0x0118, 2);
    writeRegister(MT9F002_MODE_SELECT, 0x00, 1);

    uint32_t serialFormat;
    if (interface == HiSPi) {
        serialFormat = (3<<8) | 2; // 2 Serial lanes
    } else {
        serialFormat = (2<<8) | 2; // 2 Serial lanes
    }
    writeRegister(MT9F002_SERIAL_FORMAT, serialFormat, 2);
    uint32_t dataFormat = (8 << 8) | 8; // 8 Bits pixel depth
    writeRegister(MT9F002_CPP_DATA_FORMAT, dataFormat, 2);

    writeRegister(MT9F002_MFR_3D00, 0x0435, 2);
    writeRegister(MT9F002_MFR_3D02, 0x435D, 2);
    writeRegister(MT9F002_MFR_3D04, 0x6698, 2);
    writeRegister(MT9F002_MFR_3D06, 0xFFFF, 2);
    writeRegister(MT9F002_MFR_3D08, 0x7783, 2);
    writeRegister(MT9F002_MFR_3D0A, 0x101B, 2);
    writeRegister(MT9F002_MFR_3D0C, 0x732C, 2);
    writeRegister(MT9F002_MFR_3D0E, 0x4230, 2);
    writeRegister(MT9F002_MFR_3D10, 0x5881, 2);
    writeRegister(MT9F002_MFR_3D12, 0x5C3A, 2);
    writeRegister(MT9F002_MFR_3D14, 0x0140, 2);
    writeRegister(MT9F002_MFR_3D16, 0x2300, 2);
    writeRegister(MT9F002_MFR_3D18, 0x815F, 2);
    writeRegister(MT9F002_MFR_3D1A, 0x6789, 2);
    writeRegister(MT9F002_MFR_3D1C, 0x5920, 2);
    writeRegister(MT9F002_MFR_3D1E, 0x0C20, 2);
    writeRegister(MT9F002_MFR_3D20, 0x21C0, 2);
    writeRegister(MT9F002_MFR_3D22, 0x4684, 2);
    writeRegister(MT9F002_MFR_3D24, 0x4892, 2);
    writeRegister(MT9F002_MFR_3D26, 0x1A00, 2);
    writeRegister(MT9F002_MFR_3D28, 0xBA4C, 2);
    writeRegister(MT9F002_MFR_3D2A, 0x8D48, 2);
    writeRegister(MT9F002_MFR_3D2C, 0x4641, 2);
    writeRegister(MT9F002_MFR_3D2E, 0x408C, 2);
    writeRegister(MT9F002_MFR_3D30, 0x4784, 2);
    writeRegister(MT9F002_MFR_3D32, 0x4A87, 2);
    writeRegister(MT9F002_MFR_3D34, 0x561A, 2);
    writeRegister(MT9F002_MFR_3D36, 0x00A5, 2);
    writeRegister(MT9F002_MFR_3D38, 0x1A00, 2);
    writeRegister(MT9F002_MFR_3D3A, 0x5693, 2);
    writeRegister(MT9F002_MFR_3D3C, 0x4D8D, 2);
    writeRegister(MT9F002_MFR_3D3E, 0x4A47, 2);
    writeRegister(MT9F002_MFR_3D40, 0x4041, 2);
    writeRegister(MT9F002_MFR_3D42, 0x8200, 2);
    writeRegister(MT9F002_MFR_3D44, 0x24B7, 2);
    writeRegister(MT9F002_MFR_3D46, 0x0024, 2);
    writeRegister(MT9F002_MFR_3D48, 0x8D4F, 2);
    writeRegister(MT9F002_MFR_3D4A, 0x831A, 2);
    writeRegister(MT9F002_MFR_3D4C, 0x00B4, 2);
    writeRegister(MT9F002_MFR_3D4E, 0x4684, 2);
    writeRegister(MT9F002_MFR_3D50, 0x49CE, 2);
    writeRegister(MT9F002_MFR_3D52, 0x4946, 2);
    writeRegister(MT9F002_MFR_3D54, 0x4140, 2);
    writeRegister(MT9F002_MFR_3D56, 0x9247, 2);
    writeRegister(MT9F002_MFR_3D58, 0x844B, 2);
    writeRegister(MT9F002_MFR_3D5A, 0xCE4B, 2);
    writeRegister(MT9F002_MFR_3D5C, 0x4741, 2);
    writeRegister(MT9F002_MFR_3D5E, 0x502F, 2);
    writeRegister(MT9F002_MFR_3D60, 0xBD3A, 2);
    writeRegister(MT9F002_MFR_3D62, 0x5181, 2);
    writeRegister(MT9F002_MFR_3D64, 0x5E73, 2);
    writeRegister(MT9F002_MFR_3D66, 0x7C0A, 2);
    writeRegister(MT9F002_MFR_3D68, 0x7770, 2);
    writeRegister(MT9F002_MFR_3D6A, 0x8085, 2);
    writeRegister(MT9F002_MFR_3D6C, 0x6A82, 2);
    writeRegister(MT9F002_MFR_3D6E, 0x6742, 2);
    writeRegister(MT9F002_MFR_3D70, 0x8244, 2);
    writeRegister(MT9F002_MFR_3D72, 0x831A, 2);
    writeRegister(MT9F002_MFR_3D74, 0x0099, 2);
    writeRegister(MT9F002_MFR_3D76, 0x44DF, 2);
    writeRegister(MT9F002_MFR_3D78, 0x1A00, 2);
    writeRegister(MT9F002_MFR_3D7A, 0x8542, 2);
    writeRegister(MT9F002_MFR_3D7C, 0x8567, 2);
    writeRegister(MT9F002_MFR_3D7E, 0x826A, 2);
    writeRegister(MT9F002_MFR_3D80, 0x857C, 2);
    writeRegister(MT9F002_MFR_3D82, 0x6B80, 2);
    writeRegister(MT9F002_MFR_3D84, 0x7000, 2);
    writeRegister(MT9F002_MFR_3D86, 0xB831, 2);
    writeRegister(MT9F002_MFR_3D88, 0x40BE, 2);
    writeRegister(MT9F002_MFR_3D8A, 0x6700, 2);
    writeRegister(MT9F002_MFR_3D8C, 0x0CBD, 2);
    writeRegister(MT9F002_MFR_3D8E, 0x4482, 2);
    writeRegister(MT9F002_MFR_3D90, 0x7898, 2);
    writeRegister(MT9F002_MFR_3D92, 0x7480, 2);
    writeRegister(MT9F002_MFR_3D94, 0x5680, 2);
    writeRegister(MT9F002_MFR_3D96, 0x9755, 2);
    writeRegister(MT9F002_MFR_3D98, 0x8057, 2);
    writeRegister(MT9F002_MFR_3D9A, 0x8056, 2);
    writeRegister(MT9F002_MFR_3D9C, 0x9256, 2);
    writeRegister(MT9F002_MFR_3D9E, 0x8057, 2);
    writeRegister(MT9F002_MFR_3DA0, 0x8055, 2);
    writeRegister(MT9F002_MFR_3DA2, 0x817C, 2);
    writeRegister(MT9F002_MFR_3DA4, 0x969B, 2);
    writeRegister(MT9F002_MFR_3DA6, 0x56A6, 2);
    writeRegister(MT9F002_MFR_3DA8, 0x44BE, 2);
    writeRegister(MT9F002_MFR_3DAA, 0x000C, 2);
    writeRegister(MT9F002_MFR_3DAC, 0x867A, 2);
    writeRegister(MT9F002_MFR_3DAE, 0x9474, 2);
    writeRegister(MT9F002_MFR_3DB0, 0x8A79, 2);
    writeRegister(MT9F002_MFR_3DB2, 0x9367, 2);
    writeRegister(MT9F002_MFR_3DB4, 0xBF6A, 2);
    writeRegister(MT9F002_MFR_3DB6, 0x816C, 2);
    writeRegister(MT9F002_MFR_3DB8, 0x8570, 2);
    writeRegister(MT9F002_MFR_3DBA, 0x836C, 2);
    writeRegister(MT9F002_MFR_3DBC, 0x826A, 2);
    writeRegister(MT9F002_MFR_3DBE, 0x8245, 2);
    writeRegister(MT9F002_MFR_3DC0, 0xFFFF, 2);
    writeRegister(MT9F002_MFR_3DC2, 0xFFD6, 2);
    writeRegister(MT9F002_MFR_3DC4, 0x4582, 2);
    writeRegister(MT9F002_MFR_3DC6, 0x6A82, 2);
    writeRegister(MT9F002_MFR_3DC8, 0x6C83, 2);
    writeRegister(MT9F002_MFR_3DCA, 0x7000, 2);
    writeRegister(MT9F002_MFR_3DCC, 0x8024, 2);
    writeRegister(MT9F002_MFR_3DCE, 0xB181, 2);
    writeRegister(MT9F002_MFR_3DD0, 0x6859, 2);
    writeRegister(MT9F002_MFR_3DD2, 0x732B, 2);
    writeRegister(MT9F002_MFR_3DD4, 0x4030, 2);
    writeRegister(MT9F002_MFR_3DD6, 0x4982, 2);
    writeRegister(MT9F002_MFR_3DD8, 0x101B, 2);
    writeRegister(MT9F002_MFR_3DDA, 0x4083, 2);
    writeRegister(MT9F002_MFR_3DDC, 0x6785, 2);
    writeRegister(MT9F002_MFR_3DDE, 0x3A00, 2);
    writeRegister(MT9F002_MFR_3DE0, 0x8820, 2);
    writeRegister(MT9F002_MFR_3DE2, 0x0C59, 2);
    writeRegister(MT9F002_MFR_3DE4, 0x8546, 2);
    writeRegister(MT9F002_MFR_3DE6, 0x8348, 2);
    writeRegister(MT9F002_MFR_3DE8, 0xD04C, 2);
    writeRegister(MT9F002_MFR_3DEA, 0x8B48, 2);
    writeRegister(MT9F002_MFR_3DEC, 0x4641, 2);
    writeRegister(MT9F002_MFR_3DEE, 0x4083, 2);
    writeRegister(MT9F002_MFR_3DF0, 0x1A00, 2);
    writeRegister(MT9F002_MFR_3DF2, 0x8347, 2);
    writeRegister(MT9F002_MFR_3DF4, 0x824A, 2);
    writeRegister(MT9F002_MFR_3DF6, 0x9A56, 2);
    writeRegister(MT9F002_MFR_3DF8, 0x1A00, 2);
    writeRegister(MT9F002_MFR_3DFA, 0x951A, 2);
    writeRegister(MT9F002_MFR_3DFC, 0x0056, 2);
    writeRegister(MT9F002_MFR_3DFE, 0x914D, 2);
    writeRegister(MT9F002_MFR_3E00, 0x8B4A, 2);
    writeRegister(MT9F002_MFR_3E02, 0x4700, 2);
    writeRegister(MT9F002_MFR_3E04, 0x0300, 2);
    writeRegister(MT9F002_MFR_3E06, 0x2492, 2);
    writeRegister(MT9F002_MFR_3E08, 0x0024, 2);
    writeRegister(MT9F002_MFR_3E0A, 0x8A1A, 2);
    writeRegister(MT9F002_MFR_3E0C, 0x004F, 2);
    writeRegister(MT9F002_MFR_3E0E, 0xB446, 2);
    writeRegister(MT9F002_MFR_3E10, 0x8349, 2);
    writeRegister(MT9F002_MFR_3E12, 0xB249, 2);
    writeRegister(MT9F002_MFR_3E14, 0x4641, 2);
    writeRegister(MT9F002_MFR_3E16, 0x408B, 2);
    writeRegister(MT9F002_MFR_3E18, 0x4783, 2);
    writeRegister(MT9F002_MFR_3E1A, 0x4BDB, 2);
    writeRegister(MT9F002_MFR_3E1C, 0x4B47, 2);
    writeRegister(MT9F002_MFR_3E1E, 0x4180, 2);
    writeRegister(MT9F002_MFR_3E20, 0x502B, 2);
    writeRegister(MT9F002_MFR_3E22, 0x4C3A, 2);
    writeRegister(MT9F002_MFR_3E24, 0x4180, 2);
    writeRegister(MT9F002_MFR_3E26, 0x737C, 2);
    writeRegister(MT9F002_MFR_3E28, 0xD124, 2);
    writeRegister(MT9F002_MFR_3E2A, 0x9068, 2);
    writeRegister(MT9F002_MFR_3E2C, 0x8A20, 2);
    writeRegister(MT9F002_MFR_3E2E, 0x2170, 2);
    writeRegister(MT9F002_MFR_3E30, 0x8081, 2);
    writeRegister(MT9F002_MFR_3E32, 0x6A67, 2);
    writeRegister(MT9F002_MFR_3E34, 0x4257, 2);
    writeRegister(MT9F002_MFR_3E36, 0x5544, 2);
    writeRegister(MT9F002_MFR_3E38, 0x8644, 2);
    writeRegister(MT9F002_MFR_3E3A, 0x9755, 2);
    writeRegister(MT9F002_MFR_3E3C, 0x5742, 2);
    writeRegister(MT9F002_MFR_3E3E, 0x676A, 2);
    writeRegister(MT9F002_MFR_3E40, 0x807D, 2);
    writeRegister(MT9F002_MFR_3E42, 0x3180, 2);
    writeRegister(MT9F002_MFR_3E44, 0x7000, 2);
    writeRegister(MT9F002_MFR_3E46, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E48, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E4A, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E4C, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E4E, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E50, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E52, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E54, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E56, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E58, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E5A, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E5C, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E5E, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E60, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E62, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E64, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E66, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E68, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E6A, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E6C, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E6E, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E70, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E72, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E74, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E76, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E78, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E7A, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E7C, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E7E, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E80, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E82, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E84, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E86, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E88, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E8A, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E8C, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E8E, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E90, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E92, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E94, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E96, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E98, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E9A, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E9C, 0x0000, 2);
    writeRegister(MT9F002_MFR_3E9E, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EA0, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EA2, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EA4, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EA6, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EA8, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EAA, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EAC, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EAE, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EB0, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EB2, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EB4, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EB6, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EB8, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EBA, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EBC, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EBE, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EC0, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EC2, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EC4, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EC6, 0x0000, 2);
    writeRegister(MT9F002_MFR_3EC8, 0x0000, 2);
    writeRegister(MT9F002_MFR_3ECA, 0x0000, 2);
    writeRegister(MT9F002_MFR_3176, 0x4000, 2);
    writeRegister(MT9F002_MFR_317C, 0xA00A, 2);
    writeRegister(MT9F002_MFR_3EE6, 0x0000, 2);
    writeRegister(MT9F002_MFR_3ED8, 0xE0E0, 2);
    writeRegister(MT9F002_MFR_3EE8, 0x0001, 2);
    writeRegister(MT9F002_SMIA_TEST, 0x0005, 2);
}

/**
 * @brief Configuration stage 2 for both MIPI and HiSPi interface
 *
 * This will do the second step of configuring the MT9F002 for both the
 * MIPI and the HiSPi interface. This only includes setting the SMIA_TEST
 * register.
 */
void MT9F002::mipiHispiStage2(void) {
    writeRegister(MT9F002_SMIA_TEST, 0x0045, 2);
}

/**
 * @brief Configuration stage 3 for both MIPI and HiSPi interface
 *
 * This will do the third and last step of configuring the MT9F002 for both
 * the MIPI and the HiSPi interface.
 */
void MT9F002::mipiHispiStage3(void) {
    writeRegister(MT9F002_EXTRA_DELAY      , 0x0000, 2);
    writeRegister(MT9F002_RESET_REGISTER   , 0x0118, 2);
    writeRegister(MT9F002_MFR_3EDC, 0x68CF, 2);
    writeRegister(MT9F002_MFR_3EE2, 0xE363, 2);
}

/**
 * @brief Configuration stage 1 for the Parallel interface
 *
 * This will do the first step of configuring the MT9F002 for the Parallell
 * interface.
 */
void MT9F002::parallelStage1(void) {
    writeRegister(MT9F002_RESET_REGISTER , 0x0010, 2);
    writeRegister(MT9F002_GLOBAL_GAIN    , 0x1430, 2);
    writeRegister(MT9F002_RESET_REGISTER , 0x0010, 2);
    writeRegister(MT9F002_RESET_REGISTER , 0x0010, 2);
    writeRegister(MT9F002_RESET_REGISTER , 0x0010, 2);
    writeRegister(MT9F002_DAC_LD_14_15   , 0xE525, 2);
    writeRegister(MT9F002_CTX_CONTROL_REG, 0x0000, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xF873, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x08AA, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x3219, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x3219, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x3219, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x3200, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x3200, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x3200, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x3200, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x3200, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x1769, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA6F3, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA6F3, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA6F3, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA6F3, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA6F3, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA6F3, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA6F3, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xAFF3, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xFA64, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xFA64, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xFA64, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xF164, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xFA64, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xFA64, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xFA64, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xF164, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x276E, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x18CF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x18CF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x18CF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x28CF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x18CF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x18CF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x18CF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x18CF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x2363, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x2363, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x2352, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x2363, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x2363, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x2363, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x2352, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x2352, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA394, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA394, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x8F8F, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA3D4, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA394, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xA394, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x8F8F, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x8FCF, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xDC23, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xDC63, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xDC63, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xDC23, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xDC23, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xDC63, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xDC63, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0xDC23, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x0F73, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x85C0, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x85C0, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x85C0, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x85C0, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x85C0, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x85C0, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x85C0, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x85C4, 2);
    writeRegister(MT9F002_CTX_WR_DATA_REG, 0x0000, 2);
    writeRegister(MT9F002_ANALOG_CONTROL4, 0x8000, 2);
    writeRegister(MT9F002_DAC_LD_14_15   , 0xE525, 2);
    writeRegister(MT9F002_DATA_PEDESTAL_ , 0x00A8, 2);
    writeRegister(MT9F002_RESET_REGISTER , 0x0090, 2);
    writeRegister(MT9F002_SERIAL_FORMAT  , 0x0301, 2);
    writeRegister(MT9F002_RESET_REGISTER , 0x1090, 2);
    writeRegister(MT9F002_SMIA_TEST      , 0x0845, 2);
    writeRegister(MT9F002_RESET_REGISTER , 0x1080, 2);
    writeRegister(MT9F002_DATAPATH_SELECT, 0xD880, 2);
    writeRegister(MT9F002_RESET_REGISTER , 0x9080, 2);
    writeRegister(MT9F002_DATAPATH_SELECT, 0xD880, 2);
    writeRegister(MT9F002_RESET_REGISTER , 0x10C8, 2);
    writeRegister(MT9F002_DATAPATH_SELECT, 0xD880, 2);
}

/**
 * @brief Configuration stage 2 for the Parallel interface
 *
 * This will do the second step of configuring the MT9F002 for the Parallell
 * interface.
 */
void MT9F002::parallelStage2(void) {
    writeRegister(MT9F002_ANALOG_CONTROL4, 0x8000, 2);
    writeRegister(MT9F002_READ_MODE, 0x0041, 2);

    writeRegister(MT9F002_READ_MODE              , 0x04C3, 2);
    writeRegister(MT9F002_READ_MODE              , 0x04C3, 2);
    writeRegister(MT9F002_ANALOG_CONTROL5        , 0x0000, 2);
    writeRegister(MT9F002_ANALOG_CONTROL5        , 0x0000, 2);
    writeRegister(MT9F002_ANALOG_CONTROL5        , 0x0000, 2);
    writeRegister(MT9F002_ANALOG_CONTROL5        , 0x0000, 2);
    writeRegister(MT9F002_DAC_LD_28_29           , 0x0047, 2);
    writeRegister(MT9F002_COLUMN_CORRECTION      , 0xB080, 2);
    writeRegister(MT9F002_COLUMN_CORRECTION      , 0xB100, 2);
    writeRegister(MT9F002_DARK_CONTROL3          , 0x0020, 2);
    writeRegister(MT9F002_DAC_LD_24_25           , 0x6349, 2);
    writeRegister(MT9F002_ANALOG_CONTROL7        , 0x800A, 2);
    writeRegister(MT9F002_RESET_REGISTER         , 0x90C8, 2);
    writeRegister(MT9F002_CTX_CONTROL_REG        , 0x8005, 2);
    writeRegister(MT9F002_ANALOG_CONTROL7        , 0x800A, 2);
    writeRegister(MT9F002_DAC_LD_28_29           , 0x0047, 2);
    writeRegister(MT9F002_DAC_LD_30_31           , 0x15F0, 2);
    writeRegister(MT9F002_DAC_LD_30_31           , 0x15F0, 2);
    writeRegister(MT9F002_DAC_LD_30_31           , 0x15F0, 2);
    writeRegister(MT9F002_DAC_LD_28_29           , 0x0047, 2);
    writeRegister(MT9F002_DAC_LD_28_29           , 0x0047, 2);
    writeRegister(MT9F002_RESET_REGISTER         , 0x10C8, 2);
    //writeRegister(MT9F002_RESET_REGISTER         , 0x14C8, 2); // reset bad frame
    writeRegister(MT9F002_COARSE_INTEGRATION_TIME, 0x08C3, 2);
    writeRegister(MT9F002_DIGITAL_TEST           , 0x0000, 2);
    //writeRegister(MT9F002_DATAPATH_SELECT        , 0xd881, 2); // permanent line valid
    writeRegister(MT9F002_DATAPATH_SELECT        , 0xd880, 2);
    writeRegister(MT9F002_READ_MODE              , 0x0041, 2);
    writeRegister(MT9F002_X_ODD_INC              , 0x0001, 2);
    writeRegister(MT9F002_Y_ODD_INC              , 0x0001, 2);
    writeRegister(MT9F002_MASK_CORRUPTED_FRAMES  , 0x0001, 1); // 0 output corrupted frame, 1 mask them
}

/**
 * @brief Write the PLL values to the registers
 *
 * This will write the PLL configuration values based on the pll_config.
 */
void MT9F002::writePll(void) {
    // Update registers
    writeRegister(MT9F002_VT_PIX_CLK_DIV , pll_config.vt_pix_clk_div, 2);
    writeRegister(MT9F002_VT_SYS_CLK_DIV , pll_config.vt_sys_clk_div, 2);
    writeRegister(MT9F002_PRE_PLL_CLK_DIV, pll_config.pre_pll_clk_div, 2);
    writeRegister(MT9F002_PLL_MULTIPLIER , pll_config.pll_multiplier, 2);
    writeRegister(MT9F002_OP_PIX_CLK_DIV , pll_config.op_pix_clk_div, 2);
    writeRegister(MT9F002_OP_SYS_CLK_DIV , pll_config.op_sys_clk_div, 2);

    uint16_t smia = readRegister(MT9F002_SMIA_TEST, 2);
    writeRegister(MT9F002_SMIA_TEST, (smia & 0xFFBF) | (pll_config.shift_vt_pix_clk_div<<6), 2);

    // Set the row speed
    uint16_t row_speed = readRegister(MT9F002_ROW_SPEED, 2);
    row_speed = (row_speed & 0xFFF8) | (pll_config.rowSpeed_2_0 & 0x07);
    row_speed = (row_speed & 0xF8FF) | ((pll_config.row_speed_10_8 & 0x07)<<8);
    row_speed = (row_speed&(~0x70)) | (0x2<<4); // Change opclk_delay
    writeRegister(MT9F002_ROW_SPEED, row_speed, 2);

    // Compute clocks
    vt_pix_clk = pll_config.input_clk_freq * (float)pll_config.pll_multiplier * (float)(1+pll_config.shift_vt_pix_clk_div)
                 / ((float)pll_config.pre_pll_clk_div * (float)pll_config.vt_sys_clk_div * (float)pll_config.vt_pix_clk_div);
    op_pix_clk = pll_config.input_clk_freq * (float)pll_config.pll_multiplier
                 / ((float)pll_config.pre_pll_clk_div * (float)pll_config.op_sys_clk_div * (float)pll_config.op_pix_clk_div);
}

/**
 * @brief Write the resolution information to the registers
 *
 * This will write to the registers configuring the resolution, skipping and scaling.
 */
void MT9F002::writeResolution(void) {
    // Set window pos
    writeRegister(MT9F002_X_ADDR_START, res_config.offset_x, 2);
    writeRegister(MT9F002_Y_ADDR_START, res_config.offset_y, 2);
    writeRegister(MT9F002_X_ADDR_END, res_config.offset_x + res_config.sensor_width - 1, 2);
    writeRegister(MT9F002_Y_ADDR_END, res_config.offset_y + res_config.sensor_height - 1, 2);

    // Set output resolution
    writeRegister(MT9F002_X_OUTPUT_SIZE, res_config.output_width, 2);
    writeRegister(MT9F002_Y_OUTPUT_SIZE, res_config.output_height, 2);

    // Calculate scaled width and height
    scaled_width = ceil((float)res_config.output_width / res_config.output_scaler);
    scaled_height = ceil((float)res_config.output_height / res_config.output_scaler);

    // Enable scaler
    if (res_config.output_scaler != 1.0) {
        writeRegister(MT9F002_SCALING_MODE, 2, 2);
        writeRegister(MT9F002_SCALE_M, (int) ceil((float)MT9F002_SCALER_N / res_config.output_scaler), 2);
    }
}

/**
 * @brief Write the blanking information to the registers
 *
 * This will write to the blanking registers based on the FPS.
 */
void MT9F002::writeBlanking(void) {
    /* Read some config values in order to calculate blanking configuration */
    uint16_t x_odd_inc = readRegister(MT9F002_X_ODD_INC, 2);
    uint16_t min_frame_blanking_lines = readRegister(MT9F002_MIN_FRAME_BLANKING_LINES, 2);

    /* Calculate minimum line length */
    float subsampling_factor = (float)(1 + x_odd_inc) / 2.0f; // See page 52
    uint16_t min_line_length = std::max((float)blank_config.min_line_length_pck, scaled_width/subsampling_factor + blank_config.min_line_blanking_pck); // EQ 9
    min_line_length = std::max((float)min_line_length, (scaled_width-1 + x_odd_inc) / subsampling_factor/2 + blank_config.min_line_blanking_pck);
    if (interface == MIPI || interface == HiSPi) {
        min_line_length = std::max((int)min_line_length, ((uint16_t)((float)scaled_width * vt_pix_clk / op_pix_clk)/2) + blank_config.min_line_fifo_pck); // 2 lanes, pll clocks
    } else {
        min_line_length = std::max((int)min_line_length, ((uint16_t)((float)scaled_width * vt_pix_clk / op_pix_clk)) + blank_config.min_line_fifo_pck); // pll clocks
    }

    /* Do some magic to get it to work with P7 ISP (with horizontal blanking) */
    uint32_t clkRatio_num = pll_config.op_sys_clk_div * pll_config.op_pix_clk_div * pll_config.row_speed_10_8 * (1 + pll_config.shift_vt_pix_clk_div);
    uint32_t clkRatio_den = pll_config.vt_sys_clk_div * pll_config.vt_pix_clk_div;

    /* Divide by the GCD to find smallest ratio */
    uint32_t clkRatio_gcd = gcd(clkRatio_num, clkRatio_den);
    clkRatio_num = clkRatio_num / clkRatio_gcd;
    clkRatio_den = clkRatio_den / clkRatio_gcd;

    /* Calculate minimum horizontal blanking, since fpga line_length must be divideable by 2 */
    uint32_t min_horizontal_blanking = clkRatio_num;
    if((clkRatio_den % 2) != 0) {
        min_horizontal_blanking = 2 * clkRatio_num;
    }

    /* Fix fpga correction based on min horizontal blanking */
    if ((min_line_length % min_horizontal_blanking) != 0) {
        min_line_length += min_horizontal_blanking - (min_line_length % min_horizontal_blanking);
    }

    /* Calculate minimum frame length lines */
    uint16_t min_frame_length = (scaled_height) / subsampling_factor + min_frame_blanking_lines; // (EQ 10)

    /* Calculate FPS we get using these minimums (Maximum FPS) */
    line_length = min_line_length;
    frame_length = min_frame_length;
    real_fps = vt_pix_clk * 1000000 / (float)(line_length * frame_length);

    /* Check if we need to downscale the FPS and bruteforce better solution */
    if(target_fps < real_fps) {
        float min_fps_err = fabs(target_fps - real_fps);
        float new_fps = real_fps;

        // Go through all possible line lengths
        for(uint32_t ll = min_line_length; ll <= MT9F002_LINE_LENGTH_MAX; ll += min_horizontal_blanking) {
            // Go through all possible frame lengths
            for(uint32_t fl = min_frame_length; fl < MT9F002_FRAME_LENGTH_MAX; ++fl) {
                new_fps = vt_pix_clk * 1000000 / (float)(ll * fl);

                // Calculate FPS error and save if it is better
                float fps_err = fabs(target_fps - new_fps);
                if(fps_err < min_fps_err) {
                    min_fps_err = fps_err;
                    line_length = ll;
                    frame_length = fl;
                    real_fps = new_fps;
                }

                // Stop searching if FPS is lower or equal
                if(target_fps > new_fps) {
                    break;
                }
            }

            // Calculate if next step is still needed (since we only need to go one step below target_fps)
            new_fps = vt_pix_clk * 1000000 / (float)(ll * min_frame_length);

            // Stop searching if FPS is lower or equal
            if(target_fps > new_fps) {
                break;
            }
        }
    }

    /* Actually set the calculated values */
    writeRegister(MT9F002_LINE_LENGTH_PCK, line_length, 2);
    writeRegister(MT9F002_FRAME_LENGTH_LINES, frame_length, 2);
}

/**
 * @brief Write the exposure information to the registers
 *
 * This will write to the exposure registers based on the target exposure in ms.
 */
void MT9F002::writeExposure(void) {
    /* Fetch minimum and maximum integration times */
    uint16_t coarse_integration_min = readRegister(MT9F002_COARSE_INTEGRATION_TIME_MIN, 2);
    uint16_t coarse_integration_max = frame_length - readRegister(MT9F002_COARSE_INTEGRATION_TIME_MAX_MARGIN, 2);
    uint16_t fine_integration_max = line_length - blank_config.fine_integration_time_max_margin;

    /* Compute fine and coarse integration time */
    uint32_t integration = target_exposure * vt_pix_clk * 1000;
    uint16_t coarse_integration = integration / line_length;
    uint16_t fine_integration = integration % line_length;

    /* Make sure fine integration is inside bounds */
    if(blank_config.fine_integration_time_min > fine_integration || fine_integration > fine_integration_max) {
        int32_t upper_coarse_integration = coarse_integration + 1;
        int32_t upper_fine_integration = blank_config.fine_integration_time_min;

        int32_t lower_coarse_integration = coarse_integration - 1;
        int32_t lower_fine_integration = fine_integration_max;

        // Check if lower case is invalid (take upper coarse)
        if(lower_coarse_integration < coarse_integration_min) {
            coarse_integration = upper_coarse_integration;
            fine_integration = upper_fine_integration;
        }
        // Check if upper case is invalid (take lower coarse)
        else if(upper_coarse_integration > coarse_integration_max) {
            coarse_integration = lower_coarse_integration;
            fine_integration = lower_fine_integration;
        }
        // Both are good
        else {
            // Calculate error to decide which is better
            int32_t upper_error = abs((line_length * upper_coarse_integration + upper_fine_integration) - integration);
            int32_t lower_error = abs((line_length * lower_coarse_integration + lower_fine_integration) - integration);

            if(upper_error < lower_error) {
                coarse_integration = upper_coarse_integration;
                fine_integration = upper_fine_integration;
            } else {
                coarse_integration = lower_coarse_integration;
                fine_integration = lower_fine_integration;
            }
        }
    }

    /* Fix saturations */
    if(fine_integration < blank_config.fine_integration_time_min)
        fine_integration = blank_config.fine_integration_time_min;
    else if(fine_integration > fine_integration_max)
        fine_integration = fine_integration_max;

    if(coarse_integration < coarse_integration_min)
        coarse_integration = coarse_integration_min;
    else if(coarse_integration > coarse_integration_max)
        coarse_integration = coarse_integration_max;

    /* Set the registers */
    real_exposure = (float)(coarse_integration * line_length + fine_integration) / (vt_pix_clk * 1000);
    writeRegister(MT9F002_COARSE_INTEGRATION_TIME, coarse_integration, 2);
    writeRegister(MT9F002_FINE_INTEGRATION_TIME_, fine_integration, 2);
}

/**
 * @brief Calculate the color gain register based on the gain value
 *
 * This will calculate the register value for a gain based on a gain value. This will
 * set the colamp, analog and digital gain.
 * @param[in] gain The target gain from 1.0 to 63.50
 * @return The register value
 */
uint16_t MT9F002::calculateGain(float gain) {
    // Check if gain is valid
    if(gain < 1.0) {
        gain = 1.0;
    }

    // Calculation of colamp, analg3 and digital gain based on table 19 p56
    uint8_t colamp_gain, analog_gain3, digital_gain;
    if (gain < 1.50) {
        // This is not recommended
        colamp_gain = 0;
        analog_gain3 = 0;
        digital_gain = 1;
    } else if (gain < 3.0) {
        colamp_gain = 1;
        analog_gain3 = 0;
        digital_gain = 1;
    } else if (gain < 6.0) {
        colamp_gain = 2;
        analog_gain3 = 0;
        digital_gain = 1;
    } else if (gain < 16.0) {
        colamp_gain = 3;
        analog_gain3 = 0;
        digital_gain = 1;
    } else if (gain < 32.0) {
        colamp_gain = 3;
        analog_gain3 = 0;
        digital_gain = 2;
    } else {
        colamp_gain = 3;
        analog_gain3 = 0;
        digital_gain = 4;
    }

    // Calculate gain 2 (fine gain)
    uint16_t analog_gain2 = gain / (float)digital_gain / (float)(1<<colamp_gain) / (float)(1<<analog_gain3) * 64.0;
    if(analog_gain2 < 1)
        analog_gain2 = 1;
    else if(analog_gain2 > 127)
        analog_gain2 = 127;

    return (analog_gain2 & 0x7F) | ((analog_gain3 & 0x7) << 7) | ((colamp_gain & 0x3) << 10) | ((digital_gain & 0xF) << 12);
}

/**
 * @brief Write the gains information to the registers
 *
 * This will write the MT9F002_GREEN1_GAIN, MT9F002_BLUE_GAIN, MT9F002_RED_GAIN and
 * MT9F002_GREEN2_GAIN register based on the gain_config.
 */
void MT9F002::writeGains(void) {
    writeRegister(MT9F002_GREEN1_GAIN, calculateGain(gain_config.green1), 2);
    writeRegister(MT9F002_BLUE_GAIN,   calculateGain(gain_config.blue), 2);
    writeRegister(MT9F002_RED_GAIN,    calculateGain(gain_config.red), 2);
    writeRegister(MT9F002_GREEN2_GAIN, calculateGain(gain_config.green2), 2);
}

/**
 * @brief Calculate the greatest common divider
 *
 * This is a simple iterative implementation for finding the greatest common divider for
 * two numbers a and b.
 * TODO: Move this function ot a math library
 * @param[in] a The first number
 * @param[in] b The second number
 * @return The greatest common divider
 */
int MT9F002::gcd(int a, int b) {
    while(true) {
        a %= b;
        if (a == 0)
            return b;
        b %= a;

        if (b == 0)
            return a;
    }
}

/**
 * @brief This will calculate the new resolution configuration
 *
 * Based in the output size and cropping this will calculate the new resolution configuration. It
 * will first set the correct output width and height and calculate the nearest skipping in both
 * x and y direction. Based on this skipping the latest part will be scaled using the scaler.
 * Note that sometimes the output width and height will change as certain configurations are
 * impossible.
 */
void MT9F002::calculateResolution(void) {
    // First calculate the skipping in X and Y direction
    /*static const uint8_t xy_odd_inc_tab[] = {1, 1, 3, 3, 7, 7, 7, 7, 15, 15, 15, 15, 15, 15, 15, 15, 31};
    uint32_t width_res = res_config.sensor_width / res_config.output_width;
    uint32_t height_res = res_config.sensor_height / res_config.output_height;
    width_res = (width_res > 4)? 4 : width_res;
    height_res = (height_res > 16)? 16 : height_res;
    res_config.x_odd_inc = xy_odd_inc_tab[width_res];
    res_config.y_odd_inc = xy_odd_inc_tab[height_res];*/


    // Calculate remaining scaling not handled by binning / skipping
    /*float hratio = ((res_config.output_width / ((res_config.x_odd_inc + 1) / 2) * MT9F002_SCALER_N) + res_config.sensor_width - 1) /
         res_config.sensor_width;
    float vratio = ((res_config.output_height / ((res_config.y_odd_inc + 1) / 2) * MT9F002_SCALER_N) + res_config.sensor_height -
          1) / res_config.sensor_height;
    float ratio = min(hratio, vratio);*/
    // TODO: Fix this calculation
}

/**
 * @brief Set the CMOS output size
 *
 * This will set the correct output width and height in pixels of the MT9F002 CMOS sensor. Based
 * on the cropping the new skipping and scaling is calculted to achieve this output width and height.
 * @param width The output width in pixels (must be even to make sure we have a full GRGB pattern)
 * @param height The output height in pixels (must be even to make sure we have a full GRGB pattern)
 */
void MT9F002::setOutput(uint16_t width, uint16_t height) {
    assert(width%2 == 0);
    assert(height%2 == 0);
}
