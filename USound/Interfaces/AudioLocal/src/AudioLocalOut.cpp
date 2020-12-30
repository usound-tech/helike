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

#include "Interfaces/Dac/pub/Dac.hpp"
#include "Interfaces/AudioLocal/pub/AudioLocalOut.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/System/pub/SystemController.hpp"
#include "Controllers/Audio/pub/AudioService.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "sai.h"


//TODO: Add input jitter buffer
//TODO: Add rate calculation on dma end

namespace PeripheralInterface
{

/**
 * Initialises the Audio Local interface instance
 */
AudioLocalOut::AudioLocalOut()
{

}

/**
 * Cleans up the resources before deleting the instance
 */
AudioLocalOut::~AudioLocalOut()
{

}

/**
 * Initialises the Audio local.
 */
void AudioLocalOut::init()
{
}

/**
 * Cleans up the endpoint resources
 */
void AudioLocalOut::deinit()
{
}

/**
 * Requests a change to the Audio Local state
 * @param action the type of the action to be performed
 */
void AudioLocalOut::doAction(System::Action action)
{
  switch (action)
  {
    case System::Action::ENABLE:
      break;

    case System::Action::DISABLE:
      deinit();
      break;

    case System::Action::START:

      start();
      break;

    case System::Action::STOP:
      stop();
      break;

    default:
      throw System::PeripheralException("Unknown action for AudioSaiOut: " + action);
  }
}

/**
 * Starts the SAI dma and enables the SAI interface if it's disabled
 */
void AudioLocalOut::start()
{
  auto systemController = globalServices->getSystemController();

  frequency = globalServices->getSystemConfiguration()->getSaiInterfaceConfiguration(System::SaiInterface::TWEETER)->frequency;

  auto saiTweeterBus = systemController->getBus(System::SystemBus::SAI_TWEETER);
  auto saiWooferBus = systemController->getBus(System::SystemBus::SAI_WOOFER);
  auto dacTweeter = systemController->getPeripheral(System::SystemPeripheral::DAC_TWEETER);
  auto dacWoofer = systemController->getPeripheral(System::SystemPeripheral::DAC_WOOFER);
  auto ampTweeterR = systemController->getPeripheral(System::SystemPeripheral::TWEETER_AMP_R);
  //auto ampTweeterL = systemController->getPeripheral(System::SystemPeripheral::TWEETER_AMP_L);
  auto ampWooferR = systemController->getPeripheral(System::SystemPeripheral::WOOFER_AMP_R);
  auto ampWooferL = systemController->getPeripheral(System::SystemPeripheral::WOOFER_AMP_L);

  saiWooferBus->enable();
  saiTweeterBus->enable();

  ampTweeterR->doAction(System::Action::START);
  //ampTweeterL->doAction(System::Action::START);

  ampWooferR->doAction(System::Action::START);
  ampWooferL->doAction(System::Action::START);

  //TODO: Check the effect of reset on standalone mode
  //TODO: This is not needed all the time, just do it once
  dacTweeter->doAction(System::Action::RESET);
  dacWoofer->doAction(System::Action::RESET);
}

/**
 * Stops the SAI DMA and disables the SAI interface
 */
void AudioLocalOut::stop()
{
  auto systemController = globalServices->getSystemController();

  auto saiTweeterBus = systemController->getBus(System::SystemBus::SAI_TWEETER);
  auto saiWooferBus = systemController->getBus(System::SystemBus::SAI_WOOFER);
  auto dacTweeter = systemController->getPeripheral(System::SystemPeripheral::DAC_TWEETER);
  auto dacWoofer = systemController->getPeripheral(System::SystemPeripheral::DAC_WOOFER);
  auto ampTweeterR = systemController->getPeripheral(System::SystemPeripheral::TWEETER_AMP_R);
  //auto ampTweeterL = systemController->getPeripheral(System::SystemPeripheral::TWEETER_AMP_L);
  auto ampWooferR = systemController->getPeripheral(System::SystemPeripheral::WOOFER_AMP_R);
  auto ampWooferL = systemController->getPeripheral(System::SystemPeripheral::WOOFER_AMP_L);

  ampWooferR->doAction(System::Action::STOP);
  ampWooferL->doAction(System::Action::STOP);

  ampTweeterR->doAction(System::Action::STOP);
  //ampTweeterL->doAction(System::Action::STOP);

  dacTweeter->doAction(System::Action::STOP);
  dacWoofer->doAction(System::Action::STOP);

  saiWooferBus->disable();
  saiTweeterBus->disable();
}

/**
 * Enqueues the data to the DMA fifo from an array
 * @param data
 * @param length
 */
void AudioLocalOut::enqueueData(uint16_t *data, uint32_t length, uint32_t interface)
{
  System::Bus *bus;
  auto systemController = globalServices->getSystemController();

  if (interface == System::SaiInterface::TWEETER)
  {
    bus = systemController->getBus(System::SystemBus::SAI_TWEETER);
  }
  else
  {
    bus = systemController->getBus(System::SystemBus::SAI_WOOFER);
  }

  bus->write(0, 0, 0, (uint8_t*) data, length, 0);
}

/**
 * Returns the SAI frequency (in Hz)
 * @return
 */
uint32_t AudioLocalOut::getFrequency()
{
  return frequency;
}

/**
 * Mutes/unmutes the DACs
 * @param enable
 */
void AudioLocalOut::mute(bool enable)
{
  auto systemController = globalServices->getSystemController();
  auto dacTweeter = (PeripheralInterface::Dac*) systemController->getPeripheral(System::SystemPeripheral::DAC_TWEETER);
  auto dacWoofer = (PeripheralInterface::Dac*) systemController->getPeripheral(System::SystemPeripheral::DAC_WOOFER);

  dacTweeter->mute(enable);
  dacWoofer->mute(enable);
}

/**
 * Sets the DAC volume
 * @param vol
 */
void AudioLocalOut::setVolume(uint8_t vol)
{
  auto systemController = globalServices->getSystemController();
  auto dacTweeter = (PeripheralInterface::Dac*) systemController->getPeripheral(System::SystemPeripheral::DAC_TWEETER);
  auto dacWoofer = (PeripheralInterface::Dac*) systemController->getPeripheral(System::SystemPeripheral::DAC_WOOFER);

  dacTweeter->setVolume(vol);
  dacWoofer->setVolume(vol);
}

}
