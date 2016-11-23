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

#ifndef DRIVERS_MT9F002_H_
#define DRIVERS_MT9F002_H_

#include <tuv/drivers/i2cbus.h>

/**
 * Driver for the MT9F002 CMOS Chipset
 */
class MT9F002 {
  public:
    /** Interface types for the MT9F002 connection */
    enum interfaces {
        MIPI,     ///< MIPI type connection
        HiSPi,    ///< HiSPi type connection
        PARALLEL  ///< Parallel type connection
    };

    /** Clock configuration */
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

    /** Resolution configuration */
    struct res_config_t {
        uint16_t offset_x;                  ///< Offset from left in pixels
        uint16_t offset_y;                  ///< Offset from top in pixels
        uint16_t output_width;              ///< Output width
        uint16_t output_height;             ///< Output height
        float output_scaler;                ///< Output scaler
        uint16_t sensor_width;              ///< Sensor width
        uint16_t sensor_height;             ///< Sensor height
        uint8_t x_odd_inc;                  ///< Skipping in x direction
        uint8_t y_odd_inc;                  ///< Skipping in y direction
    };

    /** Blanking configuration */
    struct blank_config_t {
        uint16_t min_line_blanking_pck;             ///< Minimun line blanking
        uint16_t min_line_length_pck;               ///< Minimum line length
        uint16_t min_line_fifo_pck;                 ///< Minimum line fifo
        uint16_t fine_integration_time_min;         ///< Fine integration time minimum
        uint16_t fine_integration_time_max_margin;  ///< Fine integration time maximum margin
    };

    /** Gain configuration */
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
    int gcd(int a, int b);
    void writeRegister(uint16_t address, uint32_t value, uint8_t length);
    uint32_t readRegister(uint16_t address, uint8_t length);
    uint16_t calculateGain(float gain);
    void calculateResolution(void);

    void mipiHispiStage1(void);
    void mipiHispiStage2(void);
    void mipiHispiStage3(void);
    void parallelStage1(void);
    void parallelStage2(void);

    void writePll(void);
    void writeResolution(void);
    void writeBlanking(void);
    void writeExposure(void);
    void writeGains(void);

  public:
    MT9F002(I2CBus *i2c_bus, enum interfaces interface, struct pll_config_t pll_config);

    /* Set size, crop and binning */
    void setOutput(uint16_t width, uint16_t height);
    void setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height);

    /* FPS settings */
    float getFPS(void);
    float getTargetFPS(void);
    void setFPS(float fps);

    /* Exposure settings */
    float getExposure(void);
    float getTargetExposure(void);
    void setExposure(float exposure);

    /* Gain settings */
    struct gain_config_t getGains(void);
    void setGains(struct gain_config_t gains);
};

#endif /* DRIVERS_MT9F002_H_ */
