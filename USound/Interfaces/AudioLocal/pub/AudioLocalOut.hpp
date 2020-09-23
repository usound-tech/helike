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
 * Defines if the SAI is the bus master or slave
 */
enum SaiMasterMode
{
  MASTER, //!< TWEETER
  SLAVE    //!< WOOFER
};



/**
 * This class manages the SAI OUT peripheral
 */
class AudioLocalOut: public GlobalServiceConsumer, public System::AudioSink<uint16_t>
{
private:
  uint32_t frequency = 0;

private:
  void start();
  void stop();

public:
  AudioLocalOut();
  virtual ~AudioLocalOut();

  void init() override;
  void deinit() override;
  void doAction(System::Action action) override;

  void enqueueData(uint16_t *data, uint32_t length, uint32_t interface) override;
  uint32_t getFrequency() override;

  void mute(bool enable) override;
  void setVolume(uint8_t vol) override;
};


}
