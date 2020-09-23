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
//  Description: Mlck Pll interface
//  Filename: MclkPll.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../pub/MclkPll.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include "OAL/pub/Oal.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/System/pub/SystemController.hpp"

#define REG_DEVICE_ID     0x01
#define REG_DEVICE_CTRL   0x02
#define REG_DEVICE_CFG1   0x03
#define REG_GLOBAL_CFG1   0x05
#define REG_RATIO_1       0x06
#define REG_RATIO_2       0x07
#define REG_RATIO_3       0x08
#define REG_RATIO_4       0x09

#define REG_FUNCT_CFG1    0x16
#define REG_FUNCT_CFG2    0x17
#define REG_FUNCT_CFG3    0x1E

#define AUTO_INC          0x80


namespace PeripheralInterface
{

MclkPll::MclkPll()
{
  state = System::State::UNINITIALISED;
}

void MclkPll::deinit()
{
// Not supported
}

/**
 * Initialises the MclkPll interface, detects its presence and sets the configuration
 */
void MclkPll::init()
{
  System::SystemConfiguration *systemConfiguration = globalServices->getSystemConfiguration();
  System::SystemController *systemController = globalServices->getSystemController();

  auto mclkConfiguration = systemConfiguration->getMclkPllConfiguration();
  bus = systemController->getBus(mclkConfiguration->bus);
  deviceAddress = mclkConfiguration->address;

  if (!verifyVersion())
  {
    return;
  }
  configure();

  state = System::State::ENABLED;
}

void MclkPll::doAction(System::Action action)
{
  switch (action)
  {
    case System::Action::RESET:
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
 * Reads the version register from the PLL and compares it with known values
 */
bool MclkPll::verifyVersion()
{
  uint32_t readVal = 0;
  uint16_t size = 1;

  auto status = bus->read(deviceAddress, REG_DEVICE_ID, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &readVal, &size, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return false;
  }

  if (readVal != 0x04 && readVal != 0x06)
  {
    state = System::State::ERROR;
    return false;
  }

  return true;
}

/**
 * Configures the dynamic amp registers
 */
void MclkPll::configure()
{
  uint32_t writeVal;

  writeVal = 1;
  auto status = bus->write(deviceAddress, REG_DEVICE_CFG1, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &writeVal, 1, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }

  writeVal = 1;
  status = bus->write(deviceAddress, REG_GLOBAL_CFG1, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &writeVal, 1, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }

  writeVal = 0x10;
  status = bus->write(deviceAddress, REG_FUNCT_CFG1, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &writeVal, 1, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }

  writeVal = 0x800000;
  status = bus->write(deviceAddress, REG_RATIO_1 | AUTO_INC, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &writeVal, 4, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }

  writeVal = 0;
  status = bus->write(deviceAddress, REG_DEVICE_CFG1, I2C_MEMADD_SIZE_8BIT, (uint8_t*) &writeVal, 1, 100);
  if (status != System::Status::STATUS_OK)
  {
    state = System::State::ERROR;
    return;
  }

}

}
