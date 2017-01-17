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

#include "cam/cam_bebop_bottom.h"

#include "drivers/mt9v117.h"
#include <linux/v4l2-mediabus.h>

/**
 * @brief Initialize the Bebop bottom camera
 *
 * This will setup the Linux camera, I2C-bus communication and MT9V117 driver.
 */
CamBebopBottom::CamBebopBottom(void) : CamLinux("/dev/video0"),
    i2c_bus("/dev/i2c-0"),
    mt9v117(&i2c_bus) {

}

/**
 * @brief Set the camera resolution
 *
 * Note that this is a request and sometimes the resolution will be different than
 * the requested resolution because it is linked to the pixel format.
 * For the Bebop Camera this function is hooked to initialize the subdevice.
 * @param[in] format The requested video format
 * @param[in] width The requested output width
 * @param[in] height The requested output height
 */
void CamBebopBottom::setOutput(enum Image::pixel_formats format, uint32_t width, uint32_t height) {
    // Initialize the subdevice
    initSubdevice("/dev/v4l-subdev0", 0, V4L2_MBUS_FMT_UYVY8_2X8, width, height);

    // Call the super function
    CamLinux::setOutput(format, width, height);
}
