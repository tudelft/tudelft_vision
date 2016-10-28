#ifndef _REG_AVI_H
#define _REG_AVI_H

#include <stdint.h>

/* Chain Bayer */
#include "regmap/avi_isp_vlformat_32to40.h"
#include "regmap/avi_isp_chain_bayer_inter.h"
#include "regmap/avi_isp_pedestal.h"
#include "regmap/avi_isp_green_imbalance.h"
#include "regmap/avi_isp_dead_pixel_correction.h"
#include "regmap/avi_isp_statistics_bayer.h"
#include "regmap/avi_isp_denoising.h"
#include "regmap/avi_isp_lens_shading_correction.h"
#include "regmap/avi_isp_chromatic_aberration.h"
#include "regmap/avi_isp_bayer.h"
#include "regmap/avi_isp_color_correction.h"
#include "regmap/avi_isp_vlformat_40to32.h"

/* Gamma + CSC */
#include "regmap/avi_isp_gamma_corrector.h"
#include "regmap/avi_isp_chroma.h"
#include "regmap/avi_isp_statistics_yuv.h"

/* Chain YUV */
#include "regmap/avi_isp_chain_yuv_inter.h"
#include "regmap/avi_isp_edge_enhancement_color_reduction_filter.h"
#include "regmap/avi_isp_i3d_lut.h"
#include "regmap/avi_isp_drop.h"

/* Sub modules of chain Bayer */
#define AVI_ISP_CHAIN_BAYER_INTER                       0x0000
#define AVI_ISP_VLFORMAT_32TO40                         0x1000
#define AVI_ISP_PEDESTAL                                0x2000
#define AVI_ISP_GREEN_IMBALANCE                         0x4000
#define AVI_ISP_DEAD_PIXEL_CORRECTION                   0x6000
#define AVI_ISP_DENOISING                               0x7000
#define AVI_ISP_STATISTICS_BAYER                        0x8000
#define AVI_ISP_LENS_SHADING_CORRECTION                 0xA000
#define AVI_ISP_CHROMATIC_ABERRATION                    0xC000
#define AVI_ISP_BAYER                                   0xD000
#define AVI_ISP_COLOR_CORRECTION                        0xE000
#define AVI_ISP_VLFORMAT_40TO32                         0xF000

/* In between sub modules */
#define AVI_ISP_CHROMA                                  0x0000
#define AVI_ISP_STATISTICS_YUV                          0x0000

/* Sub modules of chain YUV */
#define AVI_ISP_CHAIN_YUV_INTER                         0x0000
#define AVI_ISP_EDGE_ENHANCEMENT_COLOR_REDUCTION_FILTER 0x1000
#define AVI_ISP_I3D_LUT                                 0x2000
#define AVI_ISP_DROP                                    0x3000

/* AVI memory */
#define AVI_BASE 0x400000
#define AVI_SIZE 0x100000
#define AVI_MASK (AVI_SIZE - 1)

/* Bayer statistics size */
#define BAYERSTATS_STATX 64
#define BAYERSTATS_STATY 48

/* Expander for memory addresses */
#define AVI_DEFINE_NODE(EXPANDER) \
  EXPANDER(chain_bayer_inter)         \
  EXPANDER(vlformat_32to40)         \
  EXPANDER(pedestal)            \
  EXPANDER(green_imbalance)         \
  EXPANDER(green_imbalance_green_red_coeff_mem)     \
  EXPANDER(green_imbalance_green_blue_coeff_mem)      \
  EXPANDER(dead_pixel_correction)         \
  EXPANDER(dead_pixel_correction_list_mem)      \
  EXPANDER(denoising)           \
  EXPANDER(statistics_bayer)          \
  EXPANDER(lens_shading_correction)       \
  EXPANDER(lens_shading_correction_red_coeff_mem)     \
  EXPANDER(lens_shading_correction_green_coeff_mem)   \
  EXPANDER(lens_shading_correction_blue_coeff_mem)    \
  EXPANDER(chromatic_aberration)          \
  EXPANDER(bayer)             \
  EXPANDER(color_correction)          \
  EXPANDER(vlformat_40to32)         \
  EXPANDER(gamma_corrector)         \
  EXPANDER(gamma_corrector_ry_lut)        \
  EXPANDER(gamma_corrector_gu_lut)        \
  EXPANDER(gamma_corrector_bv_lut)        \
  EXPANDER(chroma)            \
  EXPANDER(statistics_yuv)          \
  EXPANDER(statistics_yuv_ae_histogram_y)       \
  EXPANDER(chain_yuv_inter)         \
  EXPANDER(edge_enhancement_color_reduction_filter)   \
  EXPANDER(edge_enhancement_color_reduction_filter_ee_lut)  \
  EXPANDER(i3d_lut)           \
  EXPANDER(i3d_lut_lut_outside)         \
  EXPANDER(i3d_lut_lut_inside)          \
  EXPANDER(drop)

/* Usefull expanders */
#define EXPAND_AS_ENUM(_node) \
  _node,

#define EXPAND_AS_PROTOTYPE(_node)                                        \
  void avi_isp_ ## _node ## _set_registers(struct avi_isp_ ## _node ## _regs const *regs);   \
  void avi_isp_ ## _node ## _get_registers(struct avi_isp_ ## _node ## _regs *regs);

/* AVI offsets */
struct avi_isp_offsets
{
  uint32_t chain_bayer;
  uint32_t gamma_corrector;
  uint32_t chroma;
  uint32_t statistics_yuv;
  uint32_t chain_yuv;
};

/* IOCTL implemented in AVI drivers */
#define AVI_ISP_IOGET_OFFSETS _IOR('F', 0x33, struct avi_isp_offsets)

#endif /* _REG_AVI_H */
