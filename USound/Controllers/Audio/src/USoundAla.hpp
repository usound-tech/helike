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
//  Filename: USoundAla.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io), Jonathan Arweck (jonathan.arweck@usound.com)
//  Date: 6-May-2020
//
//====================================================================


#pragma once

#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "BiquadFilters.hpp"

#define CHANNEL_BUFFER_SIZE 432

/**
 * This class implements USound's ALA (Active Linearization Algorithm).
 */
class USoundAla
{
private:
  System::AlaConfiguration *config;
  uint32_t blockSize;
  uint8_t* channelBuffers[MAX_CHANNELS];
  uint8_t* globalBuffer;

public:
  USoundAla(System::AlaConfiguration *config, uint32_t blockSize);

  void init();
  void run(float *pSrcLeft, float *pSrcRight);
};
