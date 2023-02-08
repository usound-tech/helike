//====================================================================
//
// COPYRIGHT 2018 All rights reserved.
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
//  Description: 7-stage biquad filters
//  Filename: AudioFilters.hpp
//  Author(s): Kumar Bala (kb@socfpga.io)
//  Date: 25-September-2018
//
//====================================================================


#pragma once

#include "arm_math.h"
#include "Controllers/System/pub/ModuleConfig.hpp"
#include "Drc.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "BiquadFilters.hpp"
#include "USoundAla.hpp"


/**
 * This class is a wrapper of the biquad filter operation
 */
class AudioFilters
{
private:
  uint32_t blockSize;
  BiquadFilters masterEqFilters;
  BiquadFilters xoverTweeterFilters;
  BiquadFilters xoverWooferFilters;

#if ALA_MODULE_ENABLED == 1
  USoundAla ala;
#endif

  Drc levelerDrc;
  Drc limiterDrc;
  float32_t *channelSamples[4] = { nullptr, nullptr, nullptr, nullptr };
  System::FilterConfiguration *filterConfig;

  void deinterlace16Tof32(const int16_t *pSrc, float32_t *pDst, uint32_t stride);
  void interlacef32To16(const float32_t *pSrc, int16_t *pDst, uint32_t stride);

public:
  AudioFilters(System::FilterConfiguration *filterConfig, uint32_t blockSize);

  void init();
  void run(int16_t *pSrc, int16_t *pDst[2]);
};

