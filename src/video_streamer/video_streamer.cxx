// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include <tuv/tuv.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  Linux target;
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
