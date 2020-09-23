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
//  Filename: Amp.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../pub/Amp.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include "OAL/pub/Oal.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/System/pub/SystemController.hpp"

#define REG_FUNCTION_CTRL     0x01
#define REG_AGC_ATTACK_CTRL   0x02
#define REG_AGC_RELEASE_CTRL  0x03
#define REG_AGC_HOLD_CTRL     0x04
#define REG_AGC_GAIN_CTRL     0x05
#define REG_AGC_CTRL_0        0x06
#define REG_AGC_CTRL_1        0x07

namespace PeripheralInterface
{

Amp::Amp(System::AmpInterface ampInterface) :
    ampInterface(ampInterface),
    deviceAddress(0),
    bus(nullptr),
    ampEnableGpio(nullptr)
{
  state = System::State::UNINITIALISED;
}

void Amp::deinit()
{
// Not supported
}

/**
 * Initialises the UART interface and starts the h/w RX DMA engine
 */
void Amp::init()
{
  System::SystemConfiguration *systemConfiguration = globalServices->getSystemConfiguration();
  System::SystemController *systemController = globalServices->getSystemController();

  auto ampConfiguration = systemConfiguration->getAmpInterfaceConfiguration(ampInterface);
  bus = systemController->getBus(ampConfiguration->bus);
  deviceAddress = ampConfiguration->address;
  ampEnableGpio = systemController->getGpio(ampConfiguration->ctrlGpio);

  if (ampEnableGpio)
  {
    ampEnableGpio->set(1);
  }

  if (bus)
  {
    globalServices->getOal()->delay(20);
    if (!verifyVersion())
    {
      return;
    }
    configure();
  }

  state = System::State::ENABLED;
}

void Amp::doAction(System::Action action)
{
  switch (action)
  {
    case System::Action::RESET:
      // Send a reset pulse
      if (ampEnableGpio)
      {
        ampEnableGpio->set(0);
        globalServices->getOal()->delay(15);
        ampEnableGpio->set(1);
        globalServices->getOal()->delay(15);
      }

      state = System::State::UNINITIALISED;

      if (verifyVersion())
      {
        state = System::State::ENABLED;

        configure();
      }
      break;

    default:
      break;
  }
}

bool Amp::verifyVersion()
{
  if (!bus)
  {
    return true;
  }

  uint32_t readVal = 0;
  uint16_t size = 4;

  auto status = bus->read(deviceAddress, REG_AGC_ATTACK_CTRL, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &readVal, &size, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return false;
  }

  if (readVal != 0x06000B05)
  {
    state = System::State::ERROR;
    return false;
  }

  return true;
}

/**
 * Configures the dynamic amp registers
 */
void Amp::configure()
{
  setAgcRatio(AmpAgcRatio::RATIO_1_1);
  setFixedGain(12);
}

/**
 * Sets the AGC ratio to the dynamic amp
 */
void Amp::setAgcRatio(AmpAgcRatio agcRatio)
{
  uint32_t readVal = 0;
  uint16_t size = 1;

  auto status = bus->read(deviceAddress, REG_AGC_CTRL_1, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &readVal, &size, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }

  readVal = (readVal & 0xFC) | agcRatio;

  status = bus->write(deviceAddress, REG_AGC_CTRL_1, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &readVal, 1, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }
}

/**
 * Sets the gain value
 */
void Amp::setFixedGain(int8_t fixedGain)
{
  int8_t gain = 0;

  if (fixedGain > 30)
  {
    gain = 30;
  }
  else if (fixedGain < -28)
  {
    gain = -28;
  }
  else
  {
    gain = fixedGain;
  }

  auto status = bus->write(deviceAddress, REG_AGC_GAIN_CTRL, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &gain, 1, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
  }
}

}
