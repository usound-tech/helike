//====================================================================
//
// COPYRIGHT 2019 All rights reserved.
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
//  Description: Audio Out interface - SAI sink
//  Filename: audio_out_sai.c
//  Author(s): Kumar Bala (kb@socfpga.io)
//  Date: 20-November-2019
//
//====================================================================

#pragma once

#include "Utilities/Fifo/pub/Fifo.hpp"
#include "Interfaces/pub/SystemControl.hpp"
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/Audio/pub/AudioService.hpp"

namespace PeripheralInterface
{



/**
 * This class manages the SAI In peripheral
 */
class AudioLocalIn: public GlobalServiceConsumer, public System::AudioSource<uint16_t>
{
private:
  uint32_t frequency;
  uint32_t chunkSizePerTransfer = 0;
  uint32_t audioBufferLength = 0;
  uint16_t *rxSamples = nullptr;
  uint16_t *silenceSamples = nullptr;
  uint32_t wr = 0;
  uint32_t rd = 0;
  System::SystemBus systemBus;

private:
  void enable();
  void start();
  void stop();
  uint32_t getAvailableSampleCount();

public:
  AudioLocalIn(System::SystemBus systemBus);
  virtual ~AudioLocalIn();

  void init() override;
  void deinit() override;
  void doAction(System::Action action) override;

  uint16_t* getData(uint32_t length) override;
  void consumedData(uint32_t length) override;
  void notifyDataAvailable() override;

  bool skipNext() override;
  bool skipPrev() override;
  uint32_t getFrequency() override;
};


}
