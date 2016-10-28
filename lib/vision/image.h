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

#include <stdint.h>

#ifndef VISION_IMAGE_H_
#define VISION_IMAGE_H_

class Image {
	public:
		/* The supported pixel formats */
		enum pixel_formats {
			FMT_UYVY,
            FMT_YUYV,
            FMT_JPEG,
		};

	protected:
		uint32_t width, height;						///< The image width and height
		enum pixel_formats pixel_format;	///< The image pixel format
		void *data;							          ///< The image data

	public:

        virtual uint16_t getPixelSize(void);			///< Get the size in bytes of a pixel
        virtual uint32_t getWidth(void) { return width; };
        virtual uint32_t getHeight(void) { return height; };
        virtual void *getData(void) { return data; };
        virtual uint32_t getSize(void);
};

#endif /* VISION_IMAGE_H_ */
