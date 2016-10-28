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

#include "image.h"

#include <stdexcept>

/**
 * Get the size in bytes of a single pixel
 * @return The size of one pixel
 */
uint16_t Image::getPixelSize(void) {
  switch(pixel_format) {
    case FMT_UYVY:
    case FMT_YUYV:
      return (sizeof(uint8_t) * 2);

    default:
      throw std::runtime_error("Unknown pixel format " + std::to_string(pixel_format));
  }
}

uint32_t Image::getSize(void) {
    return (getPixelSize() * width * height);
}
