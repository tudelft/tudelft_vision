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

#include "cam/cam_linux.h"

#include "drivers/clogger.h"
#include "vision/image_v4l2.h"
#include <cstring>
#include <stdexcept>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/v4l2-subdev.h>

/**
 * @brief Initialize a linux camera device
 *
 * This will initialize a new linux camera and request the capabilities and possible
 * output formats.
 * @param device_name The linux device name including path (for example: /dev/video1)
 */
CamLinux::CamLinux(std::string device_name) {
    // Set the device name
    this->device_name = device_name;

    // Try to open the device
    this->openDevice();

    // Try to fetch the capabilities of the V4L2 device
    this->getCapabilities();

    // Fetch the possible video formats
    this->getFormats();

    // Set fds properties
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
}

/**
 * @brief Close the Linux camera
 *
 * This will close the connection to the Linux camera. Make sure you stop streaming
 * before destroying the Linux camera.
 */
CamLinux::~CamLinux(void) {
    assert(fd >= 0);

    //TODO: stop streaming
    close(fd);
    CLOGGER_INFO("Closed " << device_name);
}

/**
 * @brief Start the camera streaming
 *
 * This will start the streaming of the camera and after this calls to getImage can be made
 * in order to retreive images from the Linux camera.
 */
void CamLinux::start(void) {
    assert(fd >= 0);

    // Check if the device is capable of streaming
    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        throw std::runtime_error("Device " + device_name + " isn't capable of video streaming (V4L2_CAP_STREAMING)");
    }

    // Initialize buffers if needed
    if(buffers.empty())
        initBuffers();

    // Enqueue all buffers
    for(auto const &buffer: buffers) {
        if(buffer.state == BUFFER_DEQUEUED)
            enqueueBuffer(buffer);
    }

    // Start the stream
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        throw std::runtime_error("Device " + device_name + " couldn't start stream (VIDIOC_STREAMON)");
    }
}

/**
 * @brief Stop the camera streaming
 *
 * This will stop the camera streaming and after this no calls to getImage can be made.
 */
void CamLinux::stop(void) {
    assert(fd >= 0);

    // Stop the streaming
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0) {
        throw std::runtime_error("Device " + device_name + " couldn't stop stream (VIDIOC_STREAMOFF)");
    }
}

/**
 * @brief Get an image from the camera
 *
 * This will do a select on the device and will return an Image pointer whenever
 * an image becomes available. It will automatically dequeue the buffer and save
 * the pointer in the V4L2 image, thus preventing a buffer copy.
 * Because it is actually an ImageV4L2 it will automatically enqueue the buffer when
 * the image isn't used anymore.
 * @return The image from the camera
 */
Image::Ptr CamLinux::getImage(void) {
    assert(fd >= 0);

    // Wait until an image was taken, with a timeout of 2 seconds
    int sr = 0;
    while(sr == 0) {
        // Set image timeout
        struct timeval tv = {};
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        // Wait for an image
        sr = select(fd + 1, &fds, NULL, NULL, &tv);
        if(sr < 0) {
            throw std::runtime_error("Device " + device_name + " could not take a shot");
        } else if(sr == 0) {
            printf("Timeout\r\n");
        }
    }


    // Dequeue a buffer
    struct buffer_t *buffer = dequeueBuffer();
    CLOGGER_INFO("Got new image from " << device_name);

    // Create an image
    return std::make_shared<ImageV4L2>(this, buffer->index, buffer->buf);
}

/**
 * @brief Converts a V4L2 format descriptor to string
 *
 * This will convert a V4L2 format descriptor to a string based on the 4 character code.
 * @param[in] format The V4L2 format descriptor
 * @return The string of the V4L2 format descriptor
 */
std::string CamLinux::formatToString(uint32_t format) {
    char descriptor[5];
    descriptor[0] = (format & 0xFF);
    descriptor[1] = format >> 8;
    descriptor[2] = format >> 16;
    descriptor[3] = format >> 24;
    descriptor[4] = '\0';

    return std::string(descriptor);
}

/**
 * @brief Initialize a v4l2 subdevice
 *
 * This will initialize a subdevice wth a specific pad, code, width and height.
 * @param[in] subdevice_name The subdevice name including path (for example /dev/v4l-sub1)
 * @param[in] pad The pad which it is connected to
 * @param[in] code The output format
 * @param[in] width The output image width
 * @param[in] height The output image height
 */
void CamLinux::initSubdevice(std::string subdevice_name, uint8_t pad, uint16_t code, uint32_t width, uint32_t height) {
    // Try to open the subdevice
    int sfd = open(subdevice_name.c_str(), O_RDWR, 0);
    if (sfd < 0) {
        throw std::runtime_error("Could not open " + subdevice_name + " (" + strerror(errno) + ")");
    }

    // Try to get the subdevice data format settings
    struct v4l2_subdev_format sfmt = {};
    if (ioctl(sfd, VIDIOC_SUBDEV_G_FMT, &sfmt) < 0) {
        close(sfd);
        throw std::runtime_error("Could not get video format of " + subdevice_name + " VIDIOC_SUBDEV_G_FMT (" + strerror(errno) + ")");
    }

    // Set the new settings
    sfmt.pad = pad;
    sfmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    sfmt.format.width = width;
    sfmt.format.height = height;
    sfmt.format.code = code;
    sfmt.format.field = V4L2_FIELD_NONE;
    sfmt.format.colorspace = 1;

    if (ioctl(sfd, VIDIOC_SUBDEV_S_FMT, &sfmt) < 0) {
        close(sfd);
        throw std::runtime_error("Could not set video format of " + subdevice_name + " VIDIOC_SUBDEV_G_FMT (" + strerror(errno) + ")");
    }

    // Close the device
    close(sfd);
}

/**
 * @brief Opens the linux video device
 *
 * This will try to open the video device.
 */
void CamLinux::openDevice(void) {
    assert(!device_name.empty());

    // Try to open the device
    fd = open(device_name.c_str(), O_RDWR | O_NONBLOCK, 0);
    if(fd < 0) {
        throw std::runtime_error("Could not open " + device_name + " (" + strerror(errno) + ")");
    }

    CLOGGER_INFO("Opened " << device_name);
}

/**
 * @brief Retreive the capabilities of the linux Camera device
 *
 * This will try to retreive the capabilities which are supported by the camera device.
 */
void CamLinux::getCapabilities(void) {
    assert(fd >= 0);

    // Try to retreive the device capabilities
    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
        close(fd);
        throw std::runtime_error("Could not receive capabilities (VIDIOC_QUERYCAP) of " + device_name +  + " (" + strerror(errno) + ")");
    }

    CLOGGER_DEBUG("Device driver is " << cap.driver);
    CLOGGER_DEBUG(cap.card);
    CLOGGER_DEBUG(cap.version);


    // Check if the device is capable of capturing images
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        close(fd);
        throw std::runtime_error("Device " + device_name + " isn't capable of capturing images (V4L2_CAP_VIDEO_CAPTURE)");
    }

    CLOGGER_DEBUG("The device is capable of video capturing");
}

/**
 * @brief Retreive the possible camera formats
 *
 * This will try to fetch the camera formats which are supported by the device.
 */
void CamLinux::getFormats(void) {
    assert(fd >= 0);

    // Setup the format descriptor request
    struct v4l2_fmtdesc fmtdesc = {};
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;

    // Request possible formats
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        CLOGGER_DEBUG("Found possible video format: \"" << (char*)fmtdesc.description << "\" (" << formatToString(fmtdesc.pixelformat) << ")");
        formats.push_back(fmtdesc);
        fmtdesc.index++;
    }

    // Check if at least one format is available
    if(formats.empty()) {
        close(fd);
        throw std::runtime_error("Device " + device_name + " doesn't have any available format (VIDIOC_ENUM_FMT)");
    }
}

/**
 * @brief Set the camera resolution
 *
 * Note that this is a request and sometimes the resolution will be different than
 * the requested resolution because it is linked to the pixel format.
 * @param[in] format The requested video format
 * @param[in] width The requested output width
 * @param[in] height The requested output height
 */
void CamLinux::setOutput(enum Image::pixel_formats format, uint32_t width, uint32_t height) {
    // FIXME: Check if the request is valid?

    // Set the format settings
    this->pixel_format = format;
    this->width = width;
    this->height = height;

    // Set the format settings
    struct v4l2_format fmt = {};
    uint32_t v4l2_format = toV4L2Format(pixel_format);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = v4l2_format;
    fmt.fmt.pix.colorspace = V4L2_COLORSPACE_REC709;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        throw std::runtime_error("Device " + device_name + " couldn't set requested resolution or pixelformat VIDIOC_S_FMT (" + std::to_string(width) + ", " + std::to_string(height) + ", " + formatToString(v4l2_format) + ")");
    }

    if (ioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
        throw std::runtime_error("Device " + device_name + " couldn't get resolution and pixelformat VIDIOC_G_FMT (" + std::to_string(width) + ", " + std::to_string(height) + ", " + formatToString(v4l2_format) + ")");
    }

    CLOGGER_DEBUG("Configured output format " << formatToString(v4l2_format) << " with resolution " << width << "x" << height << " for device " << device_name);
}

/**
 * @brief Set the camera crop
 *
 * This will set the cropping of the image. The new output image will start from left and top
 * values. It will generate an output window of width x height size.
 * @param[in] left The offset from the left in pixels
 * @param[in] top The offset from the top in pixels
 * @param[in] width The output width in pixels
 * @param[in] height The output height in pixels
 */
void CamLinux::setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height) {
    // Set the cropping window
    struct v4l2_crop crop = {};
    crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    crop.c.top = top;
    crop.c.left = left;
    crop.c.width = width;
    crop.c.height = height;

    if (ioctl(fd, VIDIOC_S_CROP, &crop) < 0) {
        throw std::runtime_error("Device " + device_name + " couldn't set requested crop VIDIOC_S_CROP (" + std::to_string(left) + ", " + std::to_string(top) + ", " + std::to_string(width) + ", " + std::to_string(height) + ")");
    }
}

/**
 * @brief Convert image pixel format to V4L2 pixel format
 *
 * This will convert an Image pixel format like YUYV to the corresponding V4L2
 * pixel format. It is used to configure the video device.
 * @param[in] format The pixel format to convert
 * @return The V4L2 pixel format representation
 */
uint32_t CamLinux::toV4L2Format(enum Image::pixel_formats format) {
    switch(format) {
    case Image::FMT_UYVY:
        return V4L2_PIX_FMT_UYVY;

    case Image::FMT_YUYV:
        return V4L2_PIX_FMT_YUYV;

    default:
        throw std::runtime_error("Could not convert " + std::to_string(format) + " to V4L2 pixel format");
        break;
    }
}

/**
 * @brief Initialize the v4l2 buffers
 *
 * This will use memory mapping to initialize V4L2 buffers which are used to
 * store the images. Currently it will request 10 buffers.
 */
void CamLinux::initBuffers(void) {
    // Request MMAP buffers
    struct v4l2_requestbuffers req = {};
    req.count = 10; //FIXME: variable amount of buffers
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        throw std::runtime_error("Could not request MMAP buffers for " + device_name);
    }

    // Go trough the buffers and initialize them
    for (uint16_t i = 0; i < req.count; ++i) {
        struct v4l2_buffer buf = {};

        // Request the buffer information
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
            throw std::runtime_error("Could not query MMAP buffer " + std::to_string(i) + " for " + device_name);
        }

        //  Map the buffer
        struct buffer_t buffer;
        buffer.index = i;
        buffer.state = BUFFER_DEQUEUED;
        buffer.length = buf.length;
        buffer.buf = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (MAP_FAILED == buffer.buf) {
            throw std::runtime_error("Could not MMAP buffer " + std::to_string(i) + " for " + device_name);
        }

        /*if(checkcontiguity((unsigned long)buffers[i].buf,
                            getpid(),
                            &pmem,
                            buf.length)) {
          printf("[v4l2] Physical memory %d is not contiguous with length %d from %s\n", i, buf.length, device_name);
        }*/

        //buffer.physp = pmem.paddr;
        CLOGGER_DEBUG("MMAP buffer " << i << " generated");
        buffers.push_back(buffer);
    }
}

/**
 * @brief Enqueue a v4l2 buffer
 *
 * This will enqueue a buffer using the id. This needs to be done when the
 * buffer isn't needed anymore such that the video device can use this buffer to
 * generate a new image.
 * @param[in] buffer The buffer to enqueue
 */
void CamLinux::enqueueBuffer(struct buffer_t buffer) {
    assert(fd >= 0);
    assert(buffer.state == BUFFER_DEQUEUED);

    // Enqueue the buffer
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = buffer.index;
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        throw std::runtime_error("Could not enqueue buffer " + std::to_string(buffer.index) + " for " + device_name);
    }

    buffer.state = BUFFER_ENQUEUED;
    CLOGGER_DEBUG("Enqueue buffer " << buffer.index);
}

/**
 * @brief Enqueue a v4l2 buffer
 *
 * This will enqueue a buffer with a specific ID. This needs to be done when the
 * buffer isn't needed anymore such that the video device can use this buffer to
 * generate a new image.
 * @param[in] buffer_id The buffer id to enqueue
 */
void CamLinux::enqueueBuffer(uint16_t buffer_id) {
    enqueueBuffer(buffers.at(buffer_id));
}

/**
 * @brief Dequeue a v4l2 buffer
 *
 * This will dequeue a V4L2 buffer, this must be called after select to make sure
 * a buffer can be dequeued. The dequeued buffer represents an image.
 * @return The dequeued buffer
 */
struct CamLinux::buffer_t *CamLinux::dequeueBuffer(void) {
    assert(fd >= 0);

    // Dequeue the buffer
    struct v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        throw std::runtime_error("Could not dequeue a buffer for " + device_name + " (" + strerror(errno) + ")");
    }

    CLOGGER_DEBUG("Dequeue buffer " << buf.index);

    struct buffer_t *buffer = &buffers[buf.index];
    buffer->state = BUFFER_DEQUEUED;
    return buffer;
}
