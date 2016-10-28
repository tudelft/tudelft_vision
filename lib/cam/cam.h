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
#include <memory>

#ifndef CAM_CAM_H_
#define CAM_CAM_H_

class Cam {
  protected:
    unsigned int width, height;	              ///< The output width and height
    enum Image::pixel_formats pixel_format;   ///< The output pixel format

  public:
    virtual void start(void) = 0;
    virtual void stop(void) = 0;
    virtual std::shared_ptr<Image> getImage(void) = 0;

    /* Settings */
    void setOutput(enum Image::pixel_formats format, uint32_t width, uint32_t height);
    void setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height);

    /* Usefull getters */
    unsigned int getWidth(void) { return width; };
    unsigned int getHeight(void) { return height; };
    enum Image::pixel_formats getFormat(void) { return pixel_format; };
};

#endif /* CAM_CAM_H_ */
