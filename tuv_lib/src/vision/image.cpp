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

#include "vision/image.h"

#include <string>
#include <stdexcept>
#include <assert.h>

/**
 * @brief Get the size in bytes of a single pixel
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

/**
 * @brief Get the image width
 *
 * This will return the image width in pixels.
 * @return The image width in pixels
 */
uint32_t Image::getWidth(void) {
    return width;
}

/**
 * @brief Get the image height
 *
 * This will return the image height in pixels.
 * @return The image height in pixels
 */
uint32_t Image::getHeight(void) {
    return height;
}

/**
 * @brief Get the amount of bytes
 *
 * This will return the full image size in bytes and is calculated based
 * on the pixel format.
 * @return The full image size in bytes
 */
uint32_t Image::getSize(void) {
    return (getPixelSize() * width * height);
}

/**
 * @brief Return the image data
 *
 * This will return a pointer to the image byte data. Based on the image format
 * a function can interpret this data to perform operations.
 * @return Pointer to the image data
 */
void *Image::getData(void) {
    return data;
}

/**
 * @brief Downsample the image
 *
 * This will downsample the image by dividing both the width and height by the downsample
 * factor. This is only implemented for YUV 422 images and the width must be even. The
 * result is saved in the same image.
 * @param downsample The downsample factor
 */
void Image::downsample(uint16_t downsample) {
    assert(pixel_format == FMT_UYVY || pixel_format == FMT_YUYV);
    assert(width%2 == 0);
    assert(downsample > 1);

    uint8_t *src = (uint8_t *)data;
    uint8_t *dst = (uint8_t *)data;
    uint32_t new_width = width / downsample;
    uint32_t new_height = height / downsample;
    uint32_t pixel_skip = (downsample - 1) * 2;

    for(uint32_t y = 0; y < new_height; y++) {
        for(uint32_t x = 0; x < new_width; x += 2) {
            *dst++ = *src++;
            *dst++ = *src++;
            *dst++ = *src++;
            src += pixel_skip;
            *dst++ = *src++;
            src += pixel_skip;
        }
        src += pixel_skip * width;
    }

    width = new_width;
    height = new_height;
}
