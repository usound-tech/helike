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
//  Description: This implements USound's ALA (Active Linearization Algorithm). More information: https://www.usound.com/whitepaper-active-linearization-algorithm-for-usound-mems-speakers/
//  Filename: USoundAla.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io), Jonathan Arweck (jonathan.arweck@usound.com)
//  Date: 6-May-2020
//
//====================================================================

#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "USoundAla.hpp"
#include "cmath"

extern "C" { 
  #include "Ala-Helike-Public.h" 
  }

/**
 * Default constructor.
 */
USoundAla::USoundAla(System::AlaConfiguration *config, uint32_t blockSize) :
    config(config), blockSize(blockSize)
{
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    channelBuffers[i] = new uint8_t[CHANNEL_BUFFER_SIZE];
  }

  globalBuffer = new uint8_t[8 * blockSize];
}

/**
 * Updates the internal state according to the configuration
 */
void USoundAla::init()
{
  if (!config->enabled)
  {
    return;
  }

  InitAlaCoefficients((uint8_t*)config->coefficients);
  for (int i = 0; i < MAX_CHANNELS; i++)
  {
    InitAlaChannelBuffer((uint8_t*)channelBuffers[i]);    
  }
  
}

void USoundAla::run(float *pSrcLeft, float *pSrcRight)
{
  if (!config->enabled)
  {
    return;
  }

  ApplyAla(pSrcLeft,  channelBuffers[LEFT],  globalBuffer, (uint8_t*)config->coefficients, blockSize);
  ApplyAla(pSrcRight, channelBuffers[RIGHT], globalBuffer, (uint8_t*)config->coefficients, blockSize);
}