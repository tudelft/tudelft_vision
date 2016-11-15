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

#ifndef VISION_IMAGE_BUFFER_H_
#define VISION_IMAGE_BUFFER_H_

#include <tuv/vision/image.h>
#include <vector>

/**
 * @brief Image based on a buffer
 *
 * This is an image based on a memory buffer. This buffer is automatically generated when constructing
 * an object of this ImageBuffer class.
 */
class ImageBuffer: public Image {
  public:
    ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height);
    ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height, uint32_t size);
    ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height, std::vector<uint8_t> &img);
    ~ImageBuffer(void);
};

#endif /* VISION_IMAGE_BUFFER_H_ */
