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

#include "encoding/encoder_rtp.h"

#include <assert.h>
#include <sys/time.h>

/**
 * @brief Create a new RTP encoder
 *
 * This will create a new RTP encoder for which the output socket can be set.
 * @param[in] socket The output socket
 */
EncoderRTP::EncoderRTP(UDPSocket::Ptr socket) {
    this->socket = socket;
    this->sequence = 0;
}

/**
 * @brief Create an RTP header
 *
 * This will create an RTP header based on the type, marker, sequence and timestamp.
 * @param[in] type Type of RTP data which is going to be encoded
 * @param[in] marker If the marker bit is set
 * @param[in] sequence The sequence number
 * @param[in] timestamp Current timestamp
 */
void EncoderRTP::createHeader(uint8_t type, bool marker, uint16_t sequence, uint32_t timestamp) {
    uint8_t marker_bit = marker? 1 : 0;
    data[idx++]  = 0x80;                            // RTP version (only 1 source
    data[idx++]  = type | (marker_bit<<7);          // Payload type and marker bit
    data[idx++]  = sequence >> 8;                   // Sequence number MSB
    data[idx++]  = sequence & 0xFF;                 // Sequence number LSB
    data[idx++]  = (timestamp & 0xFF000000) >> 24;  // Timestamp MSB
    data[idx++]  = (timestamp & 0x00FF0000) >> 16;  // Timestamp 2nd MSB
    data[idx++]  = (timestamp & 0x0000FF00) >> 8;   // Timestamp 2nd LSB
    data[idx++]  = timestamp & 0x000000FF;          // Timestamp LSB
    data[idx++]  = 0x13;                            // Arbritary 4-byte SSRC (sychronization source identifier)
    data[idx++]  = 0xF9;                            // Arbritary 4-byte SSRC (sychronization source identifier)
    data[idx++] = 0x7E;                             // Arbritary 4-byte SSRC (sychronization source identifier)
    data[idx++] = 0x67;                             // Arbritary 4-byte SSRC (sychronization source identifier)
}

/**
 * @brief Create a JPEG header
 *
 * This header is part of the RTP data and will describe the JPEG data encoded in the data of the RTP packet.
 * @param[in] offset The offset based from the start of the image
 * @param[in] quality The quality of the JPEG encoding
 * @param[in] format The JPEG format
 * @param[in] width Width of the image (must be dividable by 8)
 * @param[in] height Height of the image (must be dividable by 8)
 */
void EncoderRTP::createJPEGHeader(uint32_t offset, uint8_t quality, uint8_t format, uint32_t width, uint32_t height) {
    assert(width%8 == 0);
    assert(height%8 == 0);
    assert(width < 2040);
    assert(height < 2040);
    assert(offset <= 0x00FFFFFF);

    // Generate the JPEG header based on https://tools.ietf.org/html/rfc2435
    data[idx++] = 0x00;                            // Type specific
    data[idx++] = (offset & 0x00FF0000) >> 16;     // Offset MSB
    data[idx++] = (offset & 0x0000FF00) >> 8;      // Offset 2nd MSB
    data[idx++] = offset & 0x000000FF;             // Offset LSB
    data[idx++] = format;                          // Format
    data[idx++] = quality;                         // Quality
    data[idx++] = width / 8;                       // Width
    data[idx++] = height / 8;                      // Height
}

/**
 * @brief Append bytes to the RTP data
 *
 * This is used to append bytes to the RTP data. For example to append the JPEG image.
 * @param bytes The bytes to append
 * @param length The length of the input buffer
 */
void EncoderRTP::appendBytes(uint8_t *bytes, uint32_t length) {
    for(uint32_t i = 0; i < length; i++)
        data[idx++] = bytes[i];
}

/**
 * @brief Encode an image
 *
 * This will encode the image using RTP and will send the output over the output socket. If the output
 * socket is non-blocking this function will return as soon as possible.
 * @param img The image to encode
 */
void EncoderRTP::encode(Image::Ptr img) {
    uint8_t *img_buf = (uint8_t *)img->getData();
    uint32_t img_size = img->getSize();
    uint32_t packet_size = socket->getMaxPacketSize();

    struct timeval tv;
    gettimeofday(&tv, 0);
    uint32_t t = (tv.tv_sec % (256 * 256)) * 90000 + tv.tv_usec * 9 / 100;

    for(uint32_t offset = 0; (int32_t)(img_size - offset) > 0; offset += packet_size) {
        uint32_t curr_size = ((img_size - offset) > packet_size)? packet_size : (img_size - offset);

        data.clear();
        data.resize(12 + 8 + curr_size);
        idx = 0;

        createHeader(0x1A, ((img_size - offset) <= packet_size), sequence++, t);
        createJPEGHeader(offset, 80, 0, img->getWidth(), img->getHeight());
        appendBytes(&img_buf[offset], curr_size);

        socket->transmit(data);
    }
}
