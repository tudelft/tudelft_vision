// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "encoding/encoder_jpeg.h"
#include "cam/cam_bebop.h"
#include "drivers/udpsocket.h"
#include "encoding/encoder_rtp.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
  CamBebop cam;
  //CamLinux cam("/dev/video0");
  EncoderJPEG jpeg_encoder;
  UDPSocket::Ptr udp = std::make_shared<UDPSocket>("192.168.42.2", 5000);
  EncoderRTP rtp(udp);

  cam.setOutput(Image::FMT_YUYV, 800, 600);
  //cam.setCrop(114, 106, 2048, 3320);

  cam.start();
  uint8_t i = 0;
  char test[200];
  while(true) {
    Image::Ptr img = cam.getImage();
    Image::Ptr jpeg = jpeg_encoder.encode(img);
    rtp.encode(jpeg);
    usleep(200000);

    /*sprintf(test, "out%d.jpg", ++i);
    FILE *fp = fopen(test, "w");
    fwrite((uint8_t *)jpeg->getData(), jpeg->getSize(), 1, fp);
    fclose(fp);*/
  }


  cam.stop();
  return 0;
}
