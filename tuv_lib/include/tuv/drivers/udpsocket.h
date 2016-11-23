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

#ifndef DRIVERS_UDP_H_
#define DRIVERS_UDP_H_

#include <netinet/in.h>
#include <string>
#include <vector>
#include <memory>

/**
 * @brief UDP driver
 *
 * This is a simple UDP client/server implementation.
 */
class UDPSocket {
  public:
    typedef std::shared_ptr<UDPSocket> Ptr; ///< Shared pointer representation of the UDP socket

  private:
    int fd;                         ///< The socket file
    struct sockaddr_in addr_in;     ///< The input address
    struct sockaddr_in addr_out;    ///< The output address
    uint32_t max_packet_size;       ///< Maximum packet size

  public:
    UDPSocket(std::string host, uint16_t port_in, uint16_t port_out);
    UDPSocket(std::string host, uint16_t port_out);

    void transmit(std::vector<uint8_t> &data);
    uint32_t getMaxPacketSize(void);
};

#endif /* DRIVERS_UDP_H_ */
