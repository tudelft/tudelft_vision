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

#include "vision/image_ptr.h"

/**
 * @brief Create a new image pointer
 *
 * This will create a new image based on a data pointer which is managed by the handler. The
 * handler is the owner of the data pointer.
 * The size of the image is determined by the pixel format.
 * @param[in] *handler The image handler instance (owner)
 * @param[in] identifier The identifier for the image handler
 * @param[in] pixel_format The pixel format (order of channels per pixel)
 * @param[in] width The image pixel width
 * @param[in] height The image pixel height
 * @param[in] *data Pointer to the data buffer
 */
ImagePtr::ImagePtr(Handler *handler, uint16_t identifier, enum pixel_formats pixel_format, uint32_t width, uint32_t height, void *data):
    Image(pixel_format, width, height),
    handler(handler),
    identifier(identifier) {
    this->data = data;
    this->size = (getPixelSize() * width * height);
}

/**
 * @brief Create a new image pointer
 *
 * This will create a new image based on a data pointer which is managed by the handler. The
 * handler is the owner of the data pointer.
 * This should only be used if the size of the image can't be determined by the pixel format.
 * @param[in] *handler The image handler instance (owner)
 * @param[in] identifier The identifier for the image handler
 * @param[in] pixel_format The pixel format (order of channels per pixel)
 * @param[in] width The image pixel width
 * @param[in] height The image pixel height
 * @param[in] *data Pointer to the data buffer
 * @param[in] size The image size in bytes
 */
ImagePtr::ImagePtr(Handler *handler, uint16_t identifier, enum pixel_formats pixel_format, uint32_t width, uint32_t height, void *data, uint32_t size):
    Image(pixel_format, width, height, size),
    handler(handler),
    identifier(identifier) {
    this->data = data;
}

/**
 * @brief Destroy the image
 *
 * This will dequeue the buffer with the V4L2 camera.
 */
ImagePtr::~ImagePtr(void) {
    this->handler->freeImage(identifier);
}
