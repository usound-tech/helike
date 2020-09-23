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
//  Filename: PredistortionLookup.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "PreDistortionLookup.hpp"
#include "cmath"

#define UPSAMPLING_FACTOR 4
#define UPSAMPLING_IDX 0
#define DOWNSAMPLING_IDX 1

/**
 * Default constructor of the pre-distortion lookup-table filter
 */
PreDistortionLookup::PreDistortionLookup(System::PredistortionConfiguration *config, uint32_t blockSize) :
    config(config),
    resamplingFiltersLeft(
        config->coeffs[System::PredistortionCoeffcientType::UPSAMPLING],
        config->coeffs[System::PredistortionCoeffcientType::DOWNSAMPLING],
        blockSize * UPSAMPLING_FACTOR
        ),
    resamplingFiltersRight(
        config->coeffs[System::PredistortionCoeffcientType::UPSAMPLING],
        config->coeffs[System::PredistortionCoeffcientType::DOWNSAMPLING],
        blockSize * UPSAMPLING_FACTOR
        ),
    blockSize(blockSize)
{
  if (!config->rangeSize)
  {
    return;
  }

  resamplingBuffer = new float32_t[blockSize * UPSAMPLING_FACTOR];

  multiplicationBuffer = new float32_t[blockSize];
  additionBuffer = new float32_t[blockSize];

  scalingBuffer = new float32_t[config->rangeSize - 1];
}

/**
 * Updates the internal state according to the pre-distortion configuration
 */
void PreDistortionLookup::init()
{
  if (!config->rangeSize || !config->enabled)
  {
    return;
  }

  resamplingFiltersLeft.init();
  resamplingFiltersRight.init();

  for (uint32_t i = 0; i < (config->rangeSize - 1); i++)
  {
    scalingBuffer[i] = (config->outputRange[i + 1] - config->outputRange[i]) / (config->inputRange[i + 1] - config->inputRange[i]);
  }
}

void PreDistortionLookup::run(float32_t *pSrcLeft, float32_t *pSrcRight)
{
  if (!config->rangeSize || !config->enabled)
  {
    return;
  }

  runChannel(pSrcLeft, resamplingFiltersLeft);
  runChannel(pSrcRight, resamplingFiltersRight);
}

/**
 * This function runs the pre-distortion algorithm in-place, on the provided audio samples
 */
void PreDistortionLookup::runChannel(float32_t *pSrc, BiquadFilters &resamplingFilters)
{
  uint32_t i, j;
  lookupIdx = 0;

  // Upsampling
  for (i = 0, j = 0; i < blockSize * UPSAMPLING_FACTOR; i += UPSAMPLING_FACTOR, j++)
  {
    resamplingBuffer[i] = pSrc[j];
    resamplingBuffer[i + 1] = 0.0f;
    resamplingBuffer[i + 2] = 0.0f;
    resamplingBuffer[i + 3] = 0.0f;
  }

  resamplingFilters.run((PcmChannel) UPSAMPLING_IDX, resamplingBuffer, resamplingBuffer);

  // Pre-distortion filter
  filter(resamplingBuffer);

  // Downsampling
  resamplingFilters.run((PcmChannel) DOWNSAMPLING_IDX, resamplingBuffer, resamplingBuffer);

  for (i = 0, j = 0; i < blockSize * UPSAMPLING_FACTOR; i += UPSAMPLING_FACTOR, j++)
  {
    pSrc[j] = resamplingBuffer[i];
  }
}

/**
 * It finds which of the input ranges the sample falls in.
 * After finding the range, lookupIdx has the index of the input and output ranges to be used for the look up interpolation.
 *
 * @return true if the value is withing a range, false otherwise
 */
bool PreDistortionLookup::findRange(float32_t val)
{
  while (val < config->inputRange[lookupIdx])
  {
    lookupIdx--;
    if (lookupIdx < 0)
    {
      lookupIdx = 0;
      return false;
    }
  }

  while (val >= config->inputRange[lookupIdx + 1])
  {
    lookupIdx++;
    if (lookupIdx >= (int) (config->rangeSize - 1))
    {
      lookupIdx = (int) (config->rangeSize - 2);
      return false;
    }
  }

  return true;
}

/**
 * Performs the lookup operation of the pre-distortion filter
 * @param val the input audio value
 * @return the scaled value
 */
void PreDistortionLookup::filter(float32_t *inputValues)
{
  float32_t *values = inputValues;

  for (uint32_t turns = 0; turns < UPSAMPLING_FACTOR; turns++)
  {
    // Look up stage
    for (uint32_t i = 0; i < blockSize; i++)
    {
      float32_t inputVal = values[i];

      if (!findRange(inputVal))
      {
        multiplicationBuffer[i] = 1.0f;
        additionBuffer[i] = 0.0f;
      }
      else
      {
        multiplicationBuffer[i] = scalingBuffer[lookupIdx];
        additionBuffer[i] = config->outputRange[lookupIdx];
        values[i] -= config->inputRange[lookupIdx];
      }
    }

    arm_mult_f32(values, multiplicationBuffer, values, blockSize);
    arm_add_f32(values, additionBuffer, values, blockSize);

    values = &values[blockSize];
  }
}
