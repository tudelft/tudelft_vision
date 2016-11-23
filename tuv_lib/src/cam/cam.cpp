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

#include "cam/cam.h"

/**
 * @brief Get the camera output width
 *
 * This returns the camera output width in pixels.
 * @return The camera output width in pixels
 */
unsigned int Cam::getWidth(void) {
    return width;
}

/**
 * @brief Get the camera output height
 *
 * This returns the camera output height in pixels.
 * @return The camera output height in pixels
 */
unsigned int Cam::getHeight(void) {
    return height;
}

/**
 * @brief Get the camera output format
 *
 * This will return the camera output format. An example of such a format is YUYV, UYVY etc.
 * @return The camera output format (pixel information)
 */
enum Image::pixel_formats Cam::getFormat(void) {
    return pixel_format;
}
