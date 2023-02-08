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
//  Filename: BiquadFilters.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "BiquadFilters.hpp"

/**
 * Initialises a 2-channel chain of biquad filters
 * @param coeffLeft
 * @param coeffRight
 * @param number of samples to process as a block
 */
BiquadFilters::BiquadFilters(float32_t *coeffLeft, float32_t *coeffRight, uint32_t blockSize) :
    blockSize(blockSize)
{
  coefficients[PcmChannel::LEFT] = coeffLeft;
  coefficients[PcmChannel::RIGHT] = coeffRight;
}

/**
 * Initialises the internal state of the filter chains
 */
void BiquadFilters::init()
{
  for (int num = 0; num < PcmChannel::MAX_CHANNELS; num++)
  {
    if (!coefficients[num])
    {
      continue;
    }

    uint32_t stages = NUMSTAGES;
    for (uint32_t i = 0; i < NUMSTAGES; i++)
    {
      if (coefficients[num][i * 5] == 1.0f
          && coefficients[num][i * 5 + 1] == 0.0f
          && coefficients[num][i * 5 + 2] == 0.0f
          && coefficients[num][i * 5 + 3] == 0.0f)
      {
        stages = i;
        break;
      }
    }

    arm_biquad_cascade_df1_init_f32(&filter[num], stages, coefficients[num], filter_state[num]);
  }
}

/**
 * Receives a stereo stream and applies the biquad filters to each channel
 * @param pSrc
 * @param pDst
 */
void BiquadFilters::run(PcmChannel channel, float32_t *pSrc, float32_t *pDst)
{
  if (coefficients[channel] && filter[channel].numStages > 0)
  {
    arm_biquad_cascade_df1_f32(&filter[channel], pSrc, pDst, blockSize);
  }
  else if (pSrc != pDst)
  {
	memcpy(pDst, pSrc, blockSize * sizeof(float));
  }
}
