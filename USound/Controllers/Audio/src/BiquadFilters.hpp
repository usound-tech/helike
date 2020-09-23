//====================================================================
//
// COPYRIGHT 2020 All rights reserved.
//       USound
//
//====================================================================
//
//                          DISCLAIMER
// NO WARRANTIES
// USound expressly disclaims any warranty for the SOFTWARE
// PRODUCT. The SOFTWARE PRODUCT and any related documentation is
// provided "as is" without warranty of any kind, either expressed or
// implied, including, without limitation, the implied warranties or
// merchantability, fitness for a particular purpose, or noninfringe-
// ment. The entire risk arising out of the use or performance of the
// SOFTWARE PRODUCT remains with the user.
//
// NO LIABILITY FOR DAMAGES.
// Under no circumstances is USound liable for any damages
// whatsoever (including, without limitation, damages for loss of busi-
// ness profits, business interruption, loss of business information,
// or any other pecuniary loss) arising out of the use of or inability
// to use this product.
//
//====================================================================
//
//  Description: multi-stage biquad filters
//  Filename: BiquadFilters.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "arm_math.h"

#define NUMSTAGES 8

enum PcmChannel
{
  LEFT,
  RIGHT,
  MAX_CHANNELS
};

enum STREAM_ID
{
  STREAM_TWEETER,
  STREAM_WOOFER,
  MAX_STREAM_COUNT
};

class BiquadFilters
{
private:
  uint32_t blockSize;
  arm_biquad_casd_df1_inst_f32 filter[MAX_CHANNELS];
  float32_t filter_state[MAX_CHANNELS][4 * NUMSTAGES];
  float32_t *coefficients[MAX_CHANNELS];

public:
  BiquadFilters(float32_t *coeffLeft, float32_t *coeffRight, uint32_t blockSize);

  void init();
  void run(PcmChannel channel, float32_t *pSrc, float32_t *pDst);
};
