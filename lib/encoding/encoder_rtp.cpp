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

#include "encoder_rtp.h"

#include <assert.h>
#include <sys/time.h>

EncoderRTP::EncoderRTP(UDPSocket::Ptr socket) {
    this->socket = socket;
    this->sequence = 0;
}

void EncoderRTP::createHeader(uint8_t type, bool marker, uint16_t sequence, uint32_t timestamp) {
    uint8_t marker_bit = marker? 1 : 0;
    data.push_back(0x80);                            // RTP version (only 1 source
    data.push_back(type | (marker_bit<<7));          // Payload type and marker bit
    data.push_back(sequence >> 8);                   // Sequence number MSB
    data.push_back(sequence & 0xFF);                 // Sequence number LSB
    data.push_back((timestamp & 0xFF000000) >> 24);  // Timestamp MSB
    data.push_back((timestamp & 0x00FF0000) >> 16);  // Timestamp 2nd MSB
    data.push_back((timestamp & 0x0000FF00) >> 8);   // Timestamp 2nd LSB
    data.push_back(timestamp & 0x000000FF);          // Timestamp LSB
    data.push_back(0x13);                            // Arbritary 4-byte SSRC (sychronization source identifier)
    data.push_back(0xF9);                            // Arbritary 4-byte SSRC (sychronization source identifier)
    data.push_back(0x7E);                            // Arbritary 4-byte SSRC (sychronization source identifier)
    data.push_back(0x67);                            // Arbritary 4-byte SSRC (sychronization source identifier)
}

void EncoderRTP::createJPEGHeader(uint32_t offset, uint8_t quality, uint8_t format, uint32_t width, uint32_t height) {
    assert(width%8 == 0);
    assert(height%8 == 0);
    assert(width < 2040);
    assert(height < 2040);
    assert(offset <= 0x00FFFFFF);

    // Generate the JPEG header based on https://tools.ietf.org/html/rfc2435
    data.push_back(0x00);                            // Type specific
    data.push_back((offset & 0x00FF0000) >> 16);     // Offset MSB
    data.push_back((offset & 0x0000FF00) >> 8);      // Offset 2nd MSB
    data.push_back(offset & 0x000000FF);             // Offset LSB
    data.push_back(format);                          // Format
    data.push_back(quality);                         // Quality
    data.push_back(width / 8);                       // Width
    data.push_back(height / 8);                      // Height
}

void EncoderRTP::appendBytes(uint8_t *bytes, uint32_t length) {
    for(uint32_t i = 0; i < length; i++)
        data.push_back(bytes[i]);
}

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
        createHeader(0x1A, ((img_size - offset) <= packet_size), sequence++, t);
        createJPEGHeader(offset, 80, 0, img->getWidth(), img->getHeight());
        appendBytes(&img_buf[offset], curr_size);
        socket->transmit(data);
    }
}
