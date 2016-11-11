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

#include "encoding/encoder_h264.h"

#include "drivers/clogger.h"
#include "encoding/h264/h264encapi.h"
#include <stdexcept>
#include <assert.h>

/**
 * @brief Generate a new H264 encoder
 *
 * This will generate a new Hantro H264 hardware encoder. The output frame rate will be set to 30 FPS
 * and the target output bit rate will be set to 1Mbps
 * @param width The output width in pixels (must be multiple of 4)
 * @param height The output height in pixels (must be multiple of 2)
 */
EncoderH264::EncoderH264(uint32_t width, uint32_t height) {
    assert(width % 4);
    assert(height % 2);

    // Set the output settings
    output_cfg.width = width;
    output_cfg.height = height;
    output_cfg.frame_rate = 30;     // 30 FPS
    output_cfg.bit_rate = 1000000;  // 1Mbps

    // Open the encoder
    openEncoder();
}

/**
 * @brief Open the Hantro encoder
 *
 * This will create a new Hantro encoder instance
 */
void EncoderH264::openEncoder(void) {
    assert(output_cfg.width % 4);
    assert(output_cfg.height % 2);

    /* Check if we have an encoder and get the version */
    H264EncBuild encBuild = H264EncGetBuild();
    if (encBuild.hwBuild == EWL_ERROR){
        std::runtime_error("Could not find the Hantro H264 encoder");
    }
    CLOGGER_INFO("Found hantro encoder (HW: " << encBuild.hwBuild << " SW: " << encBuild.swBuild << ")");

    /* Set encoder configuration */
    cfg.width = output_cfg.width;
    cfg.height = output_cfg.height;

    cfg.frameRateDenom = 1;
    cfg.frameRateNum = output_cfg.frame_rate;

    cfg.streamType = H264ENC_BYTE_STREAM;
    cfg.level = H264ENC_LEVEL_4; // Level 4 minimum for 1080p
    cfg.viewMode = H264ENC_BASE_VIEW_DOUBLE_BUFFER;
    cfg.scaledWidth = 0;    // Disable scaled output
    cfg.scaledHeight = 0;   // Disable scaled output

    /* Initialize an encoder with the configuration */
    if(H264EncInit(&cfg, &encoder) != H264ENC_OK) {
        std::runtime_error("Could not initialize the Hantro H264 encoder");
    }

    CLOGGER_INFO("Created new Hantro encoder instance");
}

/**
 * @brief Close the Hantro encoder
 *
 * This will try to close the encoder instance of the Hantro encoder.
 */
void EncoderH264::closeEncoder(void) {
    if(H264EncRelease(encoder) != H264ENC_OK) {
        CLOGGER_WARN("Could not release Hantro encoder");
    }
}

/**
 * @brief Configure the rate controller configuration
 *
 * This will set the following settings:
 * - Target bit rate (bits/second)
 * - GOP length (same as intra rate, which is set at FPS)
 * - Hypothetical Reference Decoder model (disabled)
 */
void EncoderH264::configureRate(void) {
    /* Get the current rate control configuration */
    if(H264EncGetRateCtrl(encoder, &rcCfg) != H264ENC_OK) {
        std::runtime_error("Failed to get the rate control information for the Hantro H264");
    }

    CLOGGER_DEBUG("Rate control QP: " << rcCfg.qpHdr << " [" << rcCfg.qpMin << ", " << rcCfg.qpMax << "]");
    CLOGGER_DEBUG("Rate control Bitrate: " << rcCfg.bitPerSecond << " bps");
    CLOGGER_DEBUG("Rate control Picture based: " << rcCfg.pictureRc << "  MB based:" << rcCfg.mbRc);
    CLOGGER_DEBUG("Rate control Skip: " << rcCfg.pictureSkip);
    CLOGGER_DEBUG("Rate control HRD: " << rcCfg.hrd << "  HRD CPB: " << rcCfg.hrdCpbSize);
    CLOGGER_DEBUG("Rate control GOP length: " << rcCfg.gopLen);

    /* Update the rate control configuration */
    rcCfg.bitPerSecond = output_cfg.bit_rate;
    rcCfg.gopLen = output_cfg.frame_rate; // Advised is intra rate (FPS)
    rcCfg.hrd = 0; // Disable Hrd conformance
    if(H264EncSetRateCtrl(encoder, &rcCfg) != H264ENC_OK) {
        std::runtime_error("Failed to set the rate control information for the Hantro H264");
    }
}

/**
 * @brief Configure the coding controller
 *
 * Here nothing is adjusted, but only all settings are read for debugging information.
 */
void EncoderH264::configureCoding(void) {
    /* Get the current conding control configuration */
    if(H264EncGetCodingCtrl(encoder, &codingCfg) != H264ENC_OK) {
        std::runtime_error("Failed to get the coding control information for the Hantro H264");
    }

    CLOGGER_DEBUG("Conding control SEI messages: " << codingCfg.seiMessages << "  Slice size: " << codingCfg.sliceSize);
    CLOGGER_DEBUG("Conding control Disable de-blocking: " << codingCfg.disableDeblockingFilter << "  Video full range: " << codingCfg.videoFullRange);
    CLOGGER_DEBUG("Conding control Constrained intra prediction: " << codingCfg.constrainedIntraPrediction);

    /* Set the coding control configuration */
    /*if(H264EncSetCodingCtrl(encoder, &codingCfg) != H264ENC_OK)
    {
        std::runtime_error(std::string("Failed to set the coding control information for the Hantro H264");
    }*/
}

/**
 * @brief Configure the pre processing
 *
 * This configures the input pre processing, like input type, rotation, scaling etc.
 * The following settings are configured:
 * - Input type (Set to the input image pixel format)
 * - Rotation (Set to the input rotation)
 * - Original width (Set to the input width)
 * - Original height (Set to the input height)
 * - Scaled output (Is disabled)
 */
void EncoderH264::configurePreProcessing(void) {
    /* Get the current pre processing configuration  */
    if(H264EncGetPreProcessing(encoder, &preProcCfg) != H264ENC_OK) {
        std::runtime_error("Failed to get the pre processing information for the Hantro H264");
    }

    CLOGGER_DEBUG("Pre processor Input: " << preProcCfg.inputType << " [" << preProcCfg.origWidth << "x" << preProcCfg.origHeight << "]");
    CLOGGER_DEBUG("Pre processor Offset x: " << preProcCfg.xOffset << "  Offset Y: " << preProcCfg.yOffset);
    CLOGGER_DEBUG("Pre processor Rotation: " << preProcCfg.rotation << "  Stabilization: " << preProcCfg.videoStabilization);

    preProcCfg.inputType = getEncPictureType(input_cfg.format);
    preProcCfg.rotation = getEncPictureRotation(input_cfg.rot);
    preProcCfg.origWidth = input_cfg.width;
    preProcCfg.origHeight = input_cfg.height;
    preProcCfg.scaledOutput = 0;    // Disable scaled output

    /* Set the pre processor configuration */
    if(H264EncSetPreProcessing(encoder, &preProcCfg) != H264ENC_OK) {
        std::runtime_error("Failed to set the pre processing information for the Hantro H264");
    }
}

/**
 * @brief Convert image pixel format to H264 picture type
 *
 * This will convert an image pixel type into a H264 encoder picture type.
 * @param format The input image format
 * @return The H264 picture type of the input format
 */
H264EncPictureType EncoderH264::getEncPictureType(Image::pixel_formats format) {
    switch(format) {
        case Image::FMT_UYVY:
            return H264ENC_YUV422_INTERLEAVED_UYVY;

        case Image::FMT_YUYV:
            return H264ENC_YUV422_INTERLEAVED_YUYV;

        default:
            std::runtime_error("Invalid input picture type for the Hantro H264");
            return H264ENC_YUV420_PLANAR;
    }
}

/**
 * @brief Convert rotation into H264 picture rotation
 *
 * This will convert a rotation into a H264 encoder picture rotation type.
 * @param rot The input rotation
 * @return The H264 picture rotation type of the input rotation
 */
H264EncPictureRotation EncoderH264::getEncPictureRotation(enum rotation_t rot) {
    switch(rot) {
        case ROTATE_0:
            return H264ENC_ROTATE_0;

        case ROTATE_90L:
            return H264ENC_ROTATE_90L;

        case ROTATE_90R:
            return H264ENC_ROTATE_90R;

        default:
            std::runtime_error("Invalid input rotation type for the Hantro H264");
            return H264ENC_ROTATE_0;
    }
}
