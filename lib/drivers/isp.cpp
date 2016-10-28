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

#include "isp.h"

#include <stdexcept>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <assert.h>
#include <unistd.h>


ISP::ISP(void) : Debug("Drivers::ISP") {

}
/**
 * Initialize ISP
 * @param[in] fd The video device connected to the ISP
 */
void ISP::configure(int fd) {
  /* Define the ISP bases */
  static const unsigned isp_bases[] = {
    AVI_ISP_CHAIN_BAYER_INTER,
    AVI_ISP_VLFORMAT_32TO40,
    AVI_ISP_PEDESTAL,
    AVI_ISP_GREEN_IMBALANCE,
    AVI_ISP_GREEN_IMBALANCE + AVI_ISP_GREEN_IMBALANCE_GREEN_RED_COEFF_MEM,
    AVI_ISP_GREEN_IMBALANCE + AVI_ISP_GREEN_IMBALANCE_GREEN_BLUE_COEFF_MEM,
    AVI_ISP_DEAD_PIXEL_CORRECTION + AVI_ISP_DEAD_PIXEL_CORRECTION_CFA,
    AVI_ISP_DEAD_PIXEL_CORRECTION + AVI_ISP_DEAD_PIXEL_CORRECTION_LIST_MEM,
    AVI_ISP_DENOISING,
    AVI_ISP_STATISTICS_BAYER,
    AVI_ISP_LENS_SHADING_CORRECTION,
    AVI_ISP_LENS_SHADING_CORRECTION + AVI_ISP_LENS_SHADING_CORRECTION_RED_COEFF_MEM,
    AVI_ISP_LENS_SHADING_CORRECTION + AVI_ISP_LENS_SHADING_CORRECTION_GREEN_COEFF_MEM,
    AVI_ISP_LENS_SHADING_CORRECTION + AVI_ISP_LENS_SHADING_CORRECTION_BLUE_COEFF_MEM,
    AVI_ISP_CHROMATIC_ABERRATION,
    AVI_ISP_BAYER,
    AVI_ISP_COLOR_CORRECTION,
    AVI_ISP_VLFORMAT_40TO32,
    AVI_ISP_GAMMA_CORRECTOR_CONF,  /* GAMMA conf */
    AVI_ISP_GAMMA_CORRECTOR_RY_LUT,
    AVI_ISP_GAMMA_CORRECTOR_GU_LUT,
    AVI_ISP_GAMMA_CORRECTOR_BV_LUT,
    AVI_ISP_CHROMA,  /* CHROMA */
    AVI_ISP_STATISTICS_YUV,  /* STATS YUV*/
    AVI_ISP_STATISTICS_YUV_AE_HISTOGRAM_Y,
    AVI_ISP_CHAIN_YUV_INTER,
    AVI_ISP_EDGE_ENHANCEMENT_COLOR_REDUCTION_FILTER + AVI_ISP_EDGE_ENHANCEMENT_COLOR_REDUCTION_FILTER_EE_KERNEL_COEFF,
    AVI_ISP_EDGE_ENHANCEMENT_COLOR_REDUCTION_FILTER + AVI_ISP_EDGE_ENHANCEMENT_COLOR_REDUCTION_FILTER_EE_LUT,
    AVI_ISP_I3D_LUT + AVI_ISP_I3D_LUT_CLIP_MODE,
    AVI_ISP_I3D_LUT + AVI_ISP_I3D_LUT_LUT_OUTSIDE,
    AVI_ISP_I3D_LUT + AVI_ISP_I3D_LUT_LUT_INSIDE,
    AVI_ISP_DROP,
  };

  // Open memory device
  devmem = open("/dev/mem", O_RDWR);
  if (devmem < 0) {
    throw std::runtime_error("Could not open /dev/mem (" + errnoString() + ")");
  }

  // Get the base
  avi_base = (unsigned long) mmap(NULL, AVI_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, devmem, AVI_BASE & ~AVI_MASK);
  if (avi_base == (unsigned long) MAP_FAILED) {
    close(devmem);
    throw std::runtime_error("Could not mmap /dev/mem (" + errnoString() + ")");
  }

  // Try to get the ISP offsets
  struct avi_isp_offsets off;
  if (ioctl(fd, AVI_ISP_IOGET_OFFSETS, &off) < 0) {
    munmap((void *) avi_base, AVI_SIZE);
    close(devmem);
    throw std::runtime_error("Could not get ISP offsets AVI_ISP_IOGET_OFFSETS (" + errnoString() + ")");
  }


  /* Compute all the sub-modules offsets */
  /* Chain Bayer */
  uint16_t i = 0;
  for (i = chain_bayer_inter ; i < gamma_corrector ; i++)
    offsets[i] = avi_base + isp_bases[i] + off.chain_bayer;

  offsets[gamma_corrector]        = avi_base + isp_bases[i++] + off.gamma_corrector;
  offsets[gamma_corrector_ry_lut] = avi_base + isp_bases[i++] + off.gamma_corrector;
  offsets[gamma_corrector_gu_lut] = avi_base + isp_bases[i++] + off.gamma_corrector;
  offsets[gamma_corrector_bv_lut] = avi_base + isp_bases[i++] + off.gamma_corrector;
  offsets[chroma]                 = avi_base + isp_bases[i++] + off.chroma;
  offsets[statistics_yuv]         = avi_base + isp_bases[i++] + off.statistics_yuv;
  offsets[statistics_yuv_ae_histogram_y] = avi_base + isp_bases[i++] + off.statistics_yuv;

  /* Chain YUV */
  for (i = chain_yuv_inter ; i < ISP_NODE_NR ; i++)
    offsets[i] = avi_base + isp_bases[i] + off.chain_yuv;

  /* Configure basics */
  reset();
}

/**
 * Reset the ISP with the default settings
 */
void ISP::reset(void) {
  /*
  • 0x0: RAW10 to RAW10
  • 0x1: RAW8 to RAW10
  • 0x2: RAW3x10 to RAW10
  • 0x3: RGB3x10 to RGB3x10
  */
  reg.vlformat_32to40.format.format = 0x0;
  avi_isp_vlformat_32to40_set_registers(&reg.vlformat_32to40);

  // Set default CFA (pixel order)
  config.cfa = 2;

  // Set the default bayer chain configuration
  config.bayer_ped      = false;
  config.bayer_grim     = false;
  config.bayer_rip      = false;
  config.bayer_denoise  = true;
  config.bayer_lsc      = false;
  config.bayer_ca       = false;
  config.bayer_demos    = true;
  config.bayer_colm     = true;

  // Set default pedestal
  config.pedestal_r  = 42;
  config.pedestal_gb = 42;
  config.pedestal_gr = 42;
  config.pedestal_b  = 42;

  // avi_isp_green_imbalance_set_registers(&isp_reg.green_imbalance);
  // avi_isp_green_imbalance_green_red_coeff_mem_set_registers(&isp_reg.grim_gr);
  // avi_isp_green_imbalance_green_blue_coeff_mem_set_registers(&isp_reg.grim_gb);
  // avi_isp_dead_pixel_correction_set_registers(&isp_reg.dead_pixel_correction);

  // Set default denoise vectors
  config.denoise_red    = {0, 0, 1, 2, 4, 6, 9, 13, 16, 18, 21, 23, 25, 26};
  config.denoise_green  = {0, 1, 1, 2, 4, 6, 8, 12, 15, 18, 20, 22, 24, 26};
  config.denoise_blue   = {0, 0, 1, 2, 4, 6, 9, 13, 16, 18, 21, 23, 25, 27};

  // avi_isp_statistics_bayer_set_registers(&isp_reg.statistics_bayer);
  // avi_isp_lens_shading_correction_set_registers(&isp_reg.lens_shading_correction);
  // avi_isp_lens_shading_correction_red_coeff_mem_set_registers(&isp_reg.lsc_red_coeffs);
  // avi_isp_lens_shading_correction_green_coeff_mem_set_registers(&isp_reg.lsc_green_coeffs);
  // avi_isp_lens_shading_correction_blue_coeff_mem_set_registers(&isp_reg.lsc_blue_coeffs);

  // Set default demosaicking parameters (Bayer to RGB)
  config.demos_threshold_low    = 25;
  config.demos_threshold_high   = 200;

  // Set default color correction parameters (RGB -> RGB)
  config.cc_matrix = {{  2.558105,  -1.562012,   0.264160},
                      { -0.257812,   1.274902,  -0.047852},
                      { -0.231934,  -1.391113,   3.438965}};
  config.cc_offin   = {0, 0, 0};
  config.cc_offout  = {0, 0, 0};
  config.cc_clipmin = {0, 0, 0};
  config.cc_clipmax = {1023, 1023, 1023};

  /*
  • 0x0: RAW10 to RAW8
  • 0x1: RAW10 to RAW10
  • 0x2: RAW10 to RAW3x10
  • 0x3: RGB3x10 to RGB3x10
  • 0x4: RGB3x10 to ARGB
  */
  reg.vlformat_40to32.format.format = 0x3;
  avi_isp_vlformat_40to32_set_registers(&reg.vlformat_40to32);

  // Set the default gamma corrector parameters (RGB 10bit -> RGB 8bit)
  config.gc_enable  = true;
  config.gc_palette = false;
  config.gc_10bit   = true;

  config.gc_rlut = {
      0,    1,    1,    1,    2,    2,    3,    3,    4,    4,    5,    5,    6,    7,    7,    8,    9,   10,   11,   12,
     13,   15,   16,   17,   19,   21,   23,   24,   26,   27,   29,   30,   32,   33,   34,   35,   36,   37,   38,   39,
     40,   41,   42,   43,   44,   45,   45,   46,   47,   48,   49,   49,   50,   51,   52,   52,   53,   54,   55,   55,
     56,   57,   57,   58,   59,   59,   60,   60,   61,   62,   62,   63,   63,   64,   65,   65,   66,   66,   67,   68,
     68,   69,   69,   70,   70,   71,   71,   72,   72,   73,   73,   74,   74,   75,   75,   76,   76,   77,   77,   78,
     78,   79,   79,   80,   80,   81,   81,   82,   82,   83,   83,   83,   84,   84,   85,   85,   86,   86,   87,   87,
     87,   88,   88,   89,   89,   90,   90,   90,   91,   91,   92,   92,   92,   93,   93,   94,   94,   94,   95,   95,
     96,   96,   96,   97,   97,   98,   98,   98,   99,   99,   99,  100,  100,  101,  101,  101,  102,  102,  102,  103,
    103,  103,  104,  104,  105,  105,  105,  106,  106,  106,  107,  107,  107,  108,  108,  108,  109,  109,  109,  110,
    110,  111,  111,  111,  112,  112,  112,  113,  113,  113,  114,  114,  114,  114,  115,  115,  115,  116,  116,  116,
    117,  117,  117,  118,  118,  118,  119,  119,  119,  120,  120,  120,  121,  121,  121,  121,  122,  122,  122,  123,
    123,  123,  124,  124,  124,  125,  125,  125,  125,  126,  126,  126,  127,  127,  127,  127,  128,  128,  128,  129,
    129,  129,  130,  130,  130,  130,  131,  131,  131,  132,  132,  132,  132,  133,  133,  133,  133,  134,  134,  134,
    135,  135,  135,  135,  136,  136,  136,  137,  137,  137,  137,  138,  138,  138,  138,  139,  139,  139,  139,  140,
    140,  140,  140,  141,  141,  141,  142,  142,  142,  142,  143,  143,  143,  143,  144,  144,  144,  144,  145,  145,
    145,  145,  146,  146,  146,  146,  147,  147,  147,  147,  148,  148,  148,  148,  149,  149,  149,  149,  149,  150,
    150,  150,  150,  151,  151,  151,  151,  152,  152,  152,  152,  153,  153,  153,  153,  154,  154,  154,  154,  154,
    155,  155,  155,  155,  156,  156,  156,  156,  156,  157,  157,  157,  157,  158,  158,  158,  158,  159,  159,  159,
    159,  159,  160,  160,  160,  160,  160,  161,  161,  161,  161,  162,  162,  162,  162,  162,  163,  163,  163,  163,
    164,  164,  164,  164,  164,  165,  165,  165,  165,  165,  166,  166,  166,  166,  166,  167,  167,  167,  167,  168,
    168,  168,  168,  168,  169,  169,  169,  169,  169,  170,  170,  170,  170,  170,  171,  171,  171,  171,  171,  172,
    172,  172,  172,  172,  173,  173,  173,  173,  173,  174,  174,  174,  174,  174,  175,  175,  175,  175,  175,  175,
    176,  176,  176,  176,  176,  177,  177,  177,  177,  177,  178,  178,  178,  178,  178,  179,  179,  179,  179,  179,
    179,  180,  180,  180,  180,  180,  181,  181,  181,  181,  181,  182,  182,  182,  182,  182,  182,  183,  183,  183,
    183,  183,  184,  184,  184,  184,  184,  184,  185,  185,  185,  185,  185,  185,  186,  186,  186,  186,  186,  187,
    187,  187,  187,  187,  187,  188,  188,  188,  188,  188,  188,  189,  189,  189,  189,  189,  190,  190,  190,  190,
    190,  190,  191,  191,  191,  191,  191,  191,  192,  192,  192,  192,  192,  192,  193,  193,  193,  193,  193,  193,
    194,  194,  194,  194,  194,  194,  195,  195,  195,  195,  195,  195,  196,  196,  196,  196,  196,  196,  197,  197,
    197,  197,  197,  197,  197,  198,  198,  198,  198,  198,  198,  199,  199,  199,  199,  199,  199,  200,  200,  200,
    200,  200,  200,  201,  201,  201,  201,  201,  201,  201,  202,  202,  202,  202,  202,  202,  203,  203,  203,  203,
    203,  203,  203,  204,  204,  204,  204,  204,  204,  205,  205,  205,  205,  205,  205,  205,  206,  206,  206,  206,
    206,  206,  206,  207,  207,  207,  207,  207,  207,  208,  208,  208,  208,  208,  208,  208,  209,  209,  209,  209,
    209,  209,  209,  210,  210,  210,  210,  210,  210,  210,  211,  211,  211,  211,  211,  211,  211,  212,  212,  212,
    212,  212,  212,  212,  213,  213,  213,  213,  213,  213,  213,  214,  214,  214,  214,  214,  214,  214,  215,  215,
    215,  215,  215,  215,  215,  216,  216,  216,  216,  216,  216,  216,  216,  217,  217,  217,  217,  217,  217,  217,
    218,  218,  218,  218,  218,  218,  218,  219,  219,  219,  219,  219,  219,  219,  219,  220,  220,  220,  220,  220,
    220,  220,  220,  221,  221,  221,  221,  221,  221,  221,  222,  222,  222,  222,  222,  222,  222,  222,  223,  223,
    223,  223,  223,  223,  223,  223,  224,  224,  224,  224,  224,  224,  224,  225,  225,  225,  225,  225,  225,  225,
    225,  226,  226,  226,  226,  226,  226,  226,  226,  227,  227,  227,  227,  227,  227,  227,  227,  228,  228,  228,
    228,  228,  228,  228,  228,  229,  229,  229,  229,  229,  229,  229,  229,  230,  230,  230,  230,  230,  230,  230,
    230,  230,  231,  231,  231,  231,  231,  231,  231,  231,  232,  232,  232,  232,  232,  232,  232,  232,  233,  233,
    233,  233,  233,  233,  233,  233,  233,  234,  234,  234,  234,  234,  234,  234,  234,  235,  235,  235,  235,  235,
    235,  235,  235,  235,  236,  236,  236,  236,  236,  236,  236,  236,  236,  237,  237,  237,  237,  237,  237,  237,
    237,  238,  238,  238,  238,  238,  238,  238,  238,  238,  239,  239,  239,  239,  239,  239,  239,  239,  239,  240,
    240,  240,  240,  240,  240,  240,  240,  240,  241,  241,  241,  241,  241,  241,  241,  241,  241,  242,  242,  242,
    242,  242,  242,  242,  242,  242,  243,  243,  243,  243,  243,  243,  243,  243,  243,  244,  244,  244,  244,  244,
    244,  244,  244,  244,  245,  245,  245,  245,  245,  245,  245,  245,  245,  245,  246,  246,  246,  246,  246,  246,
    246,  246,  246,  247,  247,  247,  247,  247,  247,  247,  247,  247,  248,  248,  248,  248,  248,  248,  248,  248,
    248,  248,  249,  249,  249,  249,  249,  249,  249,  249,  249,  250,  250,  250,  250,  250,  250,  250,  250,  250,
    251,  251,  251,  251,  251,  251,  251,  251,  251,  251,  252,  252,  252,  252,  252,  252,  252,  252,  252,  252,
    253,  253,  253,  253,  253,  253,  253,  253,  253,  254,  254,  254,  254,  254,  254,  254,  254,  254,  254,  255,
    255,  255,  255,  255
  };

  config.gc_glut = {
      0,    1,    1,    1,    2,    2,    3,    3,    4,    4,    5,    5,    6,    7,    7,    8,    9,   10,   11,   12,
     13,   15,   16,   17,   19,   21,   23,   24,   26,   27,   29,   30,   32,   33,   34,   35,   36,   37,   38,   39,
     40,   41,   42,   43,   44,   45,   45,   46,   47,   48,   49,   49,   50,   51,   52,   52,   53,   54,   55,   55,
     56,   57,   57,   58,   59,   59,   60,   60,   61,   62,   62,   63,   63,   64,   65,   65,   66,   66,   67,   68,
     68,   69,   69,   70,   70,   71,   71,   72,   72,   73,   73,   74,   74,   75,   75,   76,   76,   77,   77,   78,
     78,   79,   79,   80,   80,   81,   81,   82,   82,   83,   83,   83,   84,   84,   85,   85,   86,   86,   87,   87,
     87,   88,   88,   89,   89,   90,   90,   90,   91,   91,   92,   92,   92,   93,   93,   94,   94,   94,   95,   95,
     96,   96,   96,   97,   97,   98,   98,   98,   99,   99,   99,  100,  100,  101,  101,  101,  102,  102,  102,  103,
    103,  103,  104,  104,  105,  105,  105,  106,  106,  106,  107,  107,  107,  108,  108,  108,  109,  109,  109,  110,
    110,  111,  111,  111,  112,  112,  112,  113,  113,  113,  114,  114,  114,  114,  115,  115,  115,  116,  116,  116,
    117,  117,  117,  118,  118,  118,  119,  119,  119,  120,  120,  120,  121,  121,  121,  121,  122,  122,  122,  123,
    123,  123,  124,  124,  124,  125,  125,  125,  125,  126,  126,  126,  127,  127,  127,  127,  128,  128,  128,  129,
    129,  129,  130,  130,  130,  130,  131,  131,  131,  132,  132,  132,  132,  133,  133,  133,  133,  134,  134,  134,
    135,  135,  135,  135,  136,  136,  136,  137,  137,  137,  137,  138,  138,  138,  138,  139,  139,  139,  139,  140,
    140,  140,  140,  141,  141,  141,  142,  142,  142,  142,  143,  143,  143,  143,  144,  144,  144,  144,  145,  145,
    145,  145,  146,  146,  146,  146,  147,  147,  147,  147,  148,  148,  148,  148,  149,  149,  149,  149,  149,  150,
    150,  150,  150,  151,  151,  151,  151,  152,  152,  152,  152,  153,  153,  153,  153,  154,  154,  154,  154,  154,
    155,  155,  155,  155,  156,  156,  156,  156,  156,  157,  157,  157,  157,  158,  158,  158,  158,  159,  159,  159,
    159,  159,  160,  160,  160,  160,  160,  161,  161,  161,  161,  162,  162,  162,  162,  162,  163,  163,  163,  163,
    164,  164,  164,  164,  164,  165,  165,  165,  165,  165,  166,  166,  166,  166,  166,  167,  167,  167,  167,  168,
    168,  168,  168,  168,  169,  169,  169,  169,  169,  170,  170,  170,  170,  170,  171,  171,  171,  171,  171,  172,
    172,  172,  172,  172,  173,  173,  173,  173,  173,  174,  174,  174,  174,  174,  175,  175,  175,  175,  175,  175,
    176,  176,  176,  176,  176,  177,  177,  177,  177,  177,  178,  178,  178,  178,  178,  179,  179,  179,  179,  179,
    179,  180,  180,  180,  180,  180,  181,  181,  181,  181,  181,  182,  182,  182,  182,  182,  182,  183,  183,  183,
    183,  183,  184,  184,  184,  184,  184,  184,  185,  185,  185,  185,  185,  185,  186,  186,  186,  186,  186,  187,
    187,  187,  187,  187,  187,  188,  188,  188,  188,  188,  188,  189,  189,  189,  189,  189,  190,  190,  190,  190,
    190,  190,  191,  191,  191,  191,  191,  191,  192,  192,  192,  192,  192,  192,  193,  193,  193,  193,  193,  193,
    194,  194,  194,  194,  194,  194,  195,  195,  195,  195,  195,  195,  196,  196,  196,  196,  196,  196,  197,  197,
    197,  197,  197,  197,  197,  198,  198,  198,  198,  198,  198,  199,  199,  199,  199,  199,  199,  200,  200,  200,
    200,  200,  200,  201,  201,  201,  201,  201,  201,  201,  202,  202,  202,  202,  202,  202,  203,  203,  203,  203,
    203,  203,  203,  204,  204,  204,  204,  204,  204,  205,  205,  205,  205,  205,  205,  205,  206,  206,  206,  206,
    206,  206,  206,  207,  207,  207,  207,  207,  207,  208,  208,  208,  208,  208,  208,  208,  209,  209,  209,  209,
    209,  209,  209,  210,  210,  210,  210,  210,  210,  210,  211,  211,  211,  211,  211,  211,  211,  212,  212,  212,
    212,  212,  212,  212,  213,  213,  213,  213,  213,  213,  213,  214,  214,  214,  214,  214,  214,  214,  215,  215,
    215,  215,  215,  215,  215,  216,  216,  216,  216,  216,  216,  216,  216,  217,  217,  217,  217,  217,  217,  217,
    218,  218,  218,  218,  218,  218,  218,  219,  219,  219,  219,  219,  219,  219,  219,  220,  220,  220,  220,  220,
    220,  220,  220,  221,  221,  221,  221,  221,  221,  221,  222,  222,  222,  222,  222,  222,  222,  222,  223,  223,
    223,  223,  223,  223,  223,  223,  224,  224,  224,  224,  224,  224,  224,  225,  225,  225,  225,  225,  225,  225,
    225,  226,  226,  226,  226,  226,  226,  226,  226,  227,  227,  227,  227,  227,  227,  227,  227,  228,  228,  228,
    228,  228,  228,  228,  228,  229,  229,  229,  229,  229,  229,  229,  229,  230,  230,  230,  230,  230,  230,  230,
    230,  230,  231,  231,  231,  231,  231,  231,  231,  231,  232,  232,  232,  232,  232,  232,  232,  232,  233,  233,
    233,  233,  233,  233,  233,  233,  233,  234,  234,  234,  234,  234,  234,  234,  234,  235,  235,  235,  235,  235,
    235,  235,  235,  235,  236,  236,  236,  236,  236,  236,  236,  236,  236,  237,  237,  237,  237,  237,  237,  237,
    237,  238,  238,  238,  238,  238,  238,  238,  238,  238,  239,  239,  239,  239,  239,  239,  239,  239,  239,  240,
    240,  240,  240,  240,  240,  240,  240,  240,  241,  241,  241,  241,  241,  241,  241,  241,  241,  242,  242,  242,
    242,  242,  242,  242,  242,  242,  243,  243,  243,  243,  243,  243,  243,  243,  243,  244,  244,  244,  244,  244,
    244,  244,  244,  244,  245,  245,  245,  245,  245,  245,  245,  245,  245,  245,  246,  246,  246,  246,  246,  246,
    246,  246,  246,  247,  247,  247,  247,  247,  247,  247,  247,  247,  248,  248,  248,  248,  248,  248,  248,  248,
    248,  248,  249,  249,  249,  249,  249,  249,  249,  249,  249,  250,  250,  250,  250,  250,  250,  250,  250,  250,
    251,  251,  251,  251,  251,  251,  251,  251,  251,  251,  252,  252,  252,  252,  252,  252,  252,  252,  252,  252,
    253,  253,  253,  253,  253,  253,  253,  253,  253,  254,  254,  254,  254,  254,  254,  254,  254,  254,  254,  255,
    255,  255,  255,  255
  };

  config.gc_blut = {
      0,    1,    1,    1,    2,    2,    3,    3,    4,    4,    5,    5,    6,    7,    7,    8,    9,   10,   11,   12,
     13,   15,   16,   17,   19,   21,   23,   24,   26,   27,   29,   30,   32,   33,   34,   35,   36,   37,   38,   39,
     40,   41,   42,   43,   44,   45,   45,   46,   47,   48,   49,   49,   50,   51,   52,   52,   53,   54,   55,   55,
     56,   57,   57,   58,   59,   59,   60,   60,   61,   62,   62,   63,   63,   64,   65,   65,   66,   66,   67,   68,
     68,   69,   69,   70,   70,   71,   71,   72,   72,   73,   73,   74,   74,   75,   75,   76,   76,   77,   77,   78,
     78,   79,   79,   80,   80,   81,   81,   82,   82,   83,   83,   83,   84,   84,   85,   85,   86,   86,   87,   87,
     87,   88,   88,   89,   89,   90,   90,   90,   91,   91,   92,   92,   92,   93,   93,   94,   94,   94,   95,   95,
     96,   96,   96,   97,   97,   98,   98,   98,   99,   99,   99,  100,  100,  101,  101,  101,  102,  102,  102,  103,
    103,  103,  104,  104,  105,  105,  105,  106,  106,  106,  107,  107,  107,  108,  108,  108,  109,  109,  109,  110,
    110,  111,  111,  111,  112,  112,  112,  113,  113,  113,  114,  114,  114,  114,  115,  115,  115,  116,  116,  116,
    117,  117,  117,  118,  118,  118,  119,  119,  119,  120,  120,  120,  121,  121,  121,  121,  122,  122,  122,  123,
    123,  123,  124,  124,  124,  125,  125,  125,  125,  126,  126,  126,  127,  127,  127,  127,  128,  128,  128,  129,
    129,  129,  130,  130,  130,  130,  131,  131,  131,  132,  132,  132,  132,  133,  133,  133,  133,  134,  134,  134,
    135,  135,  135,  135,  136,  136,  136,  137,  137,  137,  137,  138,  138,  138,  138,  139,  139,  139,  139,  140,
    140,  140,  140,  141,  141,  141,  142,  142,  142,  142,  143,  143,  143,  143,  144,  144,  144,  144,  145,  145,
    145,  145,  146,  146,  146,  146,  147,  147,  147,  147,  148,  148,  148,  148,  149,  149,  149,  149,  149,  150,
    150,  150,  150,  151,  151,  151,  151,  152,  152,  152,  152,  153,  153,  153,  153,  154,  154,  154,  154,  154,
    155,  155,  155,  155,  156,  156,  156,  156,  156,  157,  157,  157,  157,  158,  158,  158,  158,  159,  159,  159,
    159,  159,  160,  160,  160,  160,  160,  161,  161,  161,  161,  162,  162,  162,  162,  162,  163,  163,  163,  163,
    164,  164,  164,  164,  164,  165,  165,  165,  165,  165,  166,  166,  166,  166,  166,  167,  167,  167,  167,  168,
    168,  168,  168,  168,  169,  169,  169,  169,  169,  170,  170,  170,  170,  170,  171,  171,  171,  171,  171,  172,
    172,  172,  172,  172,  173,  173,  173,  173,  173,  174,  174,  174,  174,  174,  175,  175,  175,  175,  175,  175,
    176,  176,  176,  176,  176,  177,  177,  177,  177,  177,  178,  178,  178,  178,  178,  179,  179,  179,  179,  179,
    179,  180,  180,  180,  180,  180,  181,  181,  181,  181,  181,  182,  182,  182,  182,  182,  182,  183,  183,  183,
    183,  183,  184,  184,  184,  184,  184,  184,  185,  185,  185,  185,  185,  185,  186,  186,  186,  186,  186,  187,
    187,  187,  187,  187,  187,  188,  188,  188,  188,  188,  188,  189,  189,  189,  189,  189,  190,  190,  190,  190,
    190,  190,  191,  191,  191,  191,  191,  191,  192,  192,  192,  192,  192,  192,  193,  193,  193,  193,  193,  193,
    194,  194,  194,  194,  194,  194,  195,  195,  195,  195,  195,  195,  196,  196,  196,  196,  196,  196,  197,  197,
    197,  197,  197,  197,  197,  198,  198,  198,  198,  198,  198,  199,  199,  199,  199,  199,  199,  200,  200,  200,
    200,  200,  200,  201,  201,  201,  201,  201,  201,  201,  202,  202,  202,  202,  202,  202,  203,  203,  203,  203,
    203,  203,  203,  204,  204,  204,  204,  204,  204,  205,  205,  205,  205,  205,  205,  205,  206,  206,  206,  206,
    206,  206,  206,  207,  207,  207,  207,  207,  207,  208,  208,  208,  208,  208,  208,  208,  209,  209,  209,  209,
    209,  209,  209,  210,  210,  210,  210,  210,  210,  210,  211,  211,  211,  211,  211,  211,  211,  212,  212,  212,
    212,  212,  212,  212,  213,  213,  213,  213,  213,  213,  213,  214,  214,  214,  214,  214,  214,  214,  215,  215,
    215,  215,  215,  215,  215,  216,  216,  216,  216,  216,  216,  216,  216,  217,  217,  217,  217,  217,  217,  217,
    218,  218,  218,  218,  218,  218,  218,  219,  219,  219,  219,  219,  219,  219,  219,  220,  220,  220,  220,  220,
    220,  220,  220,  221,  221,  221,  221,  221,  221,  221,  222,  222,  222,  222,  222,  222,  222,  222,  223,  223,
    223,  223,  223,  223,  223,  223,  224,  224,  224,  224,  224,  224,  224,  225,  225,  225,  225,  225,  225,  225,
    225,  226,  226,  226,  226,  226,  226,  226,  226,  227,  227,  227,  227,  227,  227,  227,  227,  228,  228,  228,
    228,  228,  228,  228,  228,  229,  229,  229,  229,  229,  229,  229,  229,  230,  230,  230,  230,  230,  230,  230,
    230,  230,  231,  231,  231,  231,  231,  231,  231,  231,  232,  232,  232,  232,  232,  232,  232,  232,  233,  233,
    233,  233,  233,  233,  233,  233,  233,  234,  234,  234,  234,  234,  234,  234,  234,  235,  235,  235,  235,  235,
    235,  235,  235,  235,  236,  236,  236,  236,  236,  236,  236,  236,  236,  237,  237,  237,  237,  237,  237,  237,
    237,  238,  238,  238,  238,  238,  238,  238,  238,  238,  239,  239,  239,  239,  239,  239,  239,  239,  239,  240,
    240,  240,  240,  240,  240,  240,  240,  240,  241,  241,  241,  241,  241,  241,  241,  241,  241,  242,  242,  242,
    242,  242,  242,  242,  242,  242,  243,  243,  243,  243,  243,  243,  243,  243,  243,  244,  244,  244,  244,  244,
    244,  244,  244,  244,  245,  245,  245,  245,  245,  245,  245,  245,  245,  245,  246,  246,  246,  246,  246,  246,
    246,  246,  246,  247,  247,  247,  247,  247,  247,  247,  247,  247,  248,  248,  248,  248,  248,  248,  248,  248,
    248,  248,  249,  249,  249,  249,  249,  249,  249,  249,  249,  250,  250,  250,  250,  250,  250,  250,  250,  250,
    251,  251,  251,  251,  251,  251,  251,  251,  251,  251,  252,  252,  252,  252,  252,  252,  252,  252,  252,  252,
    253,  253,  253,  253,  253,  253,  253,  253,  253,  254,  254,  254,  254,  254,  254,  254,  254,  254,  254,  255,
    255,  255,  255,  255
  };

  // Set the default chroma parameters (RGB 8bit -> YUV 8bit)
  config.csc_matrix = {{  0.2568359375,   0.5039062500,   0.09814453125 },
                       {  0.4394531250,  -0.3676757813,  -0.07128906250 },
                       { -0.1484375000,  -0.2910156250,   0.43945312500 }};
  config.csc_offin   = {0, 0, 0};
  config.csc_offout  = {16, 128, 128};
  config.csc_clipmin = {16, 16, 16};
  config.csc_clipmax = {235, 240, 240};

  // Set the default YUV chain configuration
  config.yuv_ee_crf   = false;
  config.yuv_i3d_lut  = false;
  config.yuv_drop     = false;

  // avi_isp_statistics_yuv_set_registers(&isp_reg.statistics_yuv);
  // avi_isp_edge_enhancement_color_reduction_filter_set_registers(&isp_reg.eecrf);
  // avi_isp_edge_enhancement_color_reduction_filter_ee_lut_set_registers(&isp_reg.eecrf_lut);

  /* Send all configurations */
  sendBayerChain();
  sendPedestal();
  sendDenoising();
  sendDemosaicking();
  sendColorCorrection();
  sendGammaCorrector();
  sendGammaCorrectorLUT();
  sendColorSpaceConversion();
  sendYUVChain();
}

/**
 * Set the resolution of the ISP
 * This reconfigures the registers which are based on the camera resolution
 * @param[in] width The camera width
 * @param[in] height The camera height
 */
void ISP::setResolution(uint32_t width, uint32_t height) {

}

/**
 * Enable or disable parts of the bayer ISP chain
 * @param[in] ped Enable the pedestal filter
 * @param[in] grim Enable the green imbalance corrector
 * @param[in] rip Enable the dead pixel correction (TODO: Check if correct)
 * @param[in] denoise Enable denoising
 * @param[in] lsc Enable lens shading correction
 * @param[in] ca Enable chromatic aberration
 * @param[in] demos Enable demosaicking (Bayer to RGB conversion)
 * @param[in] conv Enable color matrix correction (RGB->RGB, note this is different from color space conversion)
 */
void ISP::setBayerChain(bool ped, bool grim, bool rip, bool denoise, bool lsc, bool ca, bool demos, bool colm) {
  config.bayer_ped      = ped;
  config.bayer_grim     = grim;
  config.bayer_rip      = rip;
  config.bayer_denoise  = denoise;
  config.bayer_lsc      = lsc;
  config.bayer_ca       = ca;
  config.bayer_demos    = demos;
  config.bayer_colm     = colm;
  sendBayerChain();
}

/**
 * Send the bayer chain configuration
 */
void ISP::sendBayerChain(void) {
  // Set the registers
  reg.bayer_inter.module_bypass.pedestal_bypass     = config.bayer_ped? 0:1;
  reg.bayer_inter.module_bypass.grim_bypass         = config.bayer_grim? 0:1;
  reg.bayer_inter.module_bypass.rip_bypass          = config.bayer_rip? 0:1;
  reg.bayer_inter.module_bypass.denoise_bypass      = config.bayer_denoise? 0:1;
  reg.bayer_inter.module_bypass.lsc_bypass          = config.bayer_lsc? 0:1;
  reg.bayer_inter.module_bypass.chroma_aber_bypass  = config.bayer_ca? 0:1;
  reg.bayer_inter.module_bypass.bayer_bypass        = config.bayer_demos? 0:1;
  reg.bayer_inter.module_bypass.color_matrix_bypass = config.bayer_colm? 0:1;

  // Send the register
  avi_isp_chain_bayer_inter_set_registers(&reg.bayer_inter);
}

/**
 * Set the pedestal value
 * This substracts a value from each pixel. Values are clipped at [0 1023].
 * @param[in] val_r The pedestal value for the red channel
 * @param[in] val_gb The pedestal value for the green-blue channel
 * @param[in] val_gr The pedestal value for the green-red channel
 * @param[in] val_b The pedestal value for the blue channel
 */
void ISP::setPedestal(uint16_t val_r, uint16_t val_gb, uint16_t val_gr, uint16_t val_b) {
  config.pedestal_r  = val_r;
  config.pedestal_gb = val_gb;
  config.pedestal_gr = val_gr;
  config.pedestal_b  = val_b;
  sendPedestal();
}

/**
 * Set the pedestal value for all channels
 * This substracts a value from each pixel. Values are clipped at [0 1023].
 * @param[in] val The pedestal value for all channels
 */
void ISP::setPedestal(uint16_t val) {
  setPedestal(val, val, val, val);
}

/**
 * Send the pedestal configuration
 */
void ISP::sendPedestal(void) {
  // Set the registers
  reg.pedestal.cfa.cfa       = config.cfa;
  reg.pedestal.sub_r.sub_r   = config.pedestal_r;
  reg.pedestal.sub_gb.sub_gb = config.pedestal_gb;
  reg.pedestal.sub_gr.sub_gr = config.pedestal_gr;
  reg.pedestal.sub_b.sub_b   = config.pedestal_b;

  // Send the register
  avi_isp_pedestal_set_registers(&reg.pedestal);
}

/**
 * Set the denoising coefficients
 * It applies a 7x7 sigma filter on each channel (note that the green imbalance is done before,
 * thus there is only one channel for the green) Its abcissas are:
 * 0, 4, 8, 16, 32, 64, 128, 256, 384, 512, 640, 768, 896 (and implicitely 1024).
 * The ordinates are defined in the arguments (consisting of 14 values)
 * @param[in] red The coefficients for the red channel
 * @param[in] green The coefficients for the green channel
 * @param[in] blue The coefficients for the blue channel
 */
void ISP::setDenoising(std::vector<uint8_t> red, std::vector<uint8_t> green, std::vector<uint8_t> blue) {
  config.denoise_red    = red;
  config.denoise_green  = green;
  config.denoise_blue   = blue;
  sendDenoising();
}

/**
 * Send the denoising configuration
 */
void ISP::sendDenoising(void) {
  assert(config.denoise_red.size() == 14);
  assert(config.denoise_green.size() == 14);
  assert(config.denoise_blue.size() == 14);

  // Set the cfa
  reg.denoising.cfa.cfa = config.cfa;

  // Set the red coefficients
  reg.denoising.lumocoeff_r_03_00.lumocoeff_r_00 = config.denoise_red.at( 0);
  reg.denoising.lumocoeff_r_03_00.lumocoeff_r_01 = config.denoise_red.at( 1);
  reg.denoising.lumocoeff_r_03_00.lumocoeff_r_02 = config.denoise_red.at( 2);
  reg.denoising.lumocoeff_r_03_00.lumocoeff_r_03 = config.denoise_red.at( 3);
  reg.denoising.lumocoeff_r_07_04.lumocoeff_r_04 = config.denoise_red.at( 4);
  reg.denoising.lumocoeff_r_07_04.lumocoeff_r_05 = config.denoise_red.at( 5);
  reg.denoising.lumocoeff_r_07_04.lumocoeff_r_06 = config.denoise_red.at( 6);
  reg.denoising.lumocoeff_r_07_04.lumocoeff_r_07 = config.denoise_red.at( 7);
  reg.denoising.lumocoeff_r_11_08.lumocoeff_r_08 = config.denoise_red.at( 8);
  reg.denoising.lumocoeff_r_11_08.lumocoeff_r_09 = config.denoise_red.at( 9);
  reg.denoising.lumocoeff_r_11_08.lumocoeff_r_10 = config.denoise_red.at(10);
  reg.denoising.lumocoeff_r_11_08.lumocoeff_r_11 = config.denoise_red.at(11);
  reg.denoising.lumocoeff_r_13_12.lumocoeff_r_12 = config.denoise_red.at(12);
  reg.denoising.lumocoeff_r_13_12.lumocoeff_r_13 = config.denoise_red.at(13);

  // Set the green coefficients
  reg.denoising.lumocoeff_g_03_00.lumocoeff_g_00 = config.denoise_green.at( 0);
  reg.denoising.lumocoeff_g_03_00.lumocoeff_g_01 = config.denoise_green.at( 1);
  reg.denoising.lumocoeff_g_03_00.lumocoeff_g_02 = config.denoise_green.at( 2);
  reg.denoising.lumocoeff_g_03_00.lumocoeff_g_03 = config.denoise_green.at( 3);
  reg.denoising.lumocoeff_g_07_04.lumocoeff_g_04 = config.denoise_green.at( 4);
  reg.denoising.lumocoeff_g_07_04.lumocoeff_g_05 = config.denoise_green.at( 5);
  reg.denoising.lumocoeff_g_07_04.lumocoeff_g_06 = config.denoise_green.at( 6);
  reg.denoising.lumocoeff_g_07_04.lumocoeff_g_07 = config.denoise_green.at( 7);
  reg.denoising.lumocoeff_g_11_08.lumocoeff_g_08 = config.denoise_green.at( 8);
  reg.denoising.lumocoeff_g_11_08.lumocoeff_g_09 = config.denoise_green.at( 9);
  reg.denoising.lumocoeff_g_11_08.lumocoeff_g_10 = config.denoise_green.at(10);
  reg.denoising.lumocoeff_g_11_08.lumocoeff_g_11 = config.denoise_green.at(11);
  reg.denoising.lumocoeff_g_13_12.lumocoeff_g_12 = config.denoise_green.at(12);
  reg.denoising.lumocoeff_g_13_12.lumocoeff_g_13 = config.denoise_green.at(13);

  // Set the blue coefficients
  reg.denoising.lumocoeff_b_03_00.lumocoeff_b_00 = config.denoise_blue.at( 0);
  reg.denoising.lumocoeff_b_03_00.lumocoeff_b_01 = config.denoise_blue.at( 1);
  reg.denoising.lumocoeff_b_03_00.lumocoeff_b_02 = config.denoise_blue.at( 2);
  reg.denoising.lumocoeff_b_03_00.lumocoeff_b_03 = config.denoise_blue.at( 3);
  reg.denoising.lumocoeff_b_07_04.lumocoeff_b_04 = config.denoise_blue.at( 4);
  reg.denoising.lumocoeff_b_07_04.lumocoeff_b_05 = config.denoise_blue.at( 5);
  reg.denoising.lumocoeff_b_07_04.lumocoeff_b_06 = config.denoise_blue.at( 6);
  reg.denoising.lumocoeff_b_07_04.lumocoeff_b_07 = config.denoise_blue.at( 7);
  reg.denoising.lumocoeff_b_11_08.lumocoeff_b_08 = config.denoise_blue.at( 8);
  reg.denoising.lumocoeff_b_11_08.lumocoeff_b_09 = config.denoise_blue.at( 9);
  reg.denoising.lumocoeff_b_11_08.lumocoeff_b_10 = config.denoise_blue.at(10);
  reg.denoising.lumocoeff_b_11_08.lumocoeff_b_11 = config.denoise_blue.at(11);
  reg.denoising.lumocoeff_b_13_12.lumocoeff_b_12 = config.denoise_blue.at(12);
  reg.denoising.lumocoeff_b_13_12.lumocoeff_b_13 = config.denoise_blue.at(13);

  // Send the register
  avi_isp_denoising_set_registers(&reg.denoising);
}

/**
 * Set the demosaicking thresholds
 * Demosaicking is done with the Hamilton-Adams algorithm and it reconstructs
 * RGB 4:4:4, 10 bits per component from a Bayer CFA image.
 * @param[in] low The lower threshold
 * @param[in] high The upper threshold
 */
void ISP::setDemosaicking(uint16_t low, uint16_t high) {
  config.demos_threshold_low  = low;
  config.demos_threshold_high = high;
  sendDemosaicking();
}

/**
 * Send the demosaicking configuration
 */
void ISP::sendDemosaicking(void) {
  // Set the registers
  reg.bayer.cfa.cfa = config.cfa;
  reg.bayer.threshold_1.threshold_1 = config.demos_threshold_low;
  reg.bayer.threshold_2.threshold_2 = config.demos_threshold_high;

  // Send the register
  avi_isp_bayer_set_registers(&reg.bayer);
}

/**
 * Set color correction from RGB to RGB using a 3x3 matrix
 * - First it adds the offin to each of the 3 color channels (RGB).
 * - Then it performs a per pixel vector * matrix multiplication (3x3 matrix)
 * - Then it adds the offout to each of the 3 color channels (RGB)
 * - As last it performs a clipping on each pixel using clipmin and clipmax (which are set per color)
 * @param[in] matrix A float 3x3 matrix which each RGB pixel is multiplied with
 * @param[in] offin Offsets added to the input (for each color RGB)
 * @param[in] offout Offsets added to the output (for each color RGB)
 * @param[in] clipmin Clipping minimum which is done at the output (for each color RGB)
 * @param[in] clipmax Clipping maximum which is done at the output (for each color RGB)
 */
void ISP::setColorCorrection(std::vector<std::vector<float>> matrix, std::vector<uint32_t> offin, std::vector<uint32_t> offout, std::vector<uint32_t> clipmin, std::vector<uint32_t> clipmax) {
  config.cc_matrix  = matrix;
  config.cc_offin   = offin;
  config.cc_offout  = offout;
  config.cc_clipmin = clipmin;
  config.cc_clipmax = clipmax;
  sendColorCorrection();
}

/**
 * Send the color correction configuration
 */
void ISP::sendColorCorrection(void) {
  assert(config.cc_offin.size() == 3);
  assert(config.cc_offout.size() == 3);
  assert(config.cc_clipmin.size() == 3);
  assert(config.cc_clipmax.size() == 3);
  assert(config.cc_matrix.size() == 3);
  assert(config.cc_matrix.at(0).size() == 3);
  assert(config.cc_matrix.at(1).size() == 3);
  assert(config.cc_matrix.at(2).size() == 3);

  // Set the registers
  reg.color_correction.coeff_01_00.coeff_00 = floatToQ2_11(config.cc_matrix.at(0).at(0)); //Q2.11
  reg.color_correction.coeff_01_00.coeff_01 = floatToQ2_11(config.cc_matrix.at(0).at(1)); //Q2.11
  reg.color_correction.coeff_10_02.coeff_02 = floatToQ2_11(config.cc_matrix.at(0).at(2)); //Q2.11
  reg.color_correction.coeff_10_02.coeff_10 = floatToQ2_11(config.cc_matrix.at(1).at(0)); //Q2.11
  reg.color_correction.coeff_12_11.coeff_11 = floatToQ2_11(config.cc_matrix.at(1).at(1)); //Q2.11
  reg.color_correction.coeff_12_11.coeff_12 = floatToQ2_11(config.cc_matrix.at(1).at(2)); //Q2.11
  reg.color_correction.coeff_21_20.coeff_20 = floatToQ2_11(config.cc_matrix.at(2).at(0)); //Q2.11
  reg.color_correction.coeff_21_20.coeff_21 = floatToQ2_11(config.cc_matrix.at(2).at(1)); //Q2.11
  reg.color_correction.coeff_22.coeff_22    = floatToQ2_11(config.cc_matrix.at(2).at(2)); //Q2.11

  reg.color_correction.offset_ry.offset_in  = config.cc_offin.at(0);
  reg.color_correction.offset_ry.offset_out = config.cc_offout.at(0);
  reg.color_correction.offset_gu.offset_in  = config.cc_offin.at(1);
  reg.color_correction.offset_gu.offset_out = config.cc_offout.at(1);
  reg.color_correction.offset_bv.offset_in  = config.cc_offin.at(2);
  reg.color_correction.offset_bv.offset_out = config.cc_offout.at(2);

  reg.color_correction.clip_ry.clip_min = config.cc_clipmin.at(0);
  reg.color_correction.clip_ry.clip_max = config.cc_clipmax.at(0);
  reg.color_correction.clip_gu.clip_min = config.cc_clipmin.at(1);
  reg.color_correction.clip_gu.clip_max = config.cc_clipmax.at(1);
  reg.color_correction.clip_bv.clip_min = config.cc_clipmin.at(2);
  reg.color_correction.clip_bv.clip_max = config.cc_clipmax.at(2);

  // Send the registers
  avi_isp_color_correction_set_registers(&reg.color_correction);
}

/**
 * Set the gamma corrector configuration
 * This module can transform 10 bit pixels into 8 bit pixels using gamma correction. It
 * can either be configured to convert 8bit -> 8bit or 10bit -> 8bit. The default operation
 * should be 10bit -> 8bit. In palette mode Only the first channel (of RGB) is used and
 * looked up in the lookup tables to generate RGB output. In the normal mode the different
 * channels are looked up in the corresponding lookup table to generate the output values.
 * @param[in] enable Enable the gamma corrector
 * @param[in] palette Only use first component (of RGB it will be R)
 * @param[in] bit10 If set the input should be 10bit, if not it should be 8bit
 */
void ISP::setGammaCorrector(bool enable, bool palette, bool bit10) {
  config.gc_enable  = enable;
  config.gc_palette = palette;
  config.gc_10bit   = bit10;
  sendGammaCorrector();
}

/**
 * Send the gamma corrector configuration
 */
void ISP::sendGammaCorrector(void) {
  // Set the registers
  reg.gamma_corrector.conf.bypass       = config.gc_enable? 0:1;
  reg.gamma_corrector.conf.palette      = config.gc_palette? 1:0;
  reg.gamma_corrector.conf.comp_width   = config.gc_10bit? 1:0;

  // Send the registers
  avi_isp_gamma_corrector_set_registers(&reg.gamma_corrector);
}

/**
 * Set the gamma corrector configuration
 * This module can transform 10 bit pixels into 8 bit pixels using gamma correction. It
 * can either be configured to convert 8bit -> 8bit or 10bit -> 8bit. The default operation
 * should be 10bit -> 8bit. In palette mode Only the first channel (of RGB) is used and
 * looked up in the lookup tables to generate RGB output. In the normal mode the different
 * channels are looked up in the corresponding lookup table to generate the output values.
 * @param[in] enable Enable the gamma corrector
 * @param[in] palette Only use first component (of RGB it will be R)
 * @param[in] bit10 If set the input should be 10bit, if not it should be 8bit
 * @param[in] r_lut A lookup table for the Red channel (256 items in 8bit and 1024 items in 10bit)
 * @param[in] g_lut A lookup table for the Green channel (256 items in 8bit and 1024 items in 10bit)
 * @param[in] b_lut A lookup table for the Blue channel (256 items in 8bit and 1024 items in 10bit)
 */
void ISP::setGammaCorrector(bool enable, bool palette, bool bit10, std::vector<uint16_t> r_lut, std::vector<uint16_t> g_lut, std::vector<uint16_t> b_lut) {
  // Set the gamma corrector values
  setGammaCorrector(enable, palette, bit10);

  // Set the look up tables of the colors
  config.gc_rlut = r_lut;
  config.gc_glut = g_lut;
  config.gc_blut = b_lut;
  sendGammaCorrectorLUT();
}

/**
 * Send the gamma corrector lookup tables configuration
 */
void ISP::sendGammaCorrectorLUT(void) {
  // Set the registers
  uint16_t comp_max = config.gc_10bit? 1024:256;

  for(uint16_t i = 0; i < comp_max; ++i) {
    reg.ry_lut.ry_lut[i].ry_value = config.gc_rlut.at(i);
    reg.gu_lut.gu_lut[i].gu_value = config.gc_glut.at(i);
    reg.bv_lut.bv_lut[i].bv_value = config.gc_blut.at(i);
  }

  // Send the registers
  avi_isp_gamma_corrector_ry_lut_set_registers(&reg.ry_lut);
  avi_isp_gamma_corrector_gu_lut_set_registers(&reg.gu_lut);
  avi_isp_gamma_corrector_bv_lut_set_registers(&reg.bv_lut);
}

/**
 * Set color space conversion from RGB to YUV using a 3x3 matrix
 * This performs an matrix multiplication on the 8bit RGB input to generate
 * an 8bit YUV output.
 * - First it adds the offin to each of the 3 color channels (RGB).
 * - Then it performs a per pixel vector * matrix multiplication (3x3 matrix)
 * - Then it adds the offout to each of the 3 color channels (YUV)
 * - As last it performs a clipping on each pixel using clipmin and clipmax (which are set per color)
 * @param[in] matrix A float 3x3 matrix which each RGB pixel is multiplied with
 * @param[in] offin Offsets added to the input (for each color RGB)
 * @param[in] offout Offsets added to the output (for each channel YUV)
 * @param[in] clipmin Clipping minimum which is done at the output (for each channel YUV)
 * @param[in] clipmax Clipping maximum which is done at the output (for each channel YUV)
 */
void ISP::setColorSpaceConversion(std::vector<std::vector<float>> matrix, std::vector<uint32_t> offin, std::vector<uint32_t> offout, std::vector<uint32_t> clipmin, std::vector<uint32_t> clipmax) {
  config.csc_matrix  = matrix;
  config.csc_offin   = offin;
  config.csc_offout  = offout;
  config.csc_clipmin = clipmin;
  config.csc_clipmax = clipmax;
  sendColorSpaceConversion();
}

/**
 * Send the color space conversion
 */
void ISP::sendColorSpaceConversion(void) {
  assert(config.csc_offin.size() == 3);
  assert(config.csc_offout.size() == 3);
  assert(config.csc_clipmin.size() == 3);
  assert(config.csc_clipmax.size() == 3);
  assert(config.csc_matrix.size() == 3);
  assert(config.csc_matrix.at(0).size() == 3);
  assert(config.csc_matrix.at(1).size() == 3);
  assert(config.csc_matrix.at(2).size() == 3);

  // Set the registers
  reg.chroma.coeff_01_00.coeff_00 = floatToQ2_11(config.csc_matrix.at(0).at(0)); //Q2.11
  reg.chroma.coeff_01_00.coeff_01 = floatToQ2_11(config.csc_matrix.at(0).at(1)); //Q2.11
  reg.chroma.coeff_10_02.coeff_02 = floatToQ2_11(config.csc_matrix.at(0).at(2)); //Q2.11
  reg.chroma.coeff_10_02.coeff_10 = floatToQ2_11(config.csc_matrix.at(1).at(0)); //Q2.11
  reg.chroma.coeff_12_11.coeff_11 = floatToQ2_11(config.csc_matrix.at(1).at(1)); //Q2.11
  reg.chroma.coeff_12_11.coeff_12 = floatToQ2_11(config.csc_matrix.at(1).at(2)); //Q2.11
  reg.chroma.coeff_21_20.coeff_20 = floatToQ2_11(config.csc_matrix.at(2).at(0)); //Q2.11
  reg.chroma.coeff_21_20.coeff_21 = floatToQ2_11(config.csc_matrix.at(2).at(1)); //Q2.11
  reg.chroma.coeff_22.coeff_22    = floatToQ2_11(config.csc_matrix.at(2).at(2)); //Q2.11

  reg.chroma.offset_ry.offset_in  = config.csc_offin.at(0);
  reg.chroma.offset_ry.offset_out = config.csc_offout.at(0);
  reg.chroma.offset_gu.offset_in  = config.csc_offin.at(1);
  reg.chroma.offset_gu.offset_out = config.csc_offout.at(1);
  reg.chroma.offset_bv.offset_in  = config.csc_offin.at(2);
  reg.chroma.offset_bv.offset_out = config.csc_offout.at(2);

  reg.chroma.clip_ry.clip_min = config.csc_clipmin.at(0);
  reg.chroma.clip_ry.clip_max = config.csc_clipmax.at(0);
  reg.chroma.clip_gu.clip_min = config.csc_clipmin.at(1);
  reg.chroma.clip_gu.clip_max = config.csc_clipmax.at(1);
  reg.chroma.clip_bv.clip_min = config.csc_clipmin.at(2);
  reg.chroma.clip_bv.clip_max = config.csc_clipmax.at(2);

  // Send the registers
  avi_isp_chroma_set_registers(&reg.chroma);
}

/**
 * Enable or disable parts of the YUV ISP chain
 * @param[in] ee_crf Enable the Edge Enhancement and Color Reduction Filter
 * @param[in] i3d_lut Enable the 3D Look up tables
 * @param[in] drop Enable dropping of top left corner of the image
 */
void ISP::setYUVChain(bool ee_crf, bool i3d_lut, bool drop) {
  config.yuv_ee_crf   = ee_crf;
  config.yuv_i3d_lut  = i3d_lut;
  config.yuv_drop     = drop;
  sendYUVChain();
}

/**
 * Send the YUV chain configuration
 */
void ISP::sendYUVChain(void) {
  // Set the registers
  reg.yuv_inter.module_bypass.ee_crf_bypass   = config.yuv_ee_crf? 0:1;
  reg.yuv_inter.module_bypass.i3d_lut_bypass  = config.yuv_i3d_lut? 0:1;
  reg.yuv_inter.module_bypass.drop_bypass     = config.yuv_drop? 0:1;

  // Send the register
  avi_isp_chain_yuv_inter_set_registers(&reg.yuv_inter);
}

/**
 * Copy to registers
 * Note that this functions is really unsafe as it does no checking at all!
 * Do not run this on your own computer!
 * @param[in] addr The register start address
 * @param[in] reg_base The register values starting pointer
 * @param[in] s The size to write
 */
void ISP::memcpy_to_registers(unsigned long addr, const void *reg_base, size_t s) {
  const uint32_t *reg = (const uint32_t *)reg_base;
  unsigned i;

  s /= sizeof(uint32_t); /* we write one register at a time */

  for (i = 0; i < s; i++)
    *((volatile uint32_t *)(addr + i * sizeof(uint32_t))) = reg[i];
}

/**
 * Copy from registers
 * Note that this functions is really unsafe as it does no checking at all!
 * Do not run this on your own computer!
 * @param[in] reg_base The output pointer where the registers should be written to
 * @param[in] addr The register start address
 * @param[in] s The size to read
 */
void ISP::memcpy_from_registers(void *reg_base, unsigned long addr, size_t s) {
  uint32_t *reg = (uint32_t *)reg_base;
  unsigned i;

  s /= sizeof(uint32_t); /* we read one register at a time */

  for (i = 0; i < s; i++)
    reg[i] = *((volatile uint32_t *)(addr + i * sizeof(uint32_t)));
}

/**
 * Convert a float into a signed Q2.11 value
 * The signed value is in Two's complement
 * @param[in] var The float input value
 * @return The signed Q2.11 representation
 */
uint16_t ISP::floatToQ2_11(float var) {
  if(var < 0)
    return (~((uint16_t)((-1 * var) * (1<<11) + 0.5) & 0x1FFF) + 0x1) & 0x3FFF;
  else
    return ((uint16_t)(var * (1<<11) + 0.5) & 0x1FFF);
}

/**
 * Convert a signed Q2.11 into a float value
 * The signed value is in Two's complement
 * @param[in] var The signed Q2.11 input value
 * @return The float representation
 */
float ISP::Q2_11ToFloat(uint16_t var) {
  if(var & 0x2000)
    return -((float)((~(var - 0x1))  & 0x1FFF) / (1<<11));
  else
    return (float)(var) / (1<<11);
}

#define EXPAND_AS_FUNCTION(_node)                                                                \
  void ISP::avi_isp_ ## _node ## _set_registers(struct avi_isp_ ## _node ## _regs const *regs) { \
    memcpy_to_registers(offsets[_node], regs, sizeof(*regs));                                    \
  }                                                                                              \
                                                                                                 \
  void ISP::avi_isp_ ## _node ## _get_registers(struct avi_isp_ ## _node ## _regs *regs) {       \
    memcpy_from_registers(regs, offsets[_node], sizeof(*regs));                                  \
  }

AVI_DEFINE_NODE(EXPAND_AS_FUNCTION)
