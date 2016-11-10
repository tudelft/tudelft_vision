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

#ifndef CAM_BEBOP_H_
#define CAM_BEBOP_H_

#include <tuv/cam/cam_linux.h>
#include <tuv/drivers/i2cbus.h>
#include <tuv/drivers/mt9f002.h>
#include <tuv/drivers/isp.h>
#include <vector>

/**
 * @brief Bebop Front Camera
 *
 * This is a full driver for the Bebop front camera. It is baed on the Linux camera and extends it
 * by configurations for the MT9F002 CMOS chipset and ISP.
 */
class CamBebop: public CamLinux {
  private:
    I2CBus i2c_bus;                             ///< The I2C bus connection on which the MT9F002 is connected
    struct MT9F002::pll_config_t pll_config;    ///< PLL configuration for the MT9F002
    MT9F002 mt9f002;                            ///< MT9F002 driver
    ISP isp;                                    ///< ISP driver

  public:
    CamBebop(void);

    void start(void);
    void setOutput(enum Image::pixel_formats format, uint32_t width, uint32_t height);
    void setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height);
};

#endif /* CAM_BEBOP_H_ */
