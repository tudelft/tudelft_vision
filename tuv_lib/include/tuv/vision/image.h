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

#ifndef VISION_IMAGE_H_
#define VISION_IMAGE_H_

#include <stdint.h>
#include <memory>

/**
 * @brief Abstract image definition
 *
 * This represents an image with a specific pixel format. This is an abstract
 * class and can not be initialized by itself, since this class can't contain
 * any data.
 */
class Image {
  public:
    /** The supported pixel formats */
    enum pixel_formats {
        FMT_UYVY,       ///< UYVY with 2 bytes per pixel
        FMT_YUYV,       ///< YUYV with 2 bytes per pixel
        FMT_JPEG,       ///< A JPEG encoded image
    };

    typedef std::shared_ptr<Image> Ptr; ///< Shared pointer representation of the image

  protected:
    uint32_t width;                     ///< The image width
    uint32_t height;                    ///< The image height
    enum pixel_formats pixel_format;	///< The image pixel format
    void *data;							///< The image data

  public:

    virtual uint16_t getPixelSize(void);
    virtual uint32_t getWidth(void);
    virtual uint32_t getHeight(void);
    virtual uint32_t getSize(void);
    virtual void *getData(void);
    virtual void downsample(uint16_t downsample);
};

#endif /* VISION_IMAGE_H_ */
