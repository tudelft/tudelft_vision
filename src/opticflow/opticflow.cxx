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
#include <chrono>

using namespace cv;

int main(int argc, char *argv[])
{
  PLATFORM target;
#if defined(PLATFORM_Bebop)
  EncoderH264 encoder(240, 240, 30, 4000000);
#else
  EncoderJPEG encoder;
#endif
  UDPSocket::Ptr udp = std::make_shared<UDPSocket>(UDP_TARGET, 5000);
  EncoderRTP rtp(udp);

  Cam::Ptr cam = target.getCamera(CAMERA_ID);
  cam->setOutput(Image::FMT_YUYV, 320, 240);
  cam->setCrop(0, 0, 240, 240);

#if defined(PLATFORM_Bebop)
  encoder.setInput(cam);
  encoder.start();

//  FILE *fp = fopen("video.h264", "w");
  std::vector<uint8_t> sps = encoder.getSPS();
  std::vector<uint8_t> pps = encoder.getPPS();
  rtp.setSPSPPS(sps, pps);
#endif

  cam->start();
  uint32_t i = 0;

  cv::Mat camGray;

  cv::Mat prevFrame(64, 64, CV_8UC1);
  cv::Mat currFrame(64, 64, CV_8UC1);
  std::vector<cv::Point2f> prevPts, currPts;

  auto start = std::chrono::monotonic_clock::now();

  while(true) {
    auto end = std::chrono::monotonic_clock::now();
    std::cout << "time " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;

    start = end;
    Image::Ptr img = cam->getImage();

    // Convert to OpenCV matrix (~ 0.8 ms for 240x240 on bebop)
    cv::Mat M(img->getHeight(), img->getWidth(), CV_8UC2, img->getData());
    cv::cvtColor(M, camGray, cv::COLOR_YUV2GRAY_Y422);

    // Resize camera image to a 64x64 frame (< 1 ms)
    cv::resize(camGray, currFrame, currFrame.size(), 0, 0, cv::INTER_NEAREST);

    // Find points in image (TODO: SLOW)
    cv::goodFeaturesToTrack(prevFrame, prevPts, 20, 0.01, 5);

    // If there are no points, continue to next frame
    if (prevPts.empty()) {
        cv::swap(prevFrame, currFrame);
        continue;
    }

    // Calculate optical flow
    std::vector<uchar> status;
    std::vector<float> err;
    cv::Point2f total;
    cv::calcOpticalFlowPyrLK(prevFrame, currFrame, prevPts, currPts, status, err, Size(5, 5), 0);

    // Sum all optical flow vectors
    size_t k = 0;
    for (size_t j = 0; j < prevPts.size(); ++j) {
        // If status = 0; point could not be tracked
        if (!status[j]) {
            continue;
        }
        total += currPts[j] - prevPts[j];
        k++;
    }

    // Print average flow if found
    if (k > 0) {
        cv::Point2f avg(total.x / k, total.y / k);

//        std::cout << avg << std::endl;
    }

    // Swap buffer
    cv::swap(prevFrame, currFrame);

    Image::Ptr enc_img = encoder.encode(img);
    rtp.encode(enc_img);

#if defined(PLATFORM_Bebop)
//    fwrite(sps.data(), sps.size(), 1, fp);
//    fwrite(pps.data(), pps.size(), 1, fp);
//    fwrite((uint8_t *)enc_img->getData(), enc_img->getSize(), 1, fp);
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
