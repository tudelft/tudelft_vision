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

#include <string>

#ifndef DRIVERS_DEBUG_H_
#define DRIVERS_DEBUG_H_

/**
 * A linux based debug printing class
 */
class Debug {
  private:
    bool enabled;             ///< If debug printing is enabled
    std::string identifier;   ///< The debug identifier

  protected:
    std::string errnoString(void);
    void printDebug(std::string message);
    void printDebugLine(std::string message);

    void enableDebug(void);
    void disableDebug(void);

  public:
    Debug(std::string identifier);    ///< Debug initialization
};

#endif /* DRIVERS_DEBUG_H_ */
