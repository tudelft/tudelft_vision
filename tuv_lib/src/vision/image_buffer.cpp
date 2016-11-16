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

#include "vision/image_buffer.h"

#include <stdlib.h>
#include <memory.h>

/**
 * @brief Create a new image buffer
 *
 * This will create a new image buffer based on the pixel format supplied and the image width
 * and height.
 * @param[in] pixel_format The pixel format for the image
 * @param[in] width The width in pixels
 * @param[in] height The height in pixels
 */
ImageBuffer::ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height):
    Image(pixel_format, width, height) {
    this->data = malloc(this->size);
    this->size = (getPixelSize() * width * height);
}

/**
 * @brief Create a new image buffer
 *
 * This will create a new image with a specific buffer size.
 * @param[in] pixel_format The pixel format for the image
 * @param[in] width The width in pixels
 * @param[in] height The height in pixels
 * @param[in] size The buffer size in bytes
 */
ImageBuffer::ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height, uint32_t size):
    Image(pixel_format, width, height, size) {
    this->data = malloc(this->size);
}

/**
 * @brief Create a new image buffer
 *
 * This will create a new image pased on a byte vector.
 * @param[in] pixel_format The pixel format for the image
 * @param[in] width The width in pixels
 * @param[in] height The height in pixels
 * @param[in] img The input byte buffer
 */
ImageBuffer::ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height, std::vector<uint8_t> &img):
    Image(pixel_format, width, height, img.size()) {
    this->data = malloc(this->size);
    memcpy(this->data, img.data(), this->size);
}

/**
 * @brief Destroy the image
 *
 * This will free the allocated memory of the buffer.
 */
ImageBuffer::~ImageBuffer(void) {
    free(this->data);
}
