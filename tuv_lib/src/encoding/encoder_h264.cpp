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
#include "encoding/h264/ewl.h"
#include "targets/bebop.h"
#include <stdexcept>
#include <assert.h>

/**
 * @brief Generate a new H264 encoder
 *
 * This will generate a new Hantro H264 hardware encoder. The output frame rate will be set to 30 FPS
 * and the target output bit rate will be set to 1Mbps
 * @param width The output width in pixels (must be multiple of 4)
 * @param height The output height in pixels (must be multiple of 2)
 * @param frame_rate The output frame rate in FPS in which the time increments is counted (default 30FPS)
 * @param bit_rate The output target bit rate in bits per second (default 2000000bps = 2Mbps) [10000...60000000]
 */
EncoderH264::EncoderH264(uint32_t width, uint32_t height, float frame_rate, uint32_t bit_rate):
    output_cfg{width, height, frame_rate, bit_rate} {
    assert(width % 4 == 0);
    assert(height % 2 == 0);

    // Open the encoder
    openEncoder();

    // Configure rate (since we have all settings)
    configureRate();

    // Configure coding
    configureCoding();

    // Allocate the SPS + PPS buffer
    if(EWLMallocLinear(*(void **)((uint8_t *)encoder + BEBOP_EWL_OFFSET), 128, &sps_pps_nalu) != EWL_OK) {
        throw std::runtime_error("Could not allocate SPS + PPS EWL Linear buffer");
    }
}

/**
 * @brief Close the encoder
 *
 * This will gracefully close the H264 encoder
 */
EncoderH264::~EncoderH264(void) {
    // Close the encoder
    closeEncoder();
}

/**
 * @brief Set the input configuration
 *
 * This configures the input for the H264 encoder and must be done before starting
 * the encoding. It will take the output of the camera configuration as input and will
 * set the pixel format, width and height accordingly. Note that only images with this
 * exact configuration are allowed to be encoded by this instace!
 * There is a possibility to rotate the input before encoding it, which by default is
 * disabled by setting the rotation to 0 degrees. It can either be 90 degrees left or
 * 90 degrees right.
 * Note that the stride of the images has to be a multiple of 16!
 * @param cam The camera to take the output configuration from
 * @param rot The image rotation which is applied before encoding (default: 0 degrees)
 */
void EncoderH264::setInput(Cam::Ptr cam, enum rotation_t rot) {
    assert(cam->getWidth() % 16 == 0);
    input_cfg.format = cam->getFormat();
    input_cfg.width = cam->getWidth();
    input_cfg.height = cam->getHeight();
    input_cfg.rot = rot;
}

/**
 * @brief Set the input configuration
 *
 * This configures the input for the H264 encoder and must be done before starting
 * the encoding. It will use the supplied pixel format, width and height and images
 * with this exact configuration are allowed to be encoded by this instance.
 * There is a possibility to rotate the input before encoding it, which by default is
 * disabled by setting the rotation to 0 degrees. It can either be 90 degrees left or
 * 90 degrees right.
 * Note that the stride of the images has to be a multiple of 16!
 * @param format The input pixel format (per pixel channel order)
 * @param width The input width in pixels (stride has to be multiple of 16!)
 * @param height The input height in pixels
 * @param rot The image rotation which is applied before encoding (default: 0 degrees)
 */
void EncoderH264::setInput(Image::pixel_formats format, uint32_t width, uint32_t height, enum rotation_t rot) {
    assert(width % 16 == 0);
    input_cfg.format = format;
    input_cfg.width = width;
    input_cfg.height = height;
    input_cfg.rot = rot;
}

/**
 * @brief Start encoding images
 *
 * This will apply all input and output settings and generate the SPS + PPS frame.
 * This can only be called after setInput!
 */
void EncoderH264::start(void) {
    // Reset variables
    frame_cnt = 0;
    intra_cnt = 0;

    // First apply the input settings
    configurePreProcessing();

    // Start the streaming
    streamStart();
}

/**
 * @brief Encode and image
 *
 * This will encode the input image using the hardware Hantro H264 image encoder. It will use the
 * predefined configuration and settings set for this module. The output image is a specifically
 * allocated EWL memory buffer and will be reused when it isn't used anymore.
 * The input image buffer must be linear! It will we mapped to a physical address and if the buffer
 * is a V4L2 image buffer the physical address is saved in a hashmap to optimize this process.
 * @param img The image to encode
 * @return The H264 encoded image
 */
Image::Ptr EncoderH264::encode(Image::Ptr img) {
    assert(img->getPixelFormat() == Image::FMT_UYVY || img->getPixelFormat() == Image::FMT_YUYV);

    // Check cache only for V4L2 images due to limiting amount of image buffers
    bool check_cache = false;
    if(dynamic_cast<ImagePtr*>(img.get())) {
        check_cache = true;
    }

    // Fetch the physical address and check contiguity
    uint64_t phys_addr;
    if(!Bebop::checkContiguity((uintptr_t)img->getData(), img->getSize(), &phys_addr, check_cache)) {
        throw std::runtime_error("Input image is not contigious in the Hantro H264 encoder");
    }

    // Encoder input and output
    H264EncIn encoder_input;
    H264EncOut encoder_output;

    // Setup output buffer settings
    output_buf_t *output_buffer = getFreeBuffer();
    output_buffer->is_free = false;
    encoder_input.pOutBuf = output_buffer->mem.virtualAddress;
    encoder_input.busOutBuf = output_buffer->mem.busAddress;
    encoder_input.outBufSize = output_buffer->mem.size;

    // Setup input buffer settings
    encoder_input.busLuma = phys_addr;
    encoder_input.timeIncrement = (frame_cnt == 0)? 0 : 1; // FIXME: Use the real image time to calculate the increment
    encoder_input.codingType = (intra_cnt == 0)? H264ENC_INTRA_FRAME : H264ENC_PREDICTED_FRAME;

    // Encode the frame
    H264EncRet ret = H264EncStrmEncode(encoder, &encoder_input, &encoder_output, NULL, NULL);

    // Detect errors while encoding
    if(ret == H264ENC_OUTPUT_BUFFER_OVERFLOW) {
        CLOGGER_WARN("H264 encoder has a buffer overflow and couldn't generate an image");
        output_buffer->is_free = true;
        return nullptr;
    } else if(ret != H264ENC_FRAME_READY) {
        throw std::runtime_error("Hantro H264 encoder could not encode frame with error code: " + std::to_string(ret));
    }

    // Update frame information
    CLOGGER_DEBUG("H264 Image " << output_buffer->index << " encoded (frame: " << frame_cnt << ", intra: " << ((intra_cnt == 0)? 1 : 0) << ", nalus:" << encoder_output.numNalus << ")");
    frame_cnt++;
    intra_cnt = (intra_cnt + 1) % (uint32_t)output_cfg.frame_rate;

    // Create a new pointer image
    return std::make_shared<ImagePtr>(this, output_buffer->index, Image::FMT_H264, output_cfg.width, output_cfg.height, (void*)output_buffer->mem.virtualAddress, encoder_output.streamSize);
}

/**
 * @brief Free the bufffer
 *
 * This will set the status of an output buffer to free so the encoder can reuse the
 * same buffer. This should only be called by the image pointer whenever the image is
 * deleted.
 * @param[in] identifier The buffer id to free
 */
void EncoderH264::freeImage(uint16_t identifier) {
    output_buf_t *buf = &output_buffers[identifier];
    buf->is_free = true;
}

/**
 * @brief Get the SPS NALU
 *
 * The SPS NALU is generated right after starting the encoder. Thus this function
 * should only be called after start.
 * @return The SPS NALU bytes
 */
std::vector<uint8_t> EncoderH264::getSPS(void) {
    return sps_nalu;
}

/**
 * @brief Get the PPS NALU
 *
 * The PPS NALU is generated right after starting the encoder. Thus this function
 * should only be called after start.
 * @return The PPS NALU bytes
 */
std::vector<uint8_t> EncoderH264::getPPS(void) {
    return pps_nalu;
}

/**
 * @brief Open the Hantro encoder
 *
 * This will create a new Hantro encoder instance
 */
void EncoderH264::openEncoder(void) {
    assert(output_cfg.width % 4 == 0);
    assert(output_cfg.height % 2 == 0);

    /* Check if we have an encoder and get the version */
    H264EncBuild encBuild = H264EncGetBuild();
    if (encBuild.hwBuild == (uint32_t)EWL_ERROR) {
        throw std::runtime_error("Could not find the Hantro H264 encoder");
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
        throw std::runtime_error("Could not initialize the Hantro H264 encoder");
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
        throw std::runtime_error("Failed to get the rate control information for the Hantro H264");
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
        throw std::runtime_error("Failed to set the rate control information for the Hantro H264");
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
        throw std::runtime_error("Failed to get the coding control information for the Hantro H264");
    }

    CLOGGER_DEBUG("Conding control SEI messages: " << codingCfg.seiMessages << "  Slice size: " << codingCfg.sliceSize);
    CLOGGER_DEBUG("Conding control Disable de-blocking: " << codingCfg.disableDeblockingFilter << "  Video full range: " << codingCfg.videoFullRange);
    CLOGGER_DEBUG("Conding control Constrained intra prediction: " << codingCfg.constrainedIntraPrediction);

    /* Set the coding control configuration */
    /*if(H264EncSetCodingCtrl(encoder, &codingCfg) != H264ENC_OK)
    {
        throw std::runtime_error(std::string("Failed to set the coding control information for the Hantro H264");
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
        throw std::runtime_error("Failed to get the pre processing information for the Hantro H264");
    }

    CLOGGER_DEBUG("Pre processor Input: " << preProcCfg.inputType << " [" << preProcCfg.origWidth << "x" << preProcCfg.origHeight << "]");
    CLOGGER_DEBUG("Pre processor Offset x: " << preProcCfg.xOffset << "  Offset y: " << preProcCfg.yOffset);
    CLOGGER_DEBUG("Pre processor Rotation: " << preProcCfg.rotation << "  Stabilization: " << preProcCfg.videoStabilization);

    preProcCfg.inputType = getEncPictureType(input_cfg.format);
    preProcCfg.rotation = getEncPictureRotation(input_cfg.rot);
    preProcCfg.origWidth = input_cfg.width;
    preProcCfg.origHeight = input_cfg.height;
    preProcCfg.scaledOutput = 0;    // Disable scaled output

    /* Set the pre processor configuration */
    if(H264EncSetPreProcessing(encoder, &preProcCfg) != H264ENC_OK) {
        throw std::runtime_error("Failed to set the pre processing information for the Hantro H264");
    }
}

/**
 * @brief Start the streaming
 *
 * This will start the streaming and receive the SPS + PPS frame
 */
void EncoderH264::streamStart(void) {
    // Encoder input and output
    H264EncIn encoder_input;
    H264EncOut encoder_output;

    // Setup the encoder input
    encoder_input.pOutBuf = sps_pps_nalu.virtualAddress;
    encoder_input.busOutBuf = sps_pps_nalu.busAddress;
    encoder_input.outBufSize = sps_pps_nalu.size;

    // Start the stream
    if(H264EncStrmStart(encoder, &encoder_input, &encoder_output) != H264ENC_OK) {
        throw std::runtime_error("Could not start the stream for the Hantro H264");
    }
    CLOGGER_DEBUG("Nalus: " << encoder_output.numNalus << " size: " << encoder_output.streamSize);

    // Get the SPS
    uint8_t *sps_buf = (uint8_t *)sps_pps_nalu.virtualAddress;
    sps_nalu.clear();
    sps_nalu.insert(sps_nalu.begin(), sps_buf, &sps_buf[encoder_output.pNaluSizeBuf[0]]);

    // Get the PPS
    uint8_t *pps_buf = &sps_buf[encoder_output.pNaluSizeBuf[0]];
    pps_nalu.clear();
    pps_nalu.insert(pps_nalu.begin(), pps_buf, &pps_buf[encoder_output.pNaluSizeBuf[1]]);
    CLOGGER_DEBUG("SPS size: " << sps_nalu.size() << " PPS size: " << pps_nalu.size());

    CLOGGER_INFO("Started streaming in the Hantro H264 encoder");
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
        throw std::runtime_error("Invalid input picture type for the Hantro H264");
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
        throw std::runtime_error("Invalid input rotation type for the Hantro H264");
        return H264ENC_ROTATE_0;
    }
}

/**
 * @brief Get a free output buffer
 *
 * This will get a free output buffer if available. If it is not available it will create a
 * new EWL Linear buffer.
 * @return The free buffer
 */
struct EncoderH264::output_buf_t *EncoderH264::getFreeBuffer(void) {
    // First check already created buffers
    for(uint16_t i = 0; i < output_buffers.size(); ++i) {
        if(output_buffers[i].is_free)
            return &output_buffers[i];
    }

    // Create a new buffer
    struct output_buf_t buf;
    uint32_t output_size = output_cfg.width * output_cfg.height;
    if(EWLMallocLinear(*(void **)((uint8_t *)encoder + BEBOP_EWL_OFFSET), output_size, &buf.mem) != EWL_OK) {
        throw std::runtime_error("Could not allocate EWL Linear buffer");
    }

    buf.index = output_buffers.size();
    output_buffers.push_back(buf);
    CLOGGER_DEBUG("Created new EWL buffer " << buf.index << " of size " << buf.mem.size);
    return &output_buffers.back();
}
