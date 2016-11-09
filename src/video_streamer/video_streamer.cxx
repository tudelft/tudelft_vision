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
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  PLATFORM target;
  EncoderJPEG jpeg_encoder;
  UDPSocket::Ptr udp = std::make_shared<UDPSocket>("127.0.0.1", 5000);
  EncoderRTP rtp(udp);

  Cam::Ptr cam = target.getCamera(0);
  cam->setOutput(Image::FMT_YUYV, 800, 600);
  //cam.setCrop(114, 106, 2048, 3320);

  cam->start();
  uint32_t i = 0;
  while(i < 20) {
    Image::Ptr img = cam->getImage();
    //img->downsample(5);
    Image::Ptr jpeg = jpeg_encoder.encode(img);
    rtp.encode(jpeg);
    //usleep(100000);

    std::string test = "out" + std::to_string(i) + ".jpg";
    FILE *fp = fopen(test.c_str(), "w");
    fwrite((uint8_t *)jpeg->getData(), jpeg->getSize(), 1, fp);
    fclose(fp);
    ++i;
  }


  cam->stop();
  return 0;
}
