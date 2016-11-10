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

/**
 * @brief Open the Hantro encoder
 *
 * This will create a new Hantro encoder instance
 */
void EncoderH264::openEncoder(void) {

    /* Check if we have an encoder and get the version */
    H264EncBuild encBuild = H264EncGetBuild();
    if (encBuild.hwBuild == EWL_ERROR){
        std::runtime_error("Could not find the Hantro H264 encoder");
    }
    CLOGGER_INFO("Found hantro encoder (HW: " << encBuild.hwBuild << " SW:" << encBuild.swBuild);

    /* Set encoder configuration */
    cfg.width = width;
    cfg.height = height;

    cfg.frameRateDenom = 1;
    cfg.frameRateNum = frame_rate;

    cfg.streamType = H264ENC_BYTE_STREAM;

    cfg.level = H264ENC_LEVEL_4; // level 4 minimum for 1080p
    cfg.viewMode = H264ENC_BASE_VIEW_DOUBLE_BUFFER; // maybe H264ENC_BASE_VIEW_SINGLE_BUFFER
    cfg.scaledWidth = 16; // 0
    cfg.scaledHeight = 16; // 0

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
    rcCfg.bitPerSecond = bit_rate;
    rcCfg.gopLen = intra_rate; // user guide says to set goLopen to I frame rate
    rcCfg.hrd = 0; // disable Hrd conformance
    if(H264EncSetRateCtrl(encoder, &rcCfg) != H264ENC_OK) {
        std::runtime_error("Failed to set the rate control information for the Hantro H264");
    }
}


void EncoderH264::configureCoding(void) {
    /* Get the current conding control configuration */
    if(H264EncGetCodingCtrl(encoder, &codingCfg) != H264ENC_OK) {
        std::runtime_error("Failed to get the coding control information for the Hantro H264");
    }

    CLOGGER_DEBUG("Conding control SEI messages: " << codingCfg.seiMessages << "  Slice size: " << codingCfg.sliceSize);
    CLOGGER_DEBUG("Conding control Disable de-blocking: " << codingCfg.disableDeblockingFilter << "  Video full range: " << codingCfg.videoFullRange);
    CLOGGER_DEBUG("Conding control Constrained intra prediction: " << codingCfg.constrainedIntraPrediction);

    /* Set the coding control configuration */
    /*if((ret = H264EncSetCodingCtrl(encoder, &codingCfg)) != H264ENC_OK)
    {
        std::runtime_error(std::string("Failed to set the coding control information for the Hantro H264");
    }*/
}

void EncoderH264::configurePreProcessing(void) {
    /* Get the current pre processing configuration  */
    if(H264EncGetPreProcessing(encoder, &preProcCfg) != H264ENC_OK) {
        std::runtime_error("Failed to get the pre processing information for the Hantro H264");
    }

    CLOGGER_DEBUG("Pre processor Input: " << preProcCfg.inputType << " [" << preProcCfg.origWidth << "x" << preProcCfg.origHeight << "]");
    CLOGGER_DEBUG("Pre processor Offset x: " << preProcCfg.xOffset << "  Offset Y: " << preProcCfg.yOffset);
    CLOGGER_DEBUG("Pre processor Rotation: " << preProcCfg.rotation << "  Stabilization: " << preProcCfg.videoStabilization);

    preProcCfg.inputType = input_type;
    preProcCfg.rotation = H264ENC_ROTATE_0;
    preProcCfg.origWidth = width;
    preProcCfg.origHeight = height;
    //preProcCfg.scaledOutput = 1;

    /* Set the pre processor configuration */
    if(H264EncSetPreProcessing(encoder, &preProcCfg) != H264ENC_OK) {
        std::runtime_error("Failed to set the pre processing information for the Hantro H264");
    }
}
