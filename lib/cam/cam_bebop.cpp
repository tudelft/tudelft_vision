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

#include "cam_bebop.h"

#include <linux/v4l2-mediabus.h>

/**
 * Initialize the Bebop camera
 */
CamBebop::CamBebop(void) : CamLinux("/dev/video1"),
  i2c_bus("/dev/i2c-0"),
  pll_config{(26 / 2), 7, 1, 1, 59, 8, 1, 1, 1, 1},
  mt9f002(&i2c_bus, MT9F002::PARALLEL, pll_config)
{
  printf("A\n");
}

void CamBebop::start(void) {
  CamLinux::start();
  isp.configure(fd);
}

/**
 * Set the camera resolution
 * Note that this is a request and sometimes the resolution will be different than
 * the requested resolution because it is linked to the pixel format.
 * For the Bebop Camera this function is hooked to initialize the subdevice.
 * @param[in] format The requested video format
 * @param[in] width The requested output width
 * @param[in] height The requested output height
 */
void CamBebop::setOutput(enum Image::pixel_formats format, uint32_t width, uint32_t height) {
  // Set the camera sensor size
  //mt9f002.setOutput(width, height);

  // Initialize the subdevice
  initSubdevice("/dev/v4l-subdev1", 0, V4L2_MBUS_FMT_SGRBG10_1X10, width, height);

  // Call the super function
  CamLinux::setOutput(format, width, height);
  CamLinux::setCrop(0, 0, width, height);
}

/**
 * Set the camera crop
 * @param[in] left The offset from the left in pixels
 * @param[in] top The offset from the top in pixels
 * @param[in] width The output width in pixels
 * @param[in] height The output height in pixels
 */
void CamBebop::setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height) {

}
