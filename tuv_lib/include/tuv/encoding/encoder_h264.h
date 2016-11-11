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
  public:
    /** Possible rotation options */
    enum rotation_t {
        ROTATE_0,       ///< Don't rotate the input image
        ROTATE_90R,     ///< Rotate the image 90 degrees right
        ROTATE_90L      ///< Rotate the image 90 degrees left
    };

  private:
    /** Input settings */
    struct input_cfg_t {
        Image::pixel_formats format;    ///< Input image pixel format
        uint32_t width;                 ///< Input image width in pixels
        uint32_t height;                ///< Input image height in pxels
        enum rotation_t rot;            ///< Input image rotation
    };
    struct input_cfg_t input_cfg;       ///< The input configuration

    /** Output settings */
    struct output_cfg_t {
        uint32_t width;                 ///< Output image width in pixels
        uint32_t height;                ///< Output image height in pixesl
        float frame_rate;               ///< Output frame rate in FPS
        uint32_t bit_rate;              ///< Target bit rate in bits/second [10000..60000000]
    };
    struct output_cfg_t output_cfg;     ///< The output configuration

    /* H264 contol and configuration */
    H264EncInst encoder;                ///< The H264 encoder instance
    H264EncConfig cfg;                  ///< The H264 encoder configuration
    H264EncRateCtrl rcCfg;              ///< The H264 encoder rate configuration
    H264EncCodingCtrl codingCfg;        ///< The H264 encoder coding configuration
    H264EncPreProcessingCfg preProcCfg; ///< The H264 encoder pre processing configuration

    /* Encoding information */
    H264EncIn encoder_input;            ///< The H264 encoder input
    H264EncOut encoder_output;          ///< The H264 encoder output
    EWLLinearMem_t sps_pps_nalu;        ///< SPS + PPS NALU buffer

    uint32_t frame_cnt;                 ///< Frame counter
    uint32_t intra_cnt;                 ///< Intra frame counter for determining when intra frame must be generated

    /* Initialization functions */
    void openEncoder(void);
    void closeEncoder(void);
    void configureRate(void);
    void configureCoding(void);
    void configurePreProcessing(void);

    /* Helper functions */
    H264EncPictureType getEncPictureType(Image::pixel_formats format);
    H264EncPictureRotation getEncPictureRotation(enum rotation_t rot);

  public:
    EncoderH264(uint32_t width, uint32_t height);
    EncoderH264(uint32_t width, uint32_t height, float frame_rate);
    EncoderH264(uint32_t width, uint32_t height, float frame_rate, uint32_t bit_rate);
    ~EncoderH264(void);

    void setInput(Image::pixel_formats format, uint32_t width, uint32_t height);
    void setInput(Image::pixel_formats format, uint32_t width, uint32_t height, enum rotation_t rot);
    Image::Ptr encode(Image::Ptr img);
};

#endif /* ENCODING_ENCODER_H264_H_ */
