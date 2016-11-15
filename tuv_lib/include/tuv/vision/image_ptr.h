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

#ifndef VISION_IMAGE_PTR_H_
#define VISION_IMAGE_PTR_H_

#include <tuv/vision/image.h>

/**
 * @brief Image based on a pointer
 *
 * This is an image based on a pointer to a buffer managed by a device or other program. These
 * images should only be created by the apropriate classes like V4L2, H264 encoder, etc.
 * Upon deletion of these images they will be automatically freed by their appriate handler
 */
class ImagePtr: public Image {
  public:
    /**
     * @brief Handler specification for image pointer
     *
     * This handler is an abstraction for the class handling the freeing for the
     * image buffer pointer. This is typically done by the owner class.
     */
    class Handler {
      public:
        virtual void freeImage(uint16_t identifier) = 0;
    };

  private:
    Handler *handler;       ///< Pointer to the image pointer handler (owner)
    uint16_t identifier;    ///< Identifier for the handler class

  public:
    ImagePtr(Handler *handler, uint32_t identifier, enum pixel_formats pixel_format, uint32_t width, uint32_t height, void *data);
    ImagePtr(Handler *handler, uint32_t identifier, enum pixel_formats pixel_format, uint32_t width, uint32_t height, void *data, uint32_t size);
    ~ImagePtr(void);
};

#endif /* VISION_IMAGE_PTR_H_ */
