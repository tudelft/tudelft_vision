/*
 * This file is part of the TUV library (https://github.com/tudelft/tudelft_vision).
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

#include "encoding/encoder_jpeg.h"

#include "vision/image_buffer.h"
#include <assert.h>

#define BLOCK_SIZE 16384    ///< Default memory size

/**
 * @brief Called to initialize the buffer
 *
 * This resizes the output buffer to the BLOCK_SIZE and sets the next byte to
 * the first byte of the data vector.
 * @param[in] cinfo The compression information
 */
void EncoderJPEG::initDestination(j_compress_ptr cinfo) {
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    dst->data.resize(BLOCK_SIZE);
    cinfo->dest->next_output_byte = &dst->data[0];
    cinfo->dest->free_in_buffer = dst->data.size();
}

/**
 * @brief Called when the output buffer is empty
 *
 * This will resize the output buffer by adding another BLOCK_SIZE amount of bytes.
 * @param[in] cinfo The compression information
 * @return If the buffer has new free bytes
 */
boolean EncoderJPEG::emptyOutputBuffer(j_compress_ptr cinfo) {
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    size_t oldsize = dst->data.size();
    dst->data.resize(oldsize + BLOCK_SIZE);
    cinfo->dest->next_output_byte = &dst->data[oldsize];
    cinfo->dest->free_in_buffer = dst->data.size() - oldsize;
    return TRUE;
}

/**
 * @brief Called when done writing to buffer
 *
 * This will resize the output buffer by removing the free bytes. This is done
 * to make sure that the size of the output buffer matches the amount of bytes
 * that are compressed.
 * @param cinfo The compression information
 */
void EncoderJPEG::termDestination(j_compress_ptr cinfo) {
    jpeg_destination_mem_mgr* dst = (jpeg_destination_mem_mgr*)cinfo->dest;
    dst->data.resize(dst->data.size() - cinfo->dest->free_in_buffer);
}

/**
 * @brief Initialize the JPEG encoder
 *
 * This sets up the output buffer and error handling ot the libjpeg encoder. The
 * default quality of the JPEG encoder is 80.
 * @param[in] quality Optionally you can set a different default quality [1-100]
 */
EncoderJPEG::EncoderJPEG(uint8_t quality) {
    assert(quality > 0);
    assert(quality <= 100);

    // Set default quality
    this->quality = quality;

    // Set up default error
    cinfo.err = jpeg_std_error(&jerr);

    // Setup destination with own memory destination functions
    jpeg_create_compress(&cinfo);
    cinfo.dest = (jpeg_destination_mgr*)&dmgr;
    cinfo.dest->init_destination = &initDestination;
    cinfo.dest->empty_output_buffer = &emptyOutputBuffer;
    cinfo.dest->term_destination = &termDestination;
}

/**
 * @brief Encode and image using JPEG compression
 *
 * This will use libjpeg to encode an image using JPEG compression.
 * @param[in] img The image to encode
 * @return The compressed output image
 */
Image::Ptr EncoderJPEG::encode(Image::Ptr img) {
    uint8_t *img_buf = (uint8_t *)img->getData();
    cinfo.image_width = img->getWidth();
    cinfo.image_height = img->getHeight();
    cinfo.input_components= 3;
    cinfo.in_color_space = JCS_YCbCr;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);


    uint8_t tmprowbuf[img->getWidth() * 3];
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

        JSAMPROW row_pointer = (JSAMPROW)tmprowbuf;
        jpeg_write_scanlines(&cinfo, &row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    return std::make_shared<ImageBuffer>(Image::FMT_JPEG, img->getWidth(), img->getHeight(), dmgr.data);
}

/**
 * @brief Set the quality of JPEG output
 *
 * This will set the quality of the JPEG output which can be set as a value
 * btween 1 and 100. Where 100 means the best quality.
 * @param quality The new quality [1-100]
 */
void EncoderJPEG::setQuality(uint8_t quality) {
    assert(quality > 0);
    assert(quality <= 100);

    this->quality = quality;
}

/**
 * @brief Get the quality of the JPEG output
 *
 * Outputs the currently set value of the JPEG output quality. The value will be
 * between 1 and 100. Where 100 means the best quality.
 * @return The currently set quality
 */
uint8_t EncoderJPEG::getQuality(void) {
    return this->quality;
}
