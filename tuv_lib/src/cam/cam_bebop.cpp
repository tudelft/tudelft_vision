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

#include "cam/cam_bebop.h"

#include "drivers/clogger.h"
#include "drivers/isp.h"
#include "drivers/mt9f002.h"
#include <linux/v4l2-mediabus.h>
#include <math.h>

/**
 * @brief Initialize the Bebop camera
 *
 * This will setup the Linux camera, I2C-bus communication, PLL configuration and MT9F002 driver.
 */
CamBebop::CamBebop(void) : CamLinux("/dev/video1"),
    i2c_bus("/dev/i2c-0"),
    pll_config{(26 / 2), 7, 1, 1, 59, 8, 1, 1, 1, 1},
    mt9f002(&i2c_bus, MT9F002::PARALLEL, pll_config) {

}

/**
 * @brief Start the Bebop camera
 *
 * First this will start the V4L2 linux camera and then it will configure the ISP.
 */
void CamBebop::start(void) {
    // Start the camera
    CamLinux::start();

    // Configure the ISP
    isp.configure(fd);
    isp.setCrop(0, 0, 800, 608);
}

/**
 * @brief Get a new image
 *
 * Get a new image from the front camera.
 * @return The image
 */
#define MAX_HIST_Y 235
Image::Ptr CamBebop::getImage(void) {
    Image::Ptr img = CamLinux::getImage();
    struct ISP::statistics_t stats = isp.getYUVStatistics();

    if(stats.done && !stats.error) {
        // Auto exposure
        uint32_t cdf[256];
        cdf[0] = stats.hist_y[0];
        for(uint16_t i = 1; i < 256; i++)
            cdf[i] = cdf[i-1] + stats.hist_y[i];

        uint32_t bright_pixels = cdf[MAX_HIST_Y - 1] - cdf[MAX_HIST_Y - 21];    // Top 20 bins
        uint32_t saturated_pixels = cdf[MAX_HIST_Y - 1] - cdf[MAX_HIST_Y - 6];  // Top 5 bins
        uint32_t target_bright_pixels = stats.nb_y / 20;  // 5%
        uint32_t max_saturated_pixels = stats.nb_y / 100; // 1%
        float adjustment = 1.0f;

        if (saturated_pixels + max_saturated_pixels / 10 > max_saturated_pixels) {
          // Fix saturated pixels
          adjustment = 1.0f - (float)saturated_pixels / stats.nb_y;
          adjustment *= adjustment * adjustment;  // speed up
        } else if (bright_pixels + target_bright_pixels / 10 < target_bright_pixels) {
          // increase brightness to try and hit the desired number of well exposed pixels
          int l = MAX_HIST_Y - 1;
          while (bright_pixels < target_bright_pixels && l > 0) {
            bright_pixels += cdf[l];
            bright_pixels -= cdf[l - 1];
            l--;
          }

          adjustment = (float)MAX_HIST_Y / (l + 1);
        } else if (bright_pixels - target_bright_pixels / 10 > target_bright_pixels) {
          // decrease brightness to try and hit the desired number of well exposed pixels
          int l = MAX_HIST_Y - 20;
          while (bright_pixels > target_bright_pixels && l < MAX_HIST_Y) {
            bright_pixels -= cdf[l];
            bright_pixels += cdf[l - 1];
            l++;
          }

          adjustment = (float)(MAX_HIST_Y - 20) / l;
          adjustment *= adjustment;   // speedup
        }

        // Calculate exposure
        adjustment = std::min(std::max(adjustment, 1/16.0f), 4.0f);
        mt9f002.setExposure(mt9f002.getExposure() * adjustment);

        // Calculate AWB
        struct MT9F002::gain_config_t gains = mt9f002.getGains();
        float avgU = ((float) stats.awb_sum[1] / (float) stats.nb_grey) / 240. - 0.5;
        float avgV = ((float) stats.awb_sum[2] / (float) stats.nb_grey) / 240. - 0.5;
        float threshold = 0.002f;
        float gain = 0.5;
        bool changed = false;

        if (fabs(avgU) > threshold) {
          gains.blue -= gain * avgU;
          changed = true;
        }
        if (fabs(avgV) > threshold) {
          gains.red -= gain * avgV;
          changed = true;
        }

        if (changed) {
          gains.blue = std::min(std::max(gains.blue, 2.0f), 75.0f);
          gains.red = std::min(std::max(gains.red, 2.0f), 75.0f);
          //mt9f002.setGains(gains);
        }
    }

    isp.requestYUVStatistics();
    return img;
}

/**
 * @brief Set the camera resolution
 *
 * Note that this is a request and sometimes the resolution will be different than
 * the requested resolution because it is linked to the pixel format.
 * For the Bebop Camera this function is hooked to initialize the subdevice.
 * @param[in] format The requested video format
 * @param[in] width The requested output width
 * @param[in] height The requested output height
 */
void CamBebop::setOutput(enum Image::pixel_formats format, uint32_t width, uint32_t height) {
    // Set the camera sensor size
    mt9f002.setOutput(width, height);

    // Initialize the subdevice
    initSubdevice("/dev/v4l-subdev1", 0, V4L2_MBUS_FMT_SGRBG10_1X10, width, height);

    // Call the super function
    CamLinux::setOutput(format, width, height);
    CamLinux::setCrop(0, 0, width, height);
}

/**
 * @brief Set the camera crop
 *
 * This will set the cropping of the image. The new output image will start from left and top
 * values. It will generate an output window of width x height size.
 * It will try to set the cropping using the MT9F002 CMOS settings instead of the V4L2 camera
 * settings. If a larger width or height is set than the output width and skipping and scaling
 * will be applied to retrieve the closest resolution.
 * @param[in] left The offset from the left in pixels
 * @param[in] top The offset from the top in pixels
 * @param[in] width The output width in pixels
 * @param[in] height The output height in pixels
 */
void CamBebop::setCrop(uint32_t left, uint32_t top, uint32_t width, uint32_t height) {

}
