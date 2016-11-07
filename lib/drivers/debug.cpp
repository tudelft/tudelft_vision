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

#include "debug.h"

#include <iostream>
#include <errno.h>
#include <string.h>

/**
 * @brief Initialize the debugger
 * @param[in] identifier The identifier to print before the debug information
 */
Debug::Debug(std::string identifier) {
    enabled = true;
    this->identifier = identifier;
}

/**
 * @brief Generates a debug string based on the errno
 * @return Formatted errno debug string
 */
std::string Debug::errnoString(void) {
    return std::to_string(errno) + ": " + strerror(errno);
}

/**
 * @brief Print a debug message
 * @param[in] message The message to print
 */
void Debug::printDebug(std::string message) {
    if(enabled)
        std::cout << "[" << this->identifier << "] " << message;
}

/**
 * @brief Print a debug message with a new line
 * @param[in] message The message to print
 */
void Debug::printDebugLine(std::string message) {
    if(enabled)
        std::cout  << "[" << this->identifier << "] " << message << std::endl;
}

/**
 * @brief This enables the debug output
 */
void Debug::enableDebug(void) {
    enabled = true;
}

/**
 * @brief This disables the debug output
 */
void Debug::disableDebug(void) {
    enabled = false;
}
