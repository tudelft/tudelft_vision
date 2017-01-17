/*
 * This file is part of the TUV library (https://github.com/tudelft/tudelft_vision).
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

#ifndef DRIVERS_MT9V117_H_
#define DRIVERS_MT9V117_H_

#include <tuv/drivers/i2cbus.h>

/**
 * Driver for the Aptina MT9V117 CMOS Chipset
 */
class MT9V117 {
  public:

  private:
    I2CBus *i2c_bus;            ///< The i2c bus the MT9V117 is connected to

    /* Internal functions */
    void writeRegister(uint16_t address, uint32_t value, uint8_t length);
    uint32_t readRegister(uint16_t address, uint8_t length);
    void writeVar(uint16_t var, uint16_t offset, uint32_t value, uint8_t length);
    uint32_t readVar(uint16_t var, uint16_t offset, uint8_t length);

    void writePatch(void);
    void writeConfig(void);

  public:
    MT9V117(I2CBus *i2c_bus);

};

#endif /* DRIVERS_MT9V117_H_ */
