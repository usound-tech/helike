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
//  Description: Dynamic range compressor
//  Filename: Drc.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================


#pragma once

#include "arm_math.h"
#include "Controllers/System/pub/SystemConfiguration.hpp"

/**
 * This class is a wrapper of the Dynamic range compressor operation
 */
class Drc
{
private:
  System::DrcConfiguration *drcConfig;
  uint32_t blockSize;
  float32_t *audioLevelDb = nullptr;
  float32_t *gain = nullptr;

  float32_t previousLevelLowPassPower = 1.0;
  float32_t previousGainIndB = 0.0; //last gain^2 used
  float32_t attackConst = 0.0;
  float32_t releaseConst = 0.0;
  float32_t levelLowPassConst = 0.0;
  float32_t compressionRatioConst = 0.0;

private:
  void calcAudioLevelInDb(const float32_t *pSrcLeft, const float32_t *pSrcRight, float32_t *audioLevelDbBlock);
  void calcGain(float32_t *audioLevelDbBlock, float32_t *gainBlock);
  void calcInstantaneousTargetGain(float32_t *audioLevelDbBlock, float32_t *gainBlock);
  void calcSmoothedGainInDb(float32_t *inst_targ_gain_dB_block);


public:
  Drc(System::DrcConfiguration *drcConfig, uint32_t blockSize);

  void init();
  void run(float32_t *pSrcLeft, float32_t *pSrcRight);
};
