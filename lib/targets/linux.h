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

#include "target.h"

#ifndef TARGETS_LINUX_H_
#define TARGETS_LINUX_H_

/**
 * @brief Generic Linux
 *
 * This is a generic Linux implementation which uses V4L2 to access the camera's.
 */
class Linux : public Target {
  private:

  public:
    Linux(void);

    Cam::Ptr getCamera(uint32_t id);
};

#endif /* TARGETS_LINUX_H_ */
