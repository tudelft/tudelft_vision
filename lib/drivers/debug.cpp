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
 * Initialize the debugger
 */
Debug::Debug(std::string identifier) {
  enabled = true;
  this->identifier = identifier;
}

/**
 * Generates a debug string based on the errno
 * @return Formatted errno debug string
 */
std::string Debug::errnoString(void) {
    return std::to_string(errno) + ": " + strerror(errno);
}

/**
 * Print a debug message
 */
void Debug::printDebug(std::string message) {
  if(enabled)
    std::cout << "[" << this->identifier << "] " << message;
}

/**
 * Print a debug message with a new line
 */
void Debug::printDebugLine(std::string message) {
  if(enabled)
    std::cout  << "[" << this->identifier << "] " << message << std::endl;
}
