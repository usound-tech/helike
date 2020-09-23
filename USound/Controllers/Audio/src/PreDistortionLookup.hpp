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
//  Description: Pre-distortion lookup filter. It performs an audio gain conversion function.
//  Filename: PredistortionLookup.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================


#pragma once

#include "arm_math.h"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "BiquadFilters.hpp"

/**
 * This class implements a mono-channel, pre-distortion lookup-table filter
 * with upsampling/downsampling to 192kHz
 */
class PreDistortionLookup
{
private:
  System::PredistortionConfiguration *config;
  BiquadFilters resamplingFiltersLeft;
  BiquadFilters resamplingFiltersRight;


  uint32_t blockSize;

  float32_t *resamplingBuffer;
  float32_t *scalingBuffer;
  float32_t *multiplicationBuffer;
  float32_t *additionBuffer;
  int lookupIdx = 0;

  void filter(float32_t *val);
  bool findRange(float32_t val);
  void runChannel(float32_t *pSrc, BiquadFilters &resamplingFilters);

public:
  PreDistortionLookup(System::PredistortionConfiguration *config, uint32_t blockSize);

  void init();
  void run(float32_t *pSrcLeft, float32_t *pSrcRight);
};
