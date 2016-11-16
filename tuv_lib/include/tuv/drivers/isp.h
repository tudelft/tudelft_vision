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

#ifndef DRIVERS_ISP_H_
#define DRIVERS_ISP_H_

#include <tuv/drivers/isp/reg_avi.h>

#include <cstring>
#include <vector>

/**
 * @brief ISP driver for Parrot
 *
 * This is a driver implementation for the Parrot ISP which is used on the Bebop 1 and Bebop 2.
 */
class ISP {
  private:
    /** List of all the isp registers */
    enum {
        AVI_DEFINE_NODE(EXPAND_AS_ENUM)
        ISP_NODE_NR,
    };

    int           devmem;               ///< Device memory pointer
    unsigned long avi_base;             ///< Base address
    unsigned      offsets[ISP_NODE_NR]; ///< Register offsets from base

    /** ISP registers and the values */
    struct avi_isp_registers {
        struct avi_isp_vlformat_32to40_regs vlformat_32to40;    ///< 32 bit to 40 bit converter registers
        struct avi_isp_chain_bayer_inter_regs bayer_inter;      ///< Bayer chain registers
        struct avi_isp_pedestal_regs pedestal;                  ///< Pedestal registers
        struct avi_isp_denoising_regs denoising;                ///< Denoising registers
        struct avi_isp_bayer_regs bayer;                        ///< Debayering registers
        struct avi_isp_color_correction_regs color_correction;  ///< Color correction registers
        struct avi_isp_vlformat_40to32_regs vlformat_40to32;    ///< 40 bit to 32 bit converver registers
        struct avi_isp_gamma_corrector_regs gamma_corrector;    ///< Gamma corrector registers
        struct avi_isp_gamma_corrector_ry_lut_regs ry_lut;      ///< Gamma corrector RY list registers
        struct avi_isp_gamma_corrector_gu_lut_regs gu_lut;      ///< Gamma corrector GU list registers
        struct avi_isp_gamma_corrector_bv_lut_regs bv_lut;      ///< Gamma corrector BF list registers
        struct avi_isp_chroma_regs chroma;                      ///< Chroma registers
        struct avi_isp_chain_yuv_inter_regs yuv_inter;          ///< YUV chain registers
    };
    struct avi_isp_registers reg;   ///< ISP register values

    /** ISP internal configuration */
    struct avi_isp_config {
        uint8_t cfa;            ///< CFA configuration (Bayer pixel order)

        // Bayer chain
        bool bayer_ped;         ///< Bayer Pedestal enabling
        bool bayer_grim;        ///< Bayer Green Imbalance enabling
        bool bayer_rip;         ///< Bayer RIP enabling
        bool bayer_denoise;     ///< Bayer Denosing enabling
        bool bayer_lsc;         ///< Bayer Lens Shade Corrector enabling
        bool bayer_ca;          ///< Bayer Chromatic Aberration enabling
        bool bayer_demos;       ///< Bayer Demosaicking enabling
        bool bayer_colm;        ///< Bayer Color Matrix enabling

        // pedestal
        uint16_t pedestal_r;    ///< Pedestal value for the Red channel
        uint16_t pedestal_gb;   ///< Pedestal value for the Green-Blue channel
        uint16_t pedestal_gr;   ///< Pedestal value for the Green-Red channel
        uint16_t pedestal_b;    ///< Pedestal value for the Blue channel

        // Denoising
        std::vector<uint8_t> denoise_red;   ///< Denoising Red channel vector
        std::vector<uint8_t> denoise_green; ///< Denoising Green channel vector
        std::vector<uint8_t> denoise_blue;  ///< Denoising Blue channel vector

        // Demosaicking
        uint16_t demos_threshold_low;   ///< Demosaicking lower threshold
        uint16_t demos_threshold_high;  ///< Demosaicking upper threshold

        // Color correction
        std::vector<std::vector<float>> cc_matrix;  ///< Color correction matrix
        std::vector<uint32_t> cc_offin;     ///< Color correction input offsets
        std::vector<uint32_t> cc_offout;    ///< Color correction output offsets
        std::vector<uint32_t> cc_clipmin;   ///< Color correction clipping minimums
        std::vector<uint32_t> cc_clipmax;   ///< Color correction clipping maximums

        // Gamma corrector
        bool gc_enable;     ///< Gamma correction enabling
        bool gc_palette;    ///< Gamma correction color palette
        bool gc_10bit;      ///< Gamma correction operating at 10bit channels
        std::vector<uint16_t> gc_rlut;  ///< Gamma correction red illuminance vector
        std::vector<uint16_t> gc_glut;  ///< Gamma correction green illuminance vector
        std::vector<uint16_t> gc_blut;  ///< Gamma correction blue illuminance vector

        // Color space conversion
        std::vector<std::vector<float>> csc_matrix;     ///< Color space conversion matrix
        std::vector<uint32_t> csc_offin;        ///< Color space conversion input offsets
        std::vector<uint32_t> csc_offout;       ///< Color space conversion output offsets
        std::vector<uint32_t> csc_clipmin;      ///< Color space conversion clipping minimums
        std::vector<uint32_t> csc_clipmax;      ///< Color space conversion clipping maximums

        // YUV Chain
        bool yuv_ee_crf;    ///< YUV chain Edge Enhancement enabling
        bool yuv_i3d_lut;   ///< YUV chain i3d enabling
        bool yuv_drop;      ///< YUV Drop enabling
    };
    struct avi_isp_config config;   ///< ISP configuration values

    /* Register access functions */
    void memcpy_to_registers(unsigned long addr, const void *reg_base, size_t s);
    void memcpy_from_registers(void *reg_base, unsigned long addr, size_t s);
    AVI_DEFINE_NODE(EXPAND_AS_PROTOTYPE); ///< Expand all ISP register functions

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

#endif /* DRIVERS_ISP_H_ */
