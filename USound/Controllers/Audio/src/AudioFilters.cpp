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
//  Filename: filters.c
//  Author(s): Kumar Bala (kb@socfpga.io)
//  Date: 25-September-2018
//
//====================================================================

#include <stdlib.h>
#include <string.h>
#include "AudioFilters.hpp"
#include "Drc.hpp"
#include "Controllers/System/pub/ModuleConfig.hpp"

#define STREAMS   4
#define XOVER_SAMPLES 2



/**
 * Initialises a 2-channel chain of filters
 * @param coeffLeft
 * @param coeffRight
 * @param number of samples to process as a block
 */
AudioFilters::AudioFilters(System::FilterConfiguration *filterConfig, uint32_t blockSize) :
    blockSize(blockSize),

    masterEqFilters(
        filterConfig->masterEqCoeffs[System::MasterEqCoeffcientType::LEFT],
        filterConfig->masterEqCoeffs[System::MasterEqCoeffcientType::RIGHT],
        blockSize
        ),

    xoverTweeterFilters(
        filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::LEFT_TWEETER],
        filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::RIGHT_TWEETER],
        blockSize
        ),

    xoverWooferFilters(
        filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::LEFT_WOOFER],
        filterConfig->xoverEqCoeffs[System::XoverEqCoeffcientType::RIGHT_WOOFER],
        blockSize
        ),

#if ALA_MODULE_ENABLED == 1
    ala(
        &filterConfig->alaConfig,
        blockSize
        ),
#endif

    levelerDrc(&filterConfig->levelerDrcConfig, blockSize),
    limiterDrc(&filterConfig->limiterDrcConfig, blockSize),
    filterConfig(filterConfig)
{
  channelSamples[0] = new float32_t[blockSize];
  channelSamples[1] = new float32_t[blockSize];
  channelSamples[2] = new float32_t[blockSize];
  channelSamples[3] = new float32_t[blockSize];
}

void AudioFilters::init()
{
  masterEqFilters.init();
  xoverTweeterFilters.init();
  xoverWooferFilters.init();

  levelerDrc.init();
  limiterDrc.init();

#if ALA_MODULE_ENABLED == 1
  ala.init();
#endif
}

/**
 * Converts an interleaved stream of uint16_t samples into a contiguous stream
 * @param pSrc
 * @param pDst
 * @param stride
 */
void AudioFilters::deinterlace16Tof32(const int16_t *pSrc, float32_t *pDst, uint32_t stride)
{
  for (uint32_t i = 0; i < blockSize; i++)
  {
    pDst[i] = (float32_t) *pSrc / (1 << 15);
    pSrc = &pSrc[stride];
  }
}

/**
 * Converts an contiguous audio stream (single channel) of float32_t samples into an interleaved stream
 * @param pSrc
 * @param pDst
 * @param stride
 */
void AudioFilters::interlacef32To16(const float32_t *pSrc, int16_t *pDst, uint32_t stride)
{
  for (uint32_t i = 0; i < blockSize; i++)
  {
    *pDst = (int16_t) (roundf(pSrc[i] * (1 << 15)));
    pDst = &pDst[stride];
  }
}

/**
 * Runs all the EQ and DRC filters
 * @param pSrc the input audio buffer with interleaved uint16_t samples
 * @param pDst the output audio buffer with interleaved uint16_t samples
 */
void AudioFilters::run(int16_t *pSrc, int16_t *pDst[2])
{
#if SWAP_AUDIO_CHANNELS == 1
	uint8_t leftChannelIndex = 1;
#else
	uint8_t leftChannelIndex = 0;
#endif

  deinterlace16Tof32(pSrc, channelSamples[PcmChannel::LEFT], 2);
  deinterlace16Tof32(&pSrc[1], channelSamples[PcmChannel::RIGHT], 2);

  // Master EQ filtering
  if (filterConfig->masterEqEnabled)
  {
    masterEqFilters.run(PcmChannel::LEFT, channelSamples[PcmChannel::LEFT], channelSamples[PcmChannel::LEFT]);
    masterEqFilters.run(PcmChannel::RIGHT, channelSamples[PcmChannel::RIGHT], channelSamples[PcmChannel::RIGHT]);
  }

  // DRC processing
  levelerDrc.run(channelSamples[PcmChannel::LEFT], channelSamples[PcmChannel::RIGHT]);
  limiterDrc.run(channelSamples[PcmChannel::LEFT], channelSamples[PcmChannel::RIGHT]);

  // X-over EQ
  if (filterConfig->xoverEqEnabled)
  {
    xoverWooferFilters.run(PcmChannel::LEFT, channelSamples[PcmChannel::LEFT], channelSamples[PcmChannel::LEFT + XOVER_SAMPLES]);
    xoverWooferFilters.run(PcmChannel::RIGHT, channelSamples[PcmChannel::RIGHT], channelSamples[PcmChannel::RIGHT + XOVER_SAMPLES]);

    xoverTweeterFilters.run(PcmChannel::LEFT, channelSamples[PcmChannel::LEFT], channelSamples[PcmChannel::LEFT]);
    xoverTweeterFilters.run(PcmChannel::RIGHT, channelSamples[PcmChannel::RIGHT], channelSamples[PcmChannel::RIGHT]);

    interlacef32To16(channelSamples[PcmChannel::LEFT + XOVER_SAMPLES], &pDst[STREAM_ID::STREAM_WOOFER][leftChannelIndex], 2);
    interlacef32To16(channelSamples[PcmChannel::RIGHT + XOVER_SAMPLES], &pDst[STREAM_ID::STREAM_WOOFER][!leftChannelIndex], 2);
  }
  else
  {
    interlacef32To16(channelSamples[PcmChannel::LEFT], &pDst[STREAM_ID::STREAM_WOOFER][leftChannelIndex], 2);
    interlacef32To16(channelSamples[PcmChannel::RIGHT], &pDst[STREAM_ID::STREAM_WOOFER][!leftChannelIndex], 2);
  }

#if ALA_MODULE_ENABLED == 1
  ala.run(
      channelSamples[PcmChannel::LEFT],
      channelSamples[PcmChannel::RIGHT]
      );
#endif

  interlacef32To16(channelSamples[PcmChannel::LEFT], &pDst[STREAM_ID::STREAM_TWEETER][leftChannelIndex], 2);
  interlacef32To16(channelSamples[PcmChannel::RIGHT], &pDst[STREAM_ID::STREAM_TWEETER][!leftChannelIndex], 2);
}


