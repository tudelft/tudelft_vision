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

#include "cam_linux.h"
#include "drivers/i2cbus.h"
#include "drivers/mt9f002.h"
#include "drivers/isp.h"

#ifndef CAM_BEBOP_H_
#define CAM_BEBOP_H_

class CamBebop: public CamLinux {
  private:
    I2CBus i2c_bus;
    struct MT9F002::pll_config_t pll_config;
    MT9F002 mt9f002;
    ISP isp;

  public:
    CamBebop(void);

    void start(void); ///< Start the camera streaming
    void setOutput(enum Image::pixel_formats format, uint32_t width, uint32_t height);  ///< Set output format
    void setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height);
};

#endif /* CAM_BEBOP_H_ */
