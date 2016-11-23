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

#ifndef ENCODING_ENCODER_RTP_H_
#define ENCODING_ENCODER_RTP_H_

#include <tuv/vision/image.h>
#include <tuv/drivers/udpsocket.h>
#include <stdint.h>
#include <vector>

/**
 * @brief Encode a JPEG or H264 image as RTP
 *
 * This will encode a JPEG or H264 image as RTP and transmit the result over a socket.
 */
class EncoderRTP {
  private:
    UDPSocket::Ptr socket;      ///< The socket to transmit the RTP stream over
    uint16_t sequence;          ///< Sequence number of the RTP stream
    std::vector<uint8_t> data;  ///< Internal data
    uint16_t idx;               ///< Internal data index

    std::vector<uint8_t> sps_data;  ///< H264 SPS data
    std::vector<uint8_t> pps_data;  ///< H264 PPS data

    /* Usefull helper functions */
    void createHeader(uint8_t type, bool marker, uint16_t sequence, uint32_t timestamp);
    void createJPEGHeader(uint32_t offset, uint8_t quality, uint8_t format, uint32_t width, uint32_t height);
    void createH264FragmentAHeader(bool start, bool end, uint8_t nal_hdr);
    void appendBytes(uint8_t *bytes, uint32_t length);

    /* Different encodings */
    void encodeJPEG(uint8_t *img_buf, uint32_t img_size, uint32_t width, uint32_t height);
    void encodeH264(uint8_t *img_buf, uint32_t img_size);

  public:
    EncoderRTP(UDPSocket::Ptr socket);

    void encode(Image::Ptr img);
    void setSPSPPS(std::vector<uint8_t> &sps, std::vector<uint8_t> &pps);
};

#endif /* ENCODING_ENCODER_RTP_H_ */
