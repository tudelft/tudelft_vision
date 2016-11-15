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

#include <tuv/tuv.h>
#include PLATFORM_CONFIG
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{
  PLATFORM target;
#if TARGET == Bebop
  EncoderH264 encoder(1080, 720);
  EncoderJPEG encoder_jpeg;
#else
  EncoderJPEG encoder;
#endif
  UDPSocket::Ptr udp = std::make_shared<UDPSocket>(UDP_TARGET, 5000);
  EncoderRTP rtp(udp);

  Cam::Ptr cam = target.getCamera(CAMERA_ID);
  cam->setOutput(Image::FMT_YUYV, 720, 1088);
  //cam.setCrop(114, 106, 2048, 3320);
  encoder.setInput(cam, EncoderH264::ROTATE_90L);
  encoder.start();

  std::ofstream outfile("video.h264", std::ios::out | std::ios::binary);
  std::vector<uint8_t> sps = encoder.getSPS();
  std::vector<uint8_t> pps = encoder.getPPS();

  cam->start();
  uint32_t i = 0;
  while(true) {
    Image::Ptr img = cam->getImage();
    Image::Ptr enc_img = encoder.encode(img);

    img->downsample(4);
    Image::Ptr jpeg_img = encoder_jpeg.encode(img);
    //rtp.encode(jpeg);
    //usleep(200000);

    std::cout << "Writing " << sps.data() << " size: " << sps.size() << "\r\n";
    outfile.write((char*)sps.data(), sps.size());
    std::cout << "Writing " << pps.data() << " size: " << pps.size() << "\r\n";
    outfile.write((char*)pps.data(), pps.size());
    std::cout << "Writing " << enc_img->getData() << " size: " << enc_img->getSize() << "\r\n";
    outfile.write((char*)enc_img->getData(), enc_img->getSize());

    std::string test = "out" + std::to_string(i) + ".jpg";
    FILE *fp = fopen(test.c_str(), "w");
    fwrite((uint8_t *)jpeg_img->getData(), jpeg_img->getSize(), 1, fp);
    fclose(fp);
    ++i;
    i = i %20;
  }


  cam->stop();
  return 0;
}
