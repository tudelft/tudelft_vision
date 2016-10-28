// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "encoding/encoder_jpeg.h"
#include "cam/cam_bebop.h"

int main(int argc, char *argv[])
{
  CamBebop cam;
  EncoderJPEG jpeg_encoder;
  //CamLinux cam("/dev/video1");
  cam.setOutput(Image::FMT_YUYV, 1080, 720);
  //cam.setCrop(114, 106, 2048, 3320);

  cam.start();
  uint8_t i = 0;
  char test[200];
  while(true) {
    std::shared_ptr<Image> img = cam.getImage();
    std::shared_ptr<Image> jpeg = jpeg_encoder.encode(img);

    sprintf(test, "out%d.jpg", ++i);
    FILE *fp = fopen(test, "w");
    fwrite((uint8_t *)jpeg->getData(), jpeg->getSize(), 1, fp);
    fclose(fp);
  }


  cam.stop();
  return 0;
}
