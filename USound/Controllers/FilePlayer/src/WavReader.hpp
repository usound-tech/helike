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
//  Description: Wav file reader
//  Filename: WavReader.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once
#include <Controllers/FilePlayer/pub/AudioPlayer.hpp>
#include <string>
#include "Controllers/Filesystem/pub/Filesystem.hpp"

namespace Controller
{

/**
 * This class is responsible for parsing stereo wav files, 16bit wide smaples
 */
class WavReader: public MediaFileReader
{
protected:
  System::File *wavFile = nullptr;
  uint32_t samples = 0;
  uint32_t channels = 0;
  uint32_t samplesConsumed = 0;

public:
  WavReader();

  bool init(System::File *wavFile) override;
  bool isAKnownFile(std::string &filename) override;
  bool loadNextChunk(uint8_t *buffer, uint32_t sampleCount) override;
  uint32_t getFrequency() override;
  void getPlaybackInfo(uint32_t &trackTime, uint32_t &playbackTime) override;
};

}
