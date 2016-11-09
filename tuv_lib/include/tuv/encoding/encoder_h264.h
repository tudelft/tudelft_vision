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

#ifndef ENCODING_ENCODER_H264_H_
#define ENCODING_ENCODER_H264_H_

#include <stdint.h>
#include <vector>
#include <tuv/encoding/h264/h264encapi.h>
#include <tuv/encoding/h264/ewl.h>
#include <tuv/vision/image.h>

/**
 * @brief H264 image encoder
 *
 * This is an H264 image encoder based on the hantro h264 encoder. This is currently only implemented for the
 * Bebop and the Bebop 2.
 */
class EncoderH264 {
  private:

    /** H264 Frame types */
    enum frame_type_t {
        FRAME_INVALID,      ///< Invalid frame
        FRAME_I,            ///< Intra frame
        FRAME_P             ///< Prediction frame
    };

    /** H264 buffer statusses */
    enum buffer_status_t {
        BUF_FREE,           ///< Buffer is free
        BUF_LOCKED,         ///< Buffer is locked by the user
        BUF_READY,          ///< Buffer is ready for use
        BUF_TOBE_RELEASED   ///< Buffer is going to be released
    };

    /** H264 input and output buffer description */
    struct buffer_t {
        enum buffer_status_t status;    ///< Buffer status
        uint32_t size;                  ///< Size of the buffer
        enum frame_type_t frame_type;   ///< Type of frame
        EWLLinearMem_t venc_mem;        ///< Video encoder memory
        uint32_t frame_idx;             ///< Frame index
    };

    uint32_t width;         ///< The output width in pixels
    uint32_t height;        ///< The output height in pixels
    uint32_t frame_rate;    ///< The output frame rate in FPS
    uint32_t bit_rate;      ///< The output bit rate (bits per second)
    uint32_t intra_rate;    ///< The output intra frame rate (everey intra_rate a intra frame is generated)

    H264EncInst encoder;        ///< The H264 encoder instance
    H264EncIn encoder_input;    ///< The H264 encoder input
    H264EncOut encoder_output;  ///< The H264 encoder output
    std::vector<struct buffer_t> input_buffers;     ///< The input buffers
    std::vector<struct buffer_t> output_buffers;    ///< The output buffers

    EWLLinearMem_t start_buffer;
    uint32_t start_buffer_size;

    uint32_t input_frame_cnt;
    uint32_t output_frame_cnt;
    uint32_t encoder_frame_cnt;
    uint32_t intra_frame_cnt;

  public:
    EncoderH264();

    Image::Ptr encode(Image::Ptr img);
};

#endif /* ENCODING_ENCODER_H264_H_ */
