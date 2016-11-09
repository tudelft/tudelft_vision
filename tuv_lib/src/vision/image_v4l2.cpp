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

#include "vision/image_v4l2.h"


/**
 * @brief Create a new V4L2 image
 *
 * This is a specific image for the V4L2 camera. It will automatically dequeue the buffer
 * with the V4L2 camera when the image is destroyed.
 * @param[in] *cam The V4L2 camera which took the image
 * @param[in] buffer_index The index of the V4L2 camera
 * @param[in] *data Pointer to the data buffer
 */
ImageV4L2::ImageV4L2(CamLinux *cam, uint16_t buffer_index, void *data) {
    this->pixel_format = cam->getFormat();
    this->width = cam->getWidth();
    this->height = cam->getHeight();
    this->data = data;

    this->cam = cam;
    this->buffer_index = buffer_index;
}

/**
 * @brief Destroy the image
 *
 * This will dequeue the buffer with the V4L2 camera.
 */
ImageV4L2::~ImageV4L2(void) {
    this->cam->enqueueBuffer(buffer_index);
}
