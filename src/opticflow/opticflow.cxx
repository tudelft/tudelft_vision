/*
 * This file is part of the TU Delft Vision programs (https://github.com/tudelft/tudelft_vision).
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

#include <tuv/tuv.h>
#include PLATFORM_CONFIG
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <opencv2/opencv.hpp>

using namespace cv;

int main(int argc, char *argv[])
{
  PLATFORM target;
#if defined(PLATFORM_Bebop)
  EncoderH264 encoder(320, 240, 30, 4000000);
#else
  EncoderJPEG encoder;
#endif
  UDPSocket::Ptr udp = std::make_shared<UDPSocket>(UDP_TARGET, 5000);
  EncoderRTP rtp(udp);

  Cam::Ptr cam = target.getCamera(CAMERA_ID);
  cam->setOutput(Image::FMT_YUYV, 320, 240);
  //cam->setCrop(114 + 2300, 106 + 500, 1088, 1920);

#if defined(PLATFORM_Bebop)
  encoder.setInput(cam);
  encoder.start();

  FILE *fp = fopen("video.h264", "w");
  std::vector<uint8_t> sps = encoder.getSPS();
  std::vector<uint8_t> pps = encoder.getPPS();
  rtp.setSPSPPS(sps, pps);
#endif

  cam->start();
  uint32_t i = 0;
  while(true) {
    Image::Ptr img = cam->getImage();

    // Convert to OpenCV matrix
    Mat M(img->getHeight(), img->getWidth(), CV_8UC2, img->getData());
    Mat image;
    cvtColor(M, image, CV_YUV2BGR_Y422); // cvtColor(M, image, CV_YUV2GRAY_Y422);

    // Blur for now
    blur(image, image, Size(5, 5));


    // Start encoding
    //colorrgb_opencv_to_yuv422(image, img->getData(), img->getWidth(), img->getHeight());
    Image::Ptr enc_img = encoder.encode(img);
    rtp.encode(enc_img);

#if defined(PLATFORM_Bebop)
    fwrite(sps.data(), sps.size(), 1, fp);
    fwrite(pps.data(), pps.size(), 1, fp);
    fwrite((uint8_t *)enc_img->getData(), enc_img->getSize(), 1, fp);
#else
    std::string test = "out" + std::to_string(i) + ".jpg";
    FILE *fp = fopen(test.c_str(), "w");
    fwrite((uint8_t *)enc_img->getData(), enc_img->getSize(), 1, fp);
    fclose(fp);
#endif

    ++i;
    i = i %20;
  }


  cam->stop();
  return 0;
}
