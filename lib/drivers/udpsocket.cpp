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

#include "udpsocket.h"

#include <assert.h>
#include <arpa/inet.h>

/**
 * @brief Create a new UDP socket with no input
 *
 * This will create a new UDP socket with only output
 * @param[in] host The host address to send the output to
 * @param[in] port_out The output port
 */
UDPSocket::UDPSocket(std::string host, uint16_t port_out) {
    max_packet_size = 1400;
    int one = 1;

    // Create the socket and enable reusing of address
    fd = socket(PF_INET, SOCK_DGRAM, 0);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // Setup the output address
    addr_out.sin_family = PF_INET;
    addr_out.sin_port = htons(port_out);
    addr_out.sin_addr.s_addr = inet_addr(host.c_str());
}

/**
 * @brief Create a new UDP socket with also input
 *
 * This will create a new UDP socket with both input and output
 * @param host The host address to send the output to
 * @param port_in The input port
 * @param port_out The output port
 */
UDPSocket::UDPSocket(std::string host, uint16_t port_in, uint16_t port_out) {
    max_packet_size = 1400;
    int one = 1;

    // Create the socket and enable reusing of address
    fd = socket(PF_INET, SOCK_DGRAM, 0);
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    // Setup the output address
    addr_out.sin_family = PF_INET;
    addr_out.sin_port = htons(port_out);
    addr_out.sin_addr.s_addr = inet_addr(host.c_str());

    // Create the input address and bind to it
    addr_in.sin_family = PF_INET;
    addr_in.sin_port = htons(port_in);
    addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(fd, (struct sockaddr *)&addr_in, sizeof(addr_in));
}

/**
 * @brief Transmit data to the UDP output
 *
 * This will transmit data to the UDP output
 * @param data The data to transmit
 */
void UDPSocket::transmit(std::vector<uint8_t> data) {
    ssize_t cnt = sendto(fd, data.data(), data.size(), MSG_DONTWAIT, (struct sockaddr *)&addr_out, sizeof(addr_out));
    assert(cnt > 0);
    assert((uint32_t)cnt == data.size());
}

/**
 * @brief Get the maximum packet size
 *
 * This will return the maximum packet size of an UDP packet.
 * @return The maximum packet size
 */
uint32_t UDPSocket::getMaxPacketSize(void) {
    return max_packet_size;
}
