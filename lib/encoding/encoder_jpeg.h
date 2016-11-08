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

#include <cstddef>
#include <cstdint>
#include <stdio.h>
#include <vector>
#include <jpeglib.h>

#ifndef ENCODING_ENCODER_JPEG_H_
#define ENCODING_ENCODER_JPEG_H_

/**
 * @brief JPEG encoder based on libjpeg
 *
 * This is a simple JPEG encoder and uses the libjpeg for encoding. It needs an YUV422 image as input
 * and will convert this into a JPEG buffered image. This encoder will create a new image based on a buffer.
 */
class EncoderJPEG {
  private:
    /* New jpeg destination memory */
    typedef struct _jpeg_destination_mem_mgr {
        jpeg_destination_mgr mgr;   ///< Manager which holds the function points
        std::vector<uint8_t> data;  ///< The byte data
    } jpeg_destination_mem_mgr;

    struct jpeg_compress_struct cinfo;      ///< Compression information
    struct jpeg_error_mgr jerr;             ///< Error function manager
    _jpeg_destination_mem_mgr dmgr;         ///< Destination manager
    uint8_t quality;                        ///< The output quality of the JPEG encoding

    static void initDestination(j_compress_ptr cinfo);
    static boolean emptyOutputBuffer(j_compress_ptr cinfo);
    static void termDestination(j_compress_ptr cinfo);

  public:
    EncoderJPEG(uint8_t quality = 80);

    Image::Ptr encode(Image::Ptr img);
    void setQuality(uint8_t quality);
    uint8_t getQuality(void);
};

#endif /* ENCODING_ENCODER_JPEG_H_ */
