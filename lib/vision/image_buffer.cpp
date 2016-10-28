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

#include "image_buffer.h"

#include <stdlib.h>
#include <memory.h>

/**
 * Create a new image buffer
 * @param[in] pixel_format The pixel format the image is in
 * @param[in] width The width in pixels
 * @param[in] height The height in pixels
 */
ImageBuffer::ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height) {
  this->pixel_format = pixel_format;
  this->width = width;
  this->height = height;
  this->size = Image::getSize();
  this->data = malloc(this->size);
}

ImageBuffer::ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height, uint32_t size) {
    this->pixel_format = pixel_format;
    this->width = width;
    this->height = height;
    this->size = size;
    this->data = malloc(this->size);
}

ImageBuffer::ImageBuffer(enum pixel_formats pixel_format, uint32_t width, uint32_t height, std::vector<uint8_t> img) {
    this->pixel_format = pixel_format;
    this->width = width;
    this->height = height;
    this->size = img.size();
    this->data = malloc(this->size);
    memcpy(this->data, img.data(), this->size);
}

uint32_t ImageBuffer::getSize(void) {
    return this->size;
}

/**
 * Destroy the image
 */
ImageBuffer::~ImageBuffer(void) {
  free(this->data);
}
