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

#include "../pub/Dac.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include "OAL/pub/Oal.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/System/pub/SystemController.hpp"

#define REG_CONTROL_1   0x00
#define REG_CONTROL_2   0x01
#define REG_CONTROL_3   0x02
#define REG_L1CH_ATT    0x03
#define REG_R1CH_ATT    0x04
#define REG_CONTROL_4   0x05
#define REG_DSD1        0x06
#define REG_CONTROL_5   0x07
#define REG_SOUND_CTRL  0x08
#define REG_DSD2        0x09
#define REG_CONTROL_6   0x0A
#define REG_CONTROL_7   0x0B
#define REG_CONTROL_8   0x0C
#define REG_RES_1       0x0D
#define REG_RES_2       0x0E
#define REG_RES_3       0x0F
#define REG_RES_4       0x10

#define REG_CONTROL_1_ACKS          0x80  //!< Auto rate detection
#define REG_CONTROL_1_RSTN          0x01  //!< Take DAC out of soft reset
#define REG_CONTROL_2_SOFT_MUTE     0x01

namespace PeripheralInterface
{

Dac::Dac(System::DacInterface dacInterface) :
    dacInterface(dacInterface),
    deviceAddress(0),
    bus(nullptr),
    dacEnableGpio(nullptr),
    gain(0xFF)
{
  state = System::State::UNINITIALISED;
}

void Dac::deinit()
{
// Not supported
}

/**
 * Initialises the UART interface and starts the h/w RX DMA engine
 */
void Dac::init()
{
  System::SystemConfiguration *systemConfiguration = globalServices->getSystemConfiguration();
  System::SystemController *systemController = globalServices->getSystemController();

  auto dacConfiguration = systemConfiguration->getDacInterfaceConfiguration(dacInterface);
  bus = systemController->getBus(dacConfiguration->bus);
  deviceAddress = dacConfiguration->address;
  dacEnableGpio = systemController->getGpio(dacConfiguration->ctrlGpio);

  dacEnableGpio->set(1);

  // Wait for a bit until the DAC PLL has stabilised
  globalServices->getOal()->delay(20);

  if (verifyVersion())
  {
    state = System::State::ENABLED;
  }

  configure();
}

void Dac::doAction(System::Action action)
{
  switch (action)
  {
    case System::Action::RESET:

      // Send a reset pulse
      dacEnableGpio->set(0);
      globalServices->getOal()->delay(10);
      dacEnableGpio->set(1);

      // Wait for a bit until the DAC PLL has stabilised
      globalServices->getOal()->delay(20);

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

/**
 * Reads some of the DAC registers to verify that they contain known values.
 * @return
 */
bool Dac::verifyVersion()
{
  uint32_t readVal = 0;
  uint16_t size = 4;

  auto status = bus->read(deviceAddress, REG_RES_1, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &readVal, &size, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return false;
  }

  if (readVal != 0xFFFF5000)
  {
    state = System::State::ERROR;
    return false;
  }

  return true;
}

/**
 * Configures the DAC registers
 */
void Dac::configure()
{
  mute(true);

  setVolume(gain);

  configureAudioRate();

  configureOverrides();

  //TODO: Remove and make mute and vol a public interface
  mute(false);
}

/**
 * Configures the register values provided by the user
 */
void Dac::configureOverrides()
{
  System::SystemConfiguration *systemConfiguration = globalServices->getSystemConfiguration();
  auto dacConfiguration = systemConfiguration->getDacInterfaceConfiguration(dacInterface);

  for(uint32_t i=0; i< dacConfiguration->registerValueOverrideCount; i++)
  {
    auto& regValues = dacConfiguration->registerValues[i];
    if(regValues.first <= REG_RES_4)
    {
      auto status = bus->write(deviceAddress, regValues.first, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &regValues.second, 1, 100);
      if (status != System::Status::STATUS_OK)
      {
        state = System::State::ERROR;
        return;
      }
    }
  }
}

/**
 * Configures the DAC input timings
 */
void Dac::configureAudioRate()
{
  uint32_t cfgVal = REG_CONTROL_1_ACKS | REG_CONTROL_1_RSTN;

  // 2 channels, 48kHz, 16 bit (mode 0), auto rate detection
  auto status = bus->write(deviceAddress, REG_CONTROL_1, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &cfgVal, 1, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }
}

/**
 * Soft-mutes the DAC
 */
void Dac::mute(bool enable)
{
  uint32_t readVal = 0;
  uint16_t size = 1;

  auto status = bus->read(deviceAddress, REG_CONTROL_2, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &readVal, &size, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }

  readVal = (enable) ? readVal | REG_CONTROL_2_SOFT_MUTE : readVal & ~REG_CONTROL_2_SOFT_MUTE;
  status = bus->write(deviceAddress, REG_CONTROL_2, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &readVal, 1, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }
}

/**
 * Sets the volume for both channels
 * @param vol
 */
void Dac::setVolume(uint8_t vol)
{
  uint8_t volArray[] = { vol, vol };
  gain = vol;

  auto status = bus->write(deviceAddress, REG_L1CH_ATT, I2C_MEMADD_SIZE_8BIT, volArray, 2, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }
}

}
