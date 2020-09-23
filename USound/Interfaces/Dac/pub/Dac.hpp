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
//  Description: Dac interface
//  Filename: Dac.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Interfaces/pub/SystemControl.hpp"
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"

namespace PeripheralInterface
{

/**
 * This class manages UART peripherals and the DMA RX channels.
 */
class Dac: public System::Peripheral, public GlobalServiceConsumer
{
private:

  System::DacInterface dacInterface;
  uint32_t deviceAddress;
  System::Bus *bus;
  System::Gpio *dacEnableGpio;

  bool verifyVersion();
  void configure();
  void configureAudioRate();

public:

  Dac(System::DacInterface dacInterface);
  virtual ~Dac()
  {
  }

  void init() override;
  void deinit() override;
  void doAction(System::Action action) override;

  void mute(bool enable);
  void setVolume(uint8_t vol);
};


}
