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
//  Description: Audio tone generator
//  Filename: ToneGen.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"

namespace Controller
{

enum PatternType
{
  PATTERN_SINE,
  PATTERN_RAMP,
  PATTERN_SEQUENCE
};

class ToneGen: public GlobalServiceConsumer, public System::AudioSource<uint16_t>
{
private:
  uint16_t *data;
  PatternType pattern;
  uint32_t sineFreq;
  uint16_t currentOffset;

public:
  ToneGen();

  uint16_t* getData(uint32_t length) override;
  void consumedData(uint32_t length) override;
  uint32_t getFrequency() override;
  void notifyDataAvailable() override;

  void init() override;
  void deinit() override;
  void doAction(System::Action action) override;

  bool skipNext() override;
  bool skipPrev() override;

  void setPatternType(PatternType patternType);
  void setSineFrequency(uint32_t freq);
};

}
