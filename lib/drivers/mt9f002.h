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

/**
 * Driver for the MT9F002 CMOS Chipset
 */
class MT9F002 {
  public:
    /* Interface types for the MT9F002 connection */
    enum interfaces {
      MIPI,     ///< MIPI type connection
      HiSPi,    ///< HiSPi type connection
      PARALLEL  ///< Parallel type connection
    };

    /* Clock configuration */
    struct pll_config_t {
      float input_clk_freq;               ///< Input clock frequency
      uint16_t vt_pix_clk_div;            ///< Fixed PLL config from calculator tool
      uint16_t vt_sys_clk_div;            ///< Fixed PLL config from calculator tool
      uint16_t pre_pll_clk_div;           ///< Fixed PLL config from calculator tool
      uint16_t pll_multiplier;            ///< Fixed PLL config from calculator tool
      uint16_t op_pix_clk_div;            ///< Fixed PLL config from calculator tool
      uint16_t op_sys_clk_div;            ///< Fixed PLL config from calculator tool
      uint8_t shift_vt_pix_clk_div;       ///< Fixed PLL config from calculator tool
      uint8_t rowSpeed_2_0;               ///< Fixed PLL config from calculator tool
      uint8_t row_speed_10_8;             ///< Fixed PLL config from calculator tool
    };

    /* Resolution configuration */
    struct res_config_t {
      uint16_t offset_x;                  ///< Offset from left in pixels
      uint16_t offset_y;                  ///< Offset from top in pixels
      uint16_t output_width;              ///< Output width
      uint16_t output_height;             ///< Output height
      float output_scaler;                ///< Output scaler
      uint16_t sensor_width;              ///< Sensor width
      uint16_t sensor_height;             ///< Sensor height
    };

    /* Blanking configuration */
    struct blank_config_t {
      uint16_t min_line_blanking_pck;
      uint16_t min_line_length_pck;
      uint16_t min_line_fifo_pck;
      uint16_t fine_integration_time_min;
      uint16_t fine_integration_time_max_margin;
    };

    /* Gain configuration */
    struct gain_config_t {
      float red;         ///< The red color gain
      float green1;      ///< The green1 color gain
      float blue;        ///< The blue color gain
      float green2;      ///< The green2 color gain
    };

  private:
    I2CBus *i2c_bus;            ///< The i2c bus the MT9F002 is connected to
    enum interfaces interface;  ///< Interface used to connect the MT9F002

    struct pll_config_t pll_config;     ///< The PLL onfiguration
    float vt_pix_clk;                   ///< Calculated based on PLL
    float op_pix_clk;                   ///< Calculated based on PLL
    uint16_t line_length;               ///< Calculated line length of blanking
    uint16_t frame_length;              ///< Calculated frame length of blanking

    struct res_config_t res_config;     ///< Resolution configuration
    uint16_t scaled_width;              ///< Width after corrected scaling
    uint16_t scaled_height;             ///< Height after corrected scaling

    struct blank_config_t blank_config; ///< Blanking configuration
    struct gain_config_t gain_config;   ///< Color gains configuration

    float target_fps;       ///< The target FPS
    float real_fps;         ///< The real calculated FPS

    float target_exposure;  ///< The target exposure in ms
    float real_exposure;    ///< The real calculated exposure in ms

    /* Internal functions */
    int gcd(int a, int b);  ///< Calculate the greatest common divider
    void writeRegister(uint16_t address, uint32_t value, uint8_t length); ///< Write to a register
    uint32_t readRegister(uint16_t address, uint8_t length);              ///< Read from a register
    uint16_t calculateGain(float gain);                                   ///< Calculate color gain register

    void mipiHispiStage1(void);   ///< MIPI initialization stage 1
    void mipiHispiStage2(void);   ///< MIPI initialization stage 2
    void mipiHispiStage3(void);   ///< MIPI initialization stage 3
    void parallelStage1(void);    ///< Parallel initialization stage 1
    void parallelStage2(void);    ///< Parallel initialization stage 2

    void writePll(void);          ///< Write the pll values to the registers
    void writeResolution(void);   ///< Write the resolution information to the registers
    void writeBlanking(void);     ///< Write the blanking information to the registers
    void writeExposure(void);     ///< Write the exposure information to the registers
    void writeGains(void);        ///< Write the gains information to the registers

  public:
    MT9F002(I2CBus *i2c_bus, enum interfaces interface, struct pll_config_t pll_config);

    /* Set size, crop and binning */
    void setOutput(uint16_t width, uint16_t height);
    void setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height);

    /* FPS settings */
    float getFPS(void);                   ///< Get the real FPS
    float getTargetFPS(void);             ///< Get the target FPS
    void setFPS(float fps);               ///< Set the target FPS

    /* Exposure settings */
    float getMaxExposure(void);           ///< Get the maximum exposure in ms
    float getExposure(void);              ///< Get the real exposure in ms
    float getTargetExposure(void);        ///< Get the target expusre in ms
    void setExposure(float exposure);     ///< Set the target exposure in ms

    /* Gain settings */
    struct gain_config_t getGains(void);         ///< Get the current color gains
    void setGains(struct gain_config_t gains);   ///< Set the color gains
};
