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
//  Description: System controller is responsible for enabling/disabling and
//               reporting peripheral status.
//  Filename: SystemController.c
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include <Controllers/FilePlayer/pub/AudioPlayer.hpp>
#include "../pub/SystemConfiguration.hpp"
#include "../pub/SystemController.hpp"
#include "../pub/SystemStatus.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include "OAL/pub/Hal.hpp"
#include "OAL/pub/Oal.hpp"
#include "Controllers/ToneGen/pub/ToneGen.hpp"
#include "Controllers/Audio/pub/AudioService.hpp"
#include "Interfaces/Dac/pub/Dac.hpp"
#include "Interfaces/Amp/pub/Amp.hpp"
#include "Interfaces/MclkPll/pub/MclkPll.hpp"
#include "Interfaces/AudioLocal/pub/AudioLocalOut.hpp"
#include "Interfaces/AudioLocal/pub/AudioLocalIn.hpp"
#include "Controllers/Filesystem/pub/Filesystem.hpp"
#include "gpio.h"

namespace System
{

/**
 * System controller constructor
 */
SystemController::SystemController()
{

}

/**
 * Creates all the peripheral instances and stores them in the internal array.
 */
void SystemController::init()
{
  //TODO: Read the boot system config to know which peripherals to bring up
  initGpios();

  updateSystemConfigFromDipSwitches();

  initBuses();
  initSystemStatus();
  initFilesystem();
  initPeripherals();
  initConsoles();

  auto systemConfiguration = globalServices->getSystemConfiguration();
  systemConfiguration->updateFilterConfiguration();

  initAudioEngine();
}

/**
 * Initialises the system buses. This step must be done before any peripheral initialisation,
 * as some of the buses are used to access the peripherals during their init phase.
 */
void SystemController::initBuses()
{
  HalFactory halFactory;

  auto systemConfiguration = globalServices->getSystemConfiguration();

  void *handle = systemConfiguration->getHandleForSystemBus(SystemBus::I2C_TWEETER);
  systemBuses[SystemBus::I2C_TWEETER] = handle ? halFactory.getI2c(handle, I2cInterface::I2C_TWEETER_DAC) : nullptr;

  handle = systemConfiguration->getHandleForSystemBus(SystemBus::I2C_WOOFER);
  systemBuses[SystemBus::I2C_WOOFER] = handle ? halFactory.getI2c(handle, I2cInterface::I2C_WOOFER_DAC) : nullptr;

  handle = systemConfiguration->getHandleForSystemBus(SystemBus::CTRL_UART);
  systemBuses[SystemBus::CTRL_UART] = handle ? halFactory.getUart(handle, UartInterface::UART_CTRL) : nullptr;

  handle = systemConfiguration->getHandleForSystemBus(SystemBus::DEBUG_UART);
  systemBuses[SystemBus::DEBUG_UART] = handle ? halFactory.getUart(handle, UartInterface::UART_DEBUG) : nullptr;

  handle = systemConfiguration->getHandleForSystemBus(SystemBus::SAI_TWEETER);
  SaiConfiguration *saiConfiguration = systemConfiguration->getSaiInterfaceConfiguration(SaiInterface::TWEETER);
  systemBuses[SystemBus::SAI_TWEETER] = handle ? halFactory.getSaiOut(handle, saiConfiguration) : nullptr;

  handle = systemConfiguration->getHandleForSystemBus(SystemBus::SAI_WOOFER);
  saiConfiguration = systemConfiguration->getSaiInterfaceConfiguration(SaiInterface::WOOFER);
  systemBuses[SystemBus::SAI_WOOFER] = handle ? halFactory.getSaiOut(handle, saiConfiguration) : nullptr;

  handle = systemConfiguration->getHandleForSystemBus(SystemBus::SAI_IN);
  saiConfiguration = systemConfiguration->getSaiInterfaceConfiguration(SaiInterface::IN);
  systemBuses[SystemBus::SAI_IN] = handle ? halFactory.getSaiIn(handle, saiConfiguration) : nullptr;
}

/**
 * Initialises the system gpios. This step must be done before any peripheral initialisation,
 * as some of the gpios are used to access the peripherals during their init phase.
 */
void SystemController::initGpios()
{
  HalFactory halFactory;
  GpioConfiguration *gpioConfig;
  auto systemConfiguration = globalServices->getSystemConfiguration();

  for (int i = 0; i < GpioInterface::GPIO_COUNT; i++)
  {
    gpioConfig = systemConfiguration->getGpioInterfaceConfiguration((GpioInterface) i);

    if (!gpioConfig)
    {
      systemGpios[(GpioInterface) i] = nullptr;
    }
    else if (gpioConfig->isLongKeyPress)
    {
      systemGpios[(GpioInterface) i] = halFactory.getLongPressGpio(gpioConfig);
    }
    else
    {
      systemGpios[(GpioInterface) i] = halFactory.getGpio(gpioConfig);
    }
  }

  MX_GPIO_ControlInterrupts(1);
}

/**
 * Initialises the system status reporter.
 */
void SystemController::initSystemStatus()
{
  globalServices->getSystemStatus()->init();
}

/**
 * Initialises the sd card filesystem
 */
void SystemController::initFilesystem()
{
  AudioMode audioMode = globalServices->getSystemConfiguration()->getAudioMode();
  if (audioMode == AudioMode::AM_MP3)
  {
    globalServices->getFilesystem()->mount();
  }
}

/**
 * Initialises the registered peripherals. The list of peripherals per configuration is retrieved from
 * the SystemConfiguration.
 */
void SystemController::initPeripherals()
{
  auto systemConfiguration = globalServices->getSystemConfiguration();

  if (systemConfiguration->isPeripheralAvailable(SystemPeripheral::DAC_TWEETER))
  {
    systemPeripherals[SystemPeripheral::DAC_TWEETER] = new PeripheralInterface::Dac(DacInterface::DAC_TWEETER_IFACE);
    systemPeripherals[SystemPeripheral::DAC_TWEETER]->init();
  }

  if (systemConfiguration->isPeripheralAvailable(SystemPeripheral::DAC_WOOFER))
  {
    systemPeripherals[SystemPeripheral::DAC_WOOFER] = new PeripheralInterface::Dac(DacInterface::DAC_WOOFER_IFACE);
    systemPeripherals[SystemPeripheral::DAC_WOOFER]->init();
  }

  if (systemConfiguration->isPeripheralAvailable(SystemPeripheral::TWEETER_AMP_R))
  {
    systemPeripherals[SystemPeripheral::TWEETER_AMP_R] = new PeripheralInterface::Amp(AmpInterface::AMP_TWEETER_R);
    systemPeripherals[SystemPeripheral::TWEETER_AMP_R]->init();
  }

  if (systemConfiguration->isPeripheralAvailable(SystemPeripheral::WOOFER_AMP_R))
  {
    systemPeripherals[SystemPeripheral::WOOFER_AMP_R] = new PeripheralInterface::Amp(AmpInterface::AMP_WOOFER_R);
    systemPeripherals[SystemPeripheral::WOOFER_AMP_R]->init();
  }

  if (systemConfiguration->isPeripheralAvailable(SystemPeripheral::WOOFER_AMP_L))
  {
    systemPeripherals[SystemPeripheral::WOOFER_AMP_L] = new PeripheralInterface::Amp(AmpInterface::AMP_WOOFER_L);
    systemPeripherals[SystemPeripheral::WOOFER_AMP_L]->init();
  }

  if (systemConfiguration->isPeripheralAvailable(SystemPeripheral::MCLK_PLL))
  {
    systemPeripherals[SystemPeripheral::MCLK_PLL] = new PeripheralInterface::MclkPll();
    systemPeripherals[SystemPeripheral::MCLK_PLL]->init();
  }
}


/**
 * Initialises the registered peripherals. The list of peripherals per configuration is retrieved from
 * the SystemConfiguration.
 */
void SystemController::initConsoles()
{
  auto systemConfiguration = globalServices->getSystemConfiguration();

  systemConsoles[SystemConsole::CONSOLE_CTRL] = nullptr;
  systemConsoles[SystemConsole::CONSOLE_DEBUG] = nullptr;

  if (systemBuses[SystemBus::CTRL_UART])
  {
    auto consoleConfig = systemConfiguration->getConsoleInterfaceConfiguration(UartInterface::UART_CTRL);
    if (consoleConfig)
    {
      systemConsoles[SystemConsole::CONSOLE_CTRL] = new Controller::Console(consoleConfig, SystemBus::CTRL_UART);
      systemConsoles[SystemConsole::CONSOLE_CTRL]->init();
    }
  }

  if (systemBuses[SystemBus::DEBUG_UART])
  {
    auto consoleConfig = systemConfiguration->getConsoleInterfaceConfiguration(UartInterface::UART_DEBUG);
    if (consoleConfig)
    {
      systemConsoles[SystemConsole::CONSOLE_DEBUG] = new Controller::Console(consoleConfig, SystemBus::DEBUG_UART);
      systemConsoles[SystemConsole::CONSOLE_DEBUG]->init();
    }
  }
}

/**
 * Initialises the audio service
 */
void SystemController::initAudioEngine()
{
  auto systemConfiguration = globalServices->getSystemConfiguration();

  if (systemConfiguration->isAudioSourceAvailable(SystemAudioSource::AUDIO_SRC_FILE))
  {
    systemAudioSources[SystemAudioSource::AUDIO_SRC_FILE] = new Controller::AudioPlayer();
    systemAudioSources[SystemAudioSource::AUDIO_SRC_FILE]->init();
  }

  if (systemConfiguration->isAudioSourceAvailable(SystemAudioSource::AUDIO_SRC_SAI))
  {
    systemAudioSources[SystemAudioSource::AUDIO_SRC_SAI] = new PeripheralInterface::AudioLocalIn();
    systemAudioSources[SystemAudioSource::AUDIO_SRC_SAI]->init();
  }

  if (systemConfiguration->isAudioSourceAvailable(SystemAudioSource::AUDIO_SRC_GENERATOR))
  {
    systemAudioSources[SystemAudioSource::AUDIO_SRC_GENERATOR] = new Controller::ToneGen();
    systemAudioSources[SystemAudioSource::AUDIO_SRC_GENERATOR]->init();
  }

  if (systemConfiguration->isAudioSinkAvailable(SystemAudioSink::AUDIO_SINK_LOCAL))
  {
    systemAudioSinks[SystemAudioSink::AUDIO_SINK_LOCAL] = new PeripheralInterface::AudioLocalOut();
    systemAudioSinks[SystemAudioSink::AUDIO_SINK_LOCAL]->init();
  }

  AudioMode audioMode = systemConfiguration->getAudioMode();

  switch (audioMode)
  {
    case AudioMode::AM_MP3:
      globalServices->getAudioService()->selectAudioSink(SystemAudioSink::AUDIO_SINK_LOCAL);
      globalServices->getAudioService()->selectAudioSource(SystemAudioSource::AUDIO_SRC_FILE);
      break;

    case AudioMode::AM_I2S_SLAVE:
      default:
      globalServices->getAudioService()->selectAudioSink(SystemAudioSink::AUDIO_SINK_LOCAL);
      globalServices->getAudioService()->selectAudioSource(SystemAudioSource::AUDIO_SRC_SAI);
      break;
  }

  globalServices->getAudioService()->init();
}

/**
 * Instructs the system controller to trigger a peripheral for an action.
 *
 * @param peripheral the peripheral to perform the action
 * @param action the type of the action
 */
void SystemController::doAction(SystemPeripheral peripheral, Action action)
{
  switch (peripheral)
  {
    case SystemPeripheral::DAC_TWEETER:
      break;

    default:
      break;
  }
}

/**
 * Returns the current state of a peripheral.
 *
 * @param peripheral the peripheral to be queried
 * @return the state of the peripheral
 */
State SystemController::getState(SystemPeripheral peripheral)
{
  return State::DISABLED;
}

/**
 * Reads the dip switches and triggers the system config update.
 */
void SystemController::updateSystemConfigFromDipSwitches()
{
  uint32_t dipSwitches = 0;

#if CONFIG_POS == 1
  dipSwitches = (1 << GpioInterface::GPIO_DIPSWITCH_2) | (1 << GpioInterface::GPIO_DIPSWITCH_4);
#else
  for (uint32_t i = 0; i < 8; i++)
  {
    dipSwitches |= (!systemGpios[GpioInterface::GPIO_DIPSWITCH_1 + i]->get() << (GpioInterface::GPIO_DIPSWITCH_1 + i));
  }

#endif

  auto systemConfiguration = globalServices->getSystemConfiguration();
  systemConfiguration->updateConfigFromDipSwitches(dipSwitches);
}

const char* toString(State state)
{
  switch (state)
  {
    case State::UNINITIALISED:
      return "UNINITIALISED";

    case State::ENABLED:
      return "ENABLED";

    case State::DISABLED:
      return "DISABLED";

    case State::ACTIVE:
      return "ACTIVE";

    case State::PAUSED:
      return "PAUSED";

    case State::ERROR:
      return "ERROR";
  }

  return "UNKNOWN";
}

}
