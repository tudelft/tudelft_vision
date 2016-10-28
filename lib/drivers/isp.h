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

#include "isp/reg_avi.h"

#include "debug.h"
#include <cstring>
#include <vector>

class ISP: protected Debug {
  private:
    /* List all isp registers */
    enum {
      AVI_DEFINE_NODE(EXPAND_AS_ENUM)
      ISP_NODE_NR,
    };

    int           devmem;
    unsigned long avi_base;
    unsigned      offsets[ISP_NODE_NR];

    /* ISP registers and the values */
    struct avi_isp_registers
    {
      struct avi_isp_vlformat_32to40_regs vlformat_32to40;
      struct avi_isp_chain_bayer_inter_regs bayer_inter;
      struct avi_isp_pedestal_regs pedestal;
      struct avi_isp_denoising_regs denoising;
      struct avi_isp_bayer_regs bayer;
      struct avi_isp_color_correction_regs color_correction;
      struct avi_isp_vlformat_40to32_regs vlformat_40to32;
      struct avi_isp_gamma_corrector_regs gamma_corrector;
      struct avi_isp_gamma_corrector_ry_lut_regs ry_lut;
      struct avi_isp_gamma_corrector_gu_lut_regs gu_lut;
      struct avi_isp_gamma_corrector_bv_lut_regs bv_lut;
      struct avi_isp_chroma_regs chroma;
      struct avi_isp_chain_yuv_inter_regs yuv_inter;
    };
    struct avi_isp_registers reg;

    /* ISP internal configuration */
    struct avi_isp_config
    {
      uint8_t cfa;

      // Bayer chain
      bool bayer_ped;
      bool bayer_grim;
      bool bayer_rip;
      bool bayer_denoise;
      bool bayer_lsc;
      bool bayer_ca;
      bool bayer_demos;
      bool bayer_colm;

      // pedestal
      uint16_t pedestal_r;
      uint16_t pedestal_gb;
      uint16_t pedestal_gr;
      uint16_t pedestal_b;

      // Denoising
      std::vector<uint8_t> denoise_red;
      std::vector<uint8_t> denoise_green;
      std::vector<uint8_t> denoise_blue;

      // Demosaicking
      uint16_t demos_threshold_low;
      uint16_t demos_threshold_high;

      // Color correction
      std::vector<std::vector<float>> cc_matrix;
      std::vector<uint32_t> cc_offin;
      std::vector<uint32_t> cc_offout;
      std::vector<uint32_t> cc_clipmin;
      std::vector<uint32_t> cc_clipmax;

      // Gamma corrector
      bool gc_enable;
      bool gc_palette;
      bool gc_10bit;
      std::vector<uint16_t> gc_rlut;
      std::vector<uint16_t> gc_glut;
      std::vector<uint16_t> gc_blut;

      // Color space conversion
      std::vector<std::vector<float>> csc_matrix;
      std::vector<uint32_t> csc_offin;
      std::vector<uint32_t> csc_offout;
      std::vector<uint32_t> csc_clipmin;
      std::vector<uint32_t> csc_clipmax;

      // YUV Chain
      bool yuv_ee_crf;
      bool yuv_i3d_lut;
      bool yuv_drop;
    };
    struct avi_isp_config config;

    /* Register access functions */
    void memcpy_to_registers(unsigned long addr, const void *reg_base, size_t s);
    void memcpy_from_registers(void *reg_base, unsigned long addr, size_t s);
    AVI_DEFINE_NODE(EXPAND_AS_PROTOTYPE);

    /* Internal conversion functions */
    uint16_t floatToQ2_11(float var);
    float Q2_11ToFloat(uint16_t var);

    /* Internal setting functions */
    void sendBayerChain(void);
    void sendPedestal(void);
    void sendDenoising(void);
    void sendDemosaicking(void);
    void sendColorCorrection(void);
    void sendGammaCorrector(void);
    void sendGammaCorrectorLUT(void);
    void sendColorSpaceConversion(void);
    void sendYUVChain(void);

  public:
    ISP();

    void configure(int fd); ///< Configure the ISP
    void reset(void);       ///< Reset the ISP

    /* Configuration functions */
    void setResolution(uint32_t width, uint32_t height);  ///< Set the ISP resolution
    void setBayerChain(bool ped, bool grim, bool rip, bool denoise, bool lsc, bool ca, bool demos, bool conv);
    void setPedestal(uint16_t val_r, uint16_t val_gb, uint16_t val_gr, uint16_t val_b);
    void setPedestal(uint16_t val);
    void setDenoising(std::vector<uint8_t> red, std::vector<uint8_t> green, std::vector<uint8_t> blue);
    void setDemosaicking(uint16_t low, uint16_t high);
    void setColorCorrection(std::vector<std::vector<float>> matrix, std::vector<uint32_t> offin, std::vector<uint32_t> offout, std::vector<uint32_t> clipmin, std::vector<uint32_t> clipmax);
    void setGammaCorrector(bool enable, bool palette, bool bit10);
    void setGammaCorrector(bool enable, bool palette, bool bit10, std::vector<uint16_t> r_lut, std::vector<uint16_t> g_lut, std::vector<uint16_t> b_lut);
    void setColorSpaceConversion(std::vector<std::vector<float>> matrix, std::vector<uint32_t> offin, std::vector<uint32_t> offout, std::vector<uint32_t> clipmin, std::vector<uint32_t> clipmax);
    void setYUVChain(bool ee_crf, bool i3d_lut, bool drop);
};
