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

#include "drivers/mt9v117.h"

#include "drivers/mt9v117_regs.h"
#include "drivers/clogger.h"
#include <stdexcept>
#include <assert.h>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * @brief Initialize the Aptina MT9V117 CMOS chip
 *
 * This will initialize the Aptina MT9V117 CMOS chip and set the default configuration. This default
 * configuration inclused exposure, fps etc.
 * @param[in] i2c_bus The i2c bus which the MT9V117 CMOS chip is connected to
 */
MT9V117::MT9V117(I2CBus *i2c_bus) {
    // Save the i2c bus
    this->i2c_bus = i2c_bus;

    /* Reset the device */
    int gpio129 = open("/sys/class/gpio/gpio129/value", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    int wc = write(gpio129, "0", 1);
    wc += write(gpio129, "1", 1);
    close(gpio129);

    if (wc != 2) {
        CLOGGER_WARN("MT9V117 couldn't write to GPIO 129");
    }

    /* Start PWM 9 (Which probably is the clock of the MT9V117) */
    //#define BEBOP_CAMV_PWM_FREQ 43333333
    int pwm9 = open("/sys/class/pwm/pwm_9/run", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    wc = write(pwm9, "0", 1);
    wc += write(pwm9, "1", 1);
    close(pwm9);

    if (wc != 2) {
        CLOGGER_WARN("MT9V117 couldn't write to PWM");
    }

    /* Wait 50ms */
    usleep(50000);

    /* See if the device is there and correct */
    uint16_t chip_id = readRegister(MT9V117_CHIP_ID, 2);
    if(chip_id != MT9V117_CHIP_ID_RESP) {
        throw std::runtime_error("[MT9V117] Didn't get correct response from CHIP_ID (expected: " + std::to_string(MT9V117_CHIP_ID_RESP) + ", got: " + std::to_string(chip_id) + ")");
    }

    /* Reset the device with software */
    writeRegister(MT9V117_RESET_MISC_CTRL, MT9V117_RESET_SOC_I2C, 2);
    writeRegister(MT9V117_RESET_MISC_CTRL, 0, 2);

    /* Wait 50ms */
    usleep(50000);

    /* Apply MT9V117 software patch */
    writePatch();

    /* Set basic settings */
    writeVar(MT9V117_AWB_VAR, MT9V117_AWB_PIXEL_THRESHOLD_COUNT_OFFSET, 50000, 4);
    writeVar(MT9V117_AE_RULE_VAR, MT9V117_AE_RULE_ALGO_OFFSET, MT9V117_AE_RULE_ALGO_AVERAGE, 2);

    /* Set pixclk pad slew to 6 and data out pad slew to 1 */
    writeRegister(MT9V117_PAD_SLEW, readRegister(MT9V117_PAD_SLEW, 2) | 0x0600 | 0x0001, 2);

    /* Configure the MT9V117 sensor */
    writeConfig();

    /* Enable ITU656 */
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_OUTPUT_FORMAT_OFFSET,
             readVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_OUTPUT_FORMAT_OFFSET, 2) |
             MT9V117_CAM_OUTPUT_FORMAT_BT656_ENABLE, 2);

    /* Apply the configuration */
    writeVar(MT9V117_SYSMGR_VAR, MT9V117_SYSMGR_NEXT_STATE_OFFSET, MT9V117_SYS_STATE_ENTER_CONFIG_CHANGE, 1);
    writeRegister(MT9V117_COMMAND, MT9V117_COMMAND_OK | MT9V117_COMMAND_SET_STATE, 2);

    /* Wait for command OK */
    for(uint8_t retries = 100; retries > 0; retries--) {
        /* Wait 10ms */
        usleep(10000);

        /* Check the command */
        uint16_t cmd = readRegister(MT9V117_COMMAND, 2);
        if((cmd & MT9V117_COMMAND_SET_STATE) == 0) {
            if((cmd & MT9V117_COMMAND_OK) == 0) {
                CLOGGER_WARN("Switching MT9V117 config failed (No OK)");
            }

            // Successfully configured!
            return;
        }
    }

    CLOGGER_WARN("Could not switch MT9V117 to new config after 100 tries.");
}

/**
 * @brief Write to a register
 *
 * This will write an integer value to a register of a certain length
 * @param[in] address The address to write to
 * @param[in] value The value to write to the register
 * @param[in] length The length of the value in bytes to write (1, 2 or 4)
 */
void MT9V117::writeRegister(uint16_t address, uint32_t value, uint8_t length) {
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
    this->i2c_bus->transmit(MT9V117_ADDRESS, bytes, length+2);
}

/**
 * @brief Read from a register
 *
 * This will read from a register and return the value as an integer.
 * @param[in] address The address to read fromr
 * @param[in] length The amount of bytes to read
 * @return The register value
 */
uint32_t MT9V117::readRegister(uint16_t address, uint8_t length) {
    uint8_t bytes[length+2];
    assert(this->i2c_bus != NULL);
    assert(length <= 4);

    // Set the address
    bytes[0] = address >> 8;
    bytes[1] = address & 0xFF;

    // Transmit the buffer and receive back
    this->i2c_bus->transceive(MT9V117_ADDRESS, bytes, 2, length);

    /* Fix sigdness */
    uint32_t ret = 0;
    for(uint8_t i =0; i < length; i++) {
        ret |= bytes[length-i-1] << (8*i);
    }
    return ret;
}

void MT9V117::writeVar(uint16_t var, uint16_t offset, uint32_t value, uint8_t length) {
    uint16_t address = 0x8000 | (var << 10) | offset;
    writeRegister(address, value, length);
}

uint32_t MT9V117::readVar(uint16_t var, uint16_t offset, uint8_t length) {
    uint16_t address = 0x8000 | (var << 10) | offset;
    return readRegister(address, length);
}

void MT9V117::writePatch(void) {
    /* Errata item 2 */
    writeRegister(0x301a, 0x10d0, 2);
    writeRegister(0x31c0, 0x1404, 2);
    writeRegister(0x3ed8, 0x879c, 2);
    writeRegister(0x3042, 0x20e1, 2);
    writeRegister(0x30d4, 0x8020, 2);
    writeRegister(0x30c0, 0x0026, 2);
    writeRegister(0x301a, 0x10d4, 2);

    /* Errata item 6 */
    writeVar(MT9V117_AE_TRACK_VAR, 0x0002, 0x00d3, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, 0x0078, 0x00a0, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, 0x0076, 0x0140, 2);

    /* Errata item 8 */
    writeVar(MT9V117_LOW_LIGHT_VAR, 0x0004, 0x00fc, 2);
    writeVar(MT9V117_LOW_LIGHT_VAR, 0x0038, 0x007f, 2);
    writeVar(MT9V117_LOW_LIGHT_VAR, 0x003a, 0x007f, 2);
    writeVar(MT9V117_LOW_LIGHT_VAR, 0x003c, 0x007f, 2);
    writeVar(MT9V117_LOW_LIGHT_VAR, 0x0004, 0x00f4, 2);

    /* Patch 0403; Critical; Sensor optimization */
    writeRegister(MT9V117_ACCESS_CTL_STAT, 0x0001, 2);
    writeRegister(MT9V117_PHYSICAL_ADDRESS_ACCESS, 0x7000, 2);

    /* Patch lines */
    uint8_t patch_line1[] = {
        0xf0, 0x00, 0x72, 0xcf, 0xff, 0x00, 0x3e, 0xd0, 0x92, 0x00,
        0x71, 0xcf, 0xff, 0xff, 0xf2, 0x18, 0xb1, 0x10, 0x92, 0x05,
        0xb1, 0x11, 0x92, 0x04, 0xb1, 0x12, 0x70, 0xcf, 0xff, 0x00,
        0x30, 0xc0, 0x90, 0x00, 0x7f, 0xe0, 0xb1, 0x13, 0x70, 0xcf,
        0xff, 0xff, 0xe7, 0x1c, 0x88, 0x36, 0x09, 0x0f, 0x00, 0xb3
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line1, sizeof(patch_line1));

    uint8_t patch_line2[] = {
        0xf0, 0x30, 0x69, 0x13, 0xe1, 0x80, 0xd8, 0x08, 0x20, 0xca,
        0x03, 0x22, 0x71, 0xcf, 0xff, 0xff, 0xe5, 0x68, 0x91, 0x35,
        0x22, 0x0a, 0x1f, 0x80, 0xff, 0xff, 0xf2, 0x18, 0x29, 0x05,
        0x00, 0x3e, 0x12, 0x22, 0x11, 0x01, 0x21, 0x04, 0x0f, 0x81,
        0x00, 0x00, 0xff, 0xf0, 0x21, 0x8c, 0xf0, 0x10, 0x1a, 0x22
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line2, sizeof(patch_line2));

    uint8_t patch_line3[] = {
        0xf0, 0x60, 0x10, 0x44, 0x12, 0x20, 0x11, 0x02, 0xf7, 0x87,
        0x22, 0x4f, 0x03, 0x83, 0x1a, 0x20, 0x10, 0xc4, 0xf0, 0x09,
        0xba, 0xae, 0x7b, 0x50, 0x1a, 0x20, 0x10, 0x84, 0x21, 0x45,
        0x01, 0xc1, 0x1a, 0x22, 0x10, 0x44, 0x70, 0xcf, 0xff, 0x00,
        0x3e, 0xd0, 0xb0, 0x60, 0xb0, 0x25, 0x7e, 0xe0, 0x78, 0xe0
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line3, sizeof(patch_line3));

    uint8_t patch_line4[] = {
        0xf0, 0x90, 0x71, 0xcf, 0xff, 0xff, 0xf2, 0x18, 0x91, 0x12,
        0x72, 0xcf, 0xff, 0xff, 0xe7, 0x1c, 0x8a, 0x57, 0x20, 0x04,
        0x0f, 0x80, 0x00, 0x00, 0xff, 0xf0, 0xe2, 0x80, 0x20, 0xc5,
        0x01, 0x61, 0x20, 0xc5, 0x03, 0x22, 0xb1, 0x12, 0x71, 0xcf,
        0xff, 0x00, 0x3e, 0xd0, 0xb1, 0x04, 0x7e, 0xe0, 0x78, 0xe0
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line4, sizeof(patch_line4));

    static uint8_t patch_line5[] = {
        0xf0, 0xc0, 0x70, 0xcf, 0xff, 0xff, 0xe7, 0x1c, 0x88, 0x57,
        0x71, 0xcf, 0xff, 0xff, 0xf2, 0x18, 0x91, 0x13, 0xea, 0x84,
        0xb8, 0xa9, 0x78, 0x10, 0xf0, 0x03, 0xb8, 0x89, 0xb8, 0x8c,
        0xb1, 0x13, 0x71, 0xcf, 0xff, 0x00, 0x30, 0xc0, 0xb1, 0x00,
        0x7e, 0xe0, 0xc0, 0xf1, 0x09, 0x1e, 0x03, 0xc0, 0xc1, 0xa1
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line5, sizeof(patch_line5));

    static uint8_t patch_line6[] = {
        0xf0, 0xf0, 0x75, 0x08, 0x76, 0x28, 0x77, 0x48, 0xc2, 0x40,
        0xd8, 0x20, 0x71, 0xcf, 0x00, 0x03, 0x20, 0x67, 0xda, 0x02,
        0x08, 0xae, 0x03, 0xa0, 0x73, 0xc9, 0x0e, 0x25, 0x13, 0xc0,
        0x0b, 0x5e, 0x01, 0x60, 0xd8, 0x06, 0xff, 0xbc, 0x0c, 0xce,
        0x01, 0x00, 0xd8, 0x00, 0xb8, 0x9e, 0x0e, 0x5a, 0x03, 0x20
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line6, sizeof(patch_line6));

    static uint8_t patch_line7[] = {
        0xf1, 0x20, 0xd9, 0x01, 0xd8, 0x00, 0xb8, 0x9e, 0x0e, 0xb6,
        0x03, 0x20, 0xd9, 0x01, 0x8d, 0x14, 0x08, 0x17, 0x01, 0x91,
        0x8d, 0x16, 0xe8, 0x07, 0x0b, 0x36, 0x01, 0x60, 0xd8, 0x07,
        0x0b, 0x52, 0x01, 0x60, 0xd8, 0x11, 0x8d, 0x14, 0xe0, 0x87,
        0xd8, 0x00, 0x20, 0xca, 0x02, 0x62, 0x00, 0xc9, 0x03, 0xe0
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line7, sizeof(patch_line7));

    static uint8_t patch_line8[] = {
        0xf1, 0x50, 0xc0, 0xa1, 0x78, 0xe0, 0xc0, 0xf1, 0x08, 0xb2,
        0x03, 0xc0, 0x76, 0xcf, 0xff, 0xff, 0xe5, 0x40, 0x75, 0xcf,
        0xff, 0xff, 0xe5, 0x68, 0x95, 0x17, 0x96, 0x40, 0x77, 0xcf,
        0xff, 0xff, 0xe5, 0x42, 0x95, 0x38, 0x0a, 0x0d, 0x00, 0x01,
        0x97, 0x40, 0x0a, 0x11, 0x00, 0x40, 0x0b, 0x0a, 0x01, 0x00
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line8, sizeof(patch_line8));

    static uint8_t patch_line9[] = {
        0xf1, 0x80, 0x95, 0x17, 0xb6, 0x00, 0x95, 0x18, 0xb7, 0x00,
        0x76, 0xcf, 0xff, 0xff, 0xe5, 0x44, 0x96, 0x20, 0x95, 0x15,
        0x08, 0x13, 0x00, 0x40, 0x0e, 0x1e, 0x01, 0x20, 0xd9, 0x00,
        0x95, 0x15, 0xb6, 0x00, 0xff, 0xa1, 0x75, 0xcf, 0xff, 0xff,
        0xe7, 0x1c, 0x77, 0xcf, 0xff, 0xff, 0xe5, 0x46, 0x97, 0x40
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line9, sizeof(patch_line9));

    static uint8_t patch_line10[] = {
        0xf1, 0xb0, 0x8d, 0x16, 0x76, 0xcf, 0xff, 0xff, 0xe5, 0x48,
        0x8d, 0x37, 0x08, 0x0d, 0x00, 0x81, 0x96, 0x40, 0x09, 0x15,
        0x00, 0x80, 0x0f, 0xd6, 0x01, 0x00, 0x8d, 0x16, 0xb7, 0x00,
        0x8d, 0x17, 0xb6, 0x00, 0xff, 0xb0, 0xff, 0xbc, 0x00, 0x41,
        0x03, 0xc0, 0xc0, 0xf1, 0x0d, 0x9e, 0x01, 0x00, 0xe8, 0x04
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line10, sizeof(patch_line10));

    static uint8_t patch_line11[] = {
        0xf1, 0xe0, 0xff, 0x88, 0xf0, 0x0a, 0x0d, 0x6a, 0x01, 0x00,
        0x0d, 0x8e, 0x01, 0x00, 0xe8, 0x7e, 0xff, 0x85, 0x0d, 0x72,
        0x01, 0x00, 0xff, 0x8c, 0xff, 0xa7, 0xff, 0xb2, 0xd8, 0x00,
        0x73, 0xcf, 0xff, 0xff, 0xf2, 0x40, 0x23, 0x15, 0x00, 0x01,
        0x81, 0x41, 0xe0, 0x02, 0x81, 0x20, 0x08, 0xf7, 0x81, 0x34
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line11, sizeof(patch_line11));

    static uint8_t patch_line12[] = {
        0xf2, 0x10, 0xa1, 0x40, 0xd8, 0x00, 0xc0, 0xd1, 0x7e, 0xe0,
        0x53, 0x51, 0x30, 0x34, 0x20, 0x6f, 0x6e, 0x5f, 0x73, 0x74,
        0x61, 0x72, 0x74, 0x5f, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6d,
        0x69, 0x6e, 0x67, 0x20, 0x25, 0x64, 0x20, 0x25, 0x64, 0x0a,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line12, sizeof(patch_line12));

    static uint8_t patch_line13[] = {
        0xf2, 0x40, 0xff, 0xff, 0xe8, 0x28, 0xff, 0xff, 0xf0, 0xe8,
        0xff, 0xff, 0xe8, 0x08, 0xff, 0xff, 0xf1, 0x54
    };
    this->i2c_bus->transmit(MT9V117_ADDRESS, patch_line13, sizeof(patch_line13));

    /* Confirm patch lines */
    writeRegister(MT9V117_LOGICAL_ADDRESS_ACCESS, 0x0000, 2);
    writeVar(MT9V117_PATCHLDR_VAR, MT9V117_PATCHLDR_LOADER_ADDRESS_OFFSET, 0x05d8, 2);
    writeVar(MT9V117_PATCHLDR_VAR, MT9V117_PATCHLDR_PATCH_ID_OFFSET, 0x0403, 2);
    writeVar(MT9V117_PATCHLDR_VAR, MT9V117_PATCHLDR_FIRMWARE_ID_OFFSET, 0x00430104, 4);
    writeRegister(MT9V117_COMMAND, MT9V117_COMMAND_OK | MT9V117_COMMAND_APPLY_PATCH, 2);

    /* Wait for command OK */
    for(uint8_t retries = 100; retries > 0; retries--) {
        /* Wait 10ms */
        usleep(10000);

        /* Check the command */
        uint16_t cmd = readRegister(MT9V117_COMMAND, 2);
        if((cmd & MT9V117_COMMAND_APPLY_PATCH) == 0) {
            if((cmd & MT9V117_COMMAND_OK) == 0) {
                CLOGGER_WARN("Applying MT9V117 patch failed (No OK)");
            }
            return;
        }
    }

    CLOGGER_WARN("Applying MT9V117 patch failed after 100 retries\r\n");
}

void MT9V117::writeConfig(void) {
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CFG_X_ADDR_START_OFFSET, 16, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CFG_X_ADDR_END_OFFSET, 663, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CFG_Y_ADDR_START_OFFSET, 8, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CFG_Y_ADDR_END_OFFSET,  501, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CFG_CPIPE_LAST_ROW_OFFSET, 243, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CFG_FRAME_LENGTH_LINES_OFFSET, 283, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CONTROL_READ_MODE_OFFSET, MT9V117_CAM_SENSOR_CONTROL_Y_SKIP_EN, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CFG_MAX_FDZONE_60_OFFSET, 1, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_SENSOR_CFG_TARGET_FDZONE_60_OFFSET, 1, 2);

    writeRegister(MT9V117_AE_TRACK_JUMP_DIVISOR, 0x03, 1);
    writeRegister(MT9V117_CAM_AET_SKIP_FRAMES, 0x02, 1);

    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_OUTPUT_WIDTH_OFFSET, 320, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_OUTPUT_HEIGHT_OFFSET, 240, 2);

    /* Set gain metric for 111.2 fps
     * The final fps depends on the input clock
     * (89.2fps on bebop) so a modification may be needed here */
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_LL_START_GAIN_METRIC_OFFSET, 0x03e8, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_LL_STOP_GAIN_METRIC_OFFSET, 0x1770, 2);

    /* set crop window */
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_CROP_WINDOW_XOFFSET_OFFSET, 0, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_CROP_WINDOW_YOFFSET_OFFSET, 0, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_CROP_WINDOW_WIDTH_OFFSET, 640, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_CROP_WINDOW_HEIGHT_OFFSET, 240, 2);

    /* Enable auto-stats mode */
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_CROP_MODE_OFFSET, 3, 1);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_STAT_AWB_HG_WINDOW_XEND_OFFSET, 319, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_STAT_AWB_HG_WINDOW_YEND_OFFSET, 239, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_STAT_AE_INITIAL_WINDOW_XSTART_OFFSET, 2, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_STAT_AE_INITIAL_WINDOW_YSTART_OFFSET, 2, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_STAT_AE_INITIAL_WINDOW_XEND_OFFSET, 65, 2);
    writeVar(MT9V117_CAM_CTRL_VAR, MT9V117_CAM_STAT_AE_INITIAL_WINDOW_YEND_OFFSET, 49, 2);
}
