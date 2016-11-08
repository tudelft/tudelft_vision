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

#include "cam.h"

#include <string>
#include <vector>
#include <linux/videodev2.h>

#ifndef CAM_LINUX_H_
#define CAM_LINUX_H_

/**
 * @brief A linux based camera
 *
 * This will setup a Linux based V4L2 camera.
 */
class CamLinux: public Cam {
  protected:
    /** The buffer state */
    enum buffer_state_t {
        BUFFER_ENQUEUED,          ///< The Buffer is enqueued
        BUFFER_DEQUEUED           ///< The buffer is dequeued
    };

    /** Specific V4L2 buffer */
    struct buffer_t {
        uint16_t index;             ///< The index of the buffer
        enum buffer_state_t state;  ///< The current buffer state

        size_t length;              ///< The size of the buffer
        void *buf;                  ///< Pointer to the memory mapped buffer
    };

    std::string device_name;                  ///< The device name including file path
    int fd;                                   ///< File pointer to the linux device
    fd_set fds;                               ///< Set of file pointers

    struct v4l2_capability cap;                 ///< The V4L2 capabilities of the device
    std::vector<struct v4l2_fmtdesc> formats;   ///< Possible V4L2 formats for the device
    std::vector<struct buffer_t> buffers;       ///< The V4L2 buffers

    /* Internal functions */
    std::string formatToString(uint32_t format);
    uint32_t toV4L2Format(enum Image::pixel_formats format);
    void openDevice(void);
    void getCapabilities(void);
    void getFormats(void);
    void initBuffers(void);
    void enqueueBuffer(struct buffer_t buffer);
    struct buffer_t *dequeueBuffer(void);

  public:
    CamLinux(std::string device_name);
    ~CamLinux(void);

    void initSubdevice(std::string subdevice_name, uint8_t pad, uint16_t code, uint32_t width, uint32_t height);

    void start(void);
    void stop(void);

    Image::Ptr getImage(void);
    void enqueueBuffer(uint16_t buffer_id);

    /* Settings */
    void setOutput(enum Image::pixel_formats format, uint32_t width, uint32_t height);
    void setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height);
};

#endif /* CAM_LINUX_H_ */
