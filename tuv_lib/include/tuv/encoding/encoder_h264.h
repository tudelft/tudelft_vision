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

#ifndef ENCODING_ENCODER_H264_H_
#define ENCODING_ENCODER_H264_H_

#include <stdint.h>
#include <vector>
#include <tuv/encoding/h264/h264encapi.h>
#include <tuv/encoding/h264/ewl.h>
#include <tuv/vision/image.h>
#include <tuv/vision/image_ptr.h>
#include <tuv/cam/cam.h>

#define BEBOP_EWL_OFFSET 0x658 ///< Bebop offset from encoder to EWL instance

/**
 * @brief H264 image encoder
 *
 * This is an H264 image encoder based on the hantro h264 encoder. This is currently only implemented for the
 * Bebop and the Bebop 2.
 */
class EncoderH264: public ImagePtr::Handler {
  public:
    /** Possible rotation options */
    enum rotation_t {
        ROTATE_0,       ///< Don't rotate the input image
        ROTATE_90R,     ///< Rotate the image 90 degrees right
        ROTATE_90L      ///< Rotate the image 90 degrees left
    };

  private:
    /** Output buffer */
    struct output_buf_t {
        uint16_t index;                 ///< Index of the buffer (identification for freeing)
        EWLLinearMem_t mem;             ///< EWL memory information
        bool is_free;                   ///< Whether the buffer is free
    };

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
    uint32_t frame_cnt;                 ///< Frame counter
    uint32_t intra_cnt;                 ///< Intra frame counter for determining when intra frame must be generated
    std::vector<uint8_t> sps_nalu;      ///< SPS NALU
    std::vector<uint8_t> pps_nalu;      ///< PPS NALU
    EWLLinearMem_t sps_pps_nalu;        ///< SPS + PPS NALU buffer
    std::vector<struct output_buf_t> output_buffers;    ///< Output buffers

    /* Initialization functions */
    void openEncoder(void);
    void closeEncoder(void);
    void configureRate(void);
    void configureCoding(void);
    void configurePreProcessing(void);
    void streamStart(void);

    /* Helper functions */
    H264EncPictureType getEncPictureType(Image::pixel_formats format);
    H264EncPictureRotation getEncPictureRotation(enum rotation_t rot);
    struct output_buf_t *getFreeBuffer(void);

  public:
    EncoderH264(uint32_t width, uint32_t height, float frame_rate = 15, uint32_t bit_rate = 2000000);
    ~EncoderH264(void);

    /* Input settings */
    void setInput(Cam::Ptr cam, enum rotation_t rot = ROTATE_0);
    void setInput(Image::pixel_formats format, uint32_t width, uint32_t height, enum rotation_t rot = ROTATE_0);

    /* Encoding functions */
    void start(void);
    Image::Ptr encode(Image::Ptr img);
    void freeImage(uint16_t identifier);
    std::vector<uint8_t> getSPS(void);
    std::vector<uint8_t> getPPS(void);

//   uint32_t getWidth(void);
//   uint32_t getHeight(void);
};

#endif /* ENCODING_ENCODER_H264_H_ */
