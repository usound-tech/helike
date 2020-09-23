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
//  Description: Mems/Dynamic Amp interface
//  Filename: Amp.hpp
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
 * Sets the AGC compression ration to the dynamic AMPs
 */
enum AmpAgcRatio
{
  RATIO_1_1,
  RATIO_2_1,
  RATIO_4_1,
  RATIO_8_1
};

/**
 * This class manages UART peripherals and the DMA RX channels.
 */
class Amp: public System::Peripheral, public GlobalServiceConsumer
{
private:

  System::AmpInterface ampInterface;
  uint32_t deviceAddress;
  System::Bus *bus;
  System::Gpio *ampEnableGpio;

  bool verifyVersion();
  void configure();
  void setAgcRatio(AmpAgcRatio agcRatio);
  void setFixedGain(int8_t fixedGain);

public:

  Amp(System::AmpInterface ampInterface);
  virtual ~Amp()
  {
  }

  void init() override;
  void deinit() override;
  void doAction(System::Action action) override;
};


}
