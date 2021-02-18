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
//  Description: Dynamic range compressor
//  Filename: Drc.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//  This implementation is based on OpenAudio_ArduinoLibrary/AudioEffectCompressor_F32.h
//  https://github.com/chipaudette/OpenAudio_ArduinoLibrary/blob/master/AudioEffectCompressor_F32.h
//
//====================================================================

#include "./Drc.hpp"
#include "cmath"
#include "Utilities/MathUtils.hpp"

/**
 * Default constructor of the Dynamic range compressor class
 */
Drc::Drc(System::DrcConfiguration *drcConfig, uint32_t blockSize) :
    drcConfig(drcConfig),
    blockSize(blockSize)
{
  audioLevelDb = new float32_t[blockSize];
  gain = new float32_t[blockSize];
}

/**
 * Updates the internal state according to the drc configuration
 */
void Drc::init()
{
  drcConfig->compressionRatio = std::max(0.001f, drcConfig->compressionRatio);

  compressionRatioConst = 1.0f - (1.0f / drcConfig->compressionRatio);
  attackConst = expf(-1.0f / (drcConfig->attackDuration * drcConfig->sampleRateHz));
  releaseConst = expf(-1.0f / (drcConfig->releaseDuration * drcConfig->sampleRateHz));

  float32_t lowPassDuration = std::min(drcConfig->attackDuration, drcConfig->releaseDuration) / 5.0;
  lowPassDuration = std::max(0.002f, lowPassDuration);
  levelLowPassConst = expf(-1.0f / (lowPassDuration * drcConfig->sampleRateHz));

  previousLevelLowPassPower = 1.0f;
  previousGainIndB = 0.0f;
}

/**
 * This function runs the DRC algorithm in-place, on the provided audio samples
 */
void Drc::run(float32_t *pSrcLeft, float32_t *pSrcRight)
{
  if (!drcConfig->enabled)
  {
    //TODO: We should copy the src to the dst, as we are making an assumption that src and dst are the same buffer
    return;
  }

  calcAudioLevelInDb(pSrcLeft, pSrcRight, audioLevelDb);
  calcGain(audioLevelDb, gain);

  //apply the desired gain...store the processed audio back into audio_block
  arm_mult_f32(pSrcLeft, gain, pSrcLeft, blockSize);
  arm_mult_f32(pSrcRight, gain, pSrcRight, blockSize);
}

/**
 * This function estimates the level of the audio (in dB).
 * It squares the signal and low-pass filters to get a time-averaged signal power.
 * It holds the results in audioLevelDb.
 *
 * @param audioSrc the audio samples to be compressed
 * @param audioLevelDbBlock the estimated audio levels
 */
void Drc::calcAudioLevelInDb(const float32_t *pSrcLeft, const float32_t *pSrcRight, float32_t *audioLevelDbBlock)
{
  // Find the sample
  for (uint32_t i = 0; i < blockSize; i++)
  {
    audioLevelDbBlock[i] = (abs(pSrcLeft[i]) > abs(pSrcRight[i])) ? pSrcLeft[i] : pSrcRight[i];
  }

  // calculate the instantaneous signal power (square the signal)
  arm_mult_f32(audioLevelDbBlock, audioLevelDbBlock, audioLevelDbBlock, blockSize);

  // low-pass filter and convert to dB
  float32_t c1 = levelLowPassConst;
  float32_t c2 = 1.0f - c1;

  for (uint32_t i = 0; i < blockSize; i++)
  {
    // first-order low-pass filter to get a running estimate of the average power
    audioLevelDbBlock[i] = c1 * previousLevelLowPassPower + c2 * audioLevelDbBlock[i];

    // save the state of the first-order low-pass filter
    previousLevelLowPassPower = audioLevelDbBlock[i];

    //now convert the signal power to dB (but not yet multiplied by 10.0)
    audioLevelDbBlock[i] = log10fApprox(audioLevelDbBlock[i]);
  }

  // Limit the amount that the state of the smoothing filter can go toward negative infinity
  // Never go less than -130 dBFS
  if (previousLevelLowPassPower < (1.0E-13))
  {
    previousLevelLowPassPower = 1.0E-13;
  }

  //scale the wav_pow_block by 10.0 to complete the conversion to dB
  arm_scale_f32(audioLevelDbBlock, 10.0f, audioLevelDbBlock, blockSize);
  arm_offset_f32(audioLevelDbBlock, 3, audioLevelDbBlock, blockSize);
}

/**
 * This function computes the desired gain from the compressor, given an estimate of the signal level (in dB).
 *
 * @param audioLevelDbBlock holds the audio level estimates
 * @param gainBlock receives the gain values
 */
void Drc::calcGain(float32_t *audioLevelDbBlock, float32_t *gainBlock)
{
  // Calculate the instantaneous target gain based on the compression ratio
  calcInstantaneousTargetGain(audioLevelDbBlock, gainBlock);

  // Smooth in time (attack and release) by stepping through each sample
  calcSmoothedGainInDb(gainBlock);

  // Apply make-up gain
  if (drcConfig->postGain != 0.0f)
  {
    arm_offset_f32(gainBlock, drcConfig->postGain, gainBlock, blockSize);
  }

  // Convert from dB to linear gain: gain = 10^(gain_dB/20);  (ie this takes care of the sqrt, too!)
  arm_scale_f32(gainBlock, 1.0f / 20.0f, gainBlock, blockSize);

  for (uint32_t i = 0; i < blockSize; i++)
  {
    gainBlock[i] = pow10f(gainBlock[i]);
  }
}

/**
 * This function computes the instantaneous desired gain, including the compression ratio and
 * threshold for where the compression kicks in.
 *
 * @param audioLevelDb the audio level estimates (it gets modified in the function)
 * @param gainBlock receives the instantaneous gain results
 */
void Drc::calcInstantaneousTargetGain(float32_t *audioLevelDbBlock, float32_t *gainBlock)
{
  // How much are we above the compression threshold?
  arm_offset_f32(audioLevelDbBlock, -drcConfig->compressionThresholdFullScaleDb, audioLevelDbBlock, blockSize);

  // Scale by the compression ratio...this is what the output level should be (this is our target level)
  arm_scale_f32(audioLevelDbBlock, 1.0f / drcConfig->compressionRatio, gainBlock, blockSize);

  // Compute the instantaneous gain...which is the difference between the target level and the original level
  arm_sub_f32(gainBlock, audioLevelDbBlock, gainBlock, blockSize);

  // limit the target gain to attenuation only (this part of the compressor should not make things louder!)
  for (uint32_t i = 0; i < blockSize; i++)
  {
    if (gainBlock[i] > 0.0f)
    {
      gainBlock[i] = 0.0f;
    }
  }
}

/**
 * This function applies the "attack" and "release" constants to smooth the target gain level through time.
 * It performs the calculations in place.
 *
 * @param gain the gain to be smoothed out with attack and release parameters
 */
void Drc::calcSmoothedGainInDb(float32_t *gain)
{
  float32_t one_minus_attack_const = 1.0f - attackConst;
  float32_t one_minus_release_const = 1.0f - releaseConst;

  for (uint32_t i = 0; i < blockSize; i++)
  {
    float32_t gaindB = gain[i];

    // Smooth the gain using the attack or release constants
    if (gaindB < previousGainIndB)
    {
      // Are we in the attack phase?
      gain[i] = attackConst * previousGainIndB + one_minus_attack_const * gaindB;
    }
    else
    {
      // We're in the release phase
      gain[i] = releaseConst * previousGainIndB + one_minus_release_const * gaindB;
    }

    // Save value for the next time through this loop
    previousGainIndB = gain[i];
  }
}


