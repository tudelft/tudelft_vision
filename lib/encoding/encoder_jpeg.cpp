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

#include "encoder_jpeg.h"
#include "vision/image_buffer.h"

#define BLOCK_SIZE 16384

void EncoderJPEG::initDestination(j_compress_ptr cinfo) {
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    dst->data.resize(BLOCK_SIZE);
    cinfo->dest->next_output_byte = &dst->data[0];
    cinfo->dest->free_in_buffer = dst->data.size();
}

boolean EncoderJPEG::emptyOutputBuffer(j_compress_ptr cinfo)
{
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    size_t oldsize = dst->data.size();
    dst->data.resize(oldsize + BLOCK_SIZE);
    cinfo->dest->next_output_byte = &dst->data[oldsize];
    cinfo->dest->free_in_buffer = dst->data.size() - oldsize;
    return true;
}

void EncoderJPEG::termDestination(j_compress_ptr cinfo)
{
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    dst->data.resize(dst->data.size() - cinfo->dest->free_in_buffer);
}

/**
 * @brief Initialize the JPEG encoder
 * This sets up the output buffer and error handling ot the libjpeg encoder.
 */
EncoderJPEG::EncoderJPEG(void) {
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    cinfo.dest = (jpeg_destination_mgr*)&dmgr;
    cinfo.dest->init_destination = &initDestination;
    cinfo.dest->empty_output_buffer = &emptyOutputBuffer;
    cinfo.dest->term_destination = &termDestination;
}

/**
 * @brief Encode and image using JPEG compression
 * @param img The image to encode
 * @return The compressed output image
 */
std::shared_ptr<Image> EncoderJPEG::encode(std::shared_ptr<Image> img) {
    uint8_t *img_buf = (uint8_t *)img->getData();
    cinfo.image_width = img->getWidth();
    cinfo.image_height = img->getHeight();
    cinfo.input_components= 3;
    cinfo.in_color_space = JCS_YCbCr;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality (&cinfo, 75, true);
    jpeg_start_compress(&cinfo, true);


    std::vector<uint8_t> tmprowbuf(img->getWidth() * 3);
    while (cinfo.next_scanline < cinfo.image_height) {
        uint32_t i, j;
        uint32_t offset = cinfo.next_scanline * cinfo.image_width * 2; //offset to the correct row
        for (i = 0, j = 0; i < cinfo.image_width * 2; i += 4, j += 6) { //input strides by 4 bytes, output strides by 6 (2 pixels)
            tmprowbuf[j + 0] = img_buf[offset + i + 0]; // Y (unique to this pixel)
            tmprowbuf[j + 1] = img_buf[offset + i + 1]; // U (shared between pixels)
            tmprowbuf[j + 2] = img_buf[offset + i + 3]; // V (shared between pixels)
            tmprowbuf[j + 3] = img_buf[offset + i + 2]; // Y (unique to this pixel)
            tmprowbuf[j + 4] = img_buf[offset + i + 1]; // U (shared between pixels)
            tmprowbuf[j + 5] = img_buf[offset + i + 3]; // V (shared between pixels)
        }

        JSAMPROW row_pointer = (JSAMPROW)tmprowbuf.data();
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    return std::make_shared<ImageBuffer>(Image::FMT_JPEG, img->getWidth(), img->getHeight(), dmgr.data);
}
