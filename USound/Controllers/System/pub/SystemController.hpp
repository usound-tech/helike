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
//  Filename: SystemController.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Interfaces/pub/SystemControl.hpp"
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/Console/pub/Console.hpp"

namespace Controller
{
class Telemetry;
}

namespace System
{

/**
 * This class manages the status and operational integrity of all the system peripherals.
 */
class SystemController: public GlobalServiceConsumer
{
private:

  //!< This table holds the pointers to all the system peripherals
  Peripheral *systemPeripherals[PERIPHERAL_COUNT];

  //!< This table holds the pointers to all the system buses
  Bus *systemBuses[BUS_COUNT];

  //!< This table holds the instances of all consoles
  Controller::Console *systemConsoles[CONSOLE_COUNT];

  //!< This table holds the instances of all gpios
  Gpio *systemGpios[GPIO_COUNT];

  //!< This table holds the instances of all audio sinks
  System::AudioSink<uint16_t> *systemAudioSinks[AUDIO_SINK_COUNT];

  //!< This table holds the instances of all audio sources
  System::AudioSource<uint16_t> *systemAudioSources[AUDIO_SRC_COUNT];

  //!< This object holds the pointer to the telemetry controller
  Controller::Telemetry *telemetry;

  void initBuses();
  void initPeripherals();
  void initFilesystem();
  void initConsoles();
  void initGpios();
  void initSystemStatus();
  void initAudioEngine();
  void updateSystemConfigFromDipSwitches();

public:
  SystemController();

  void init();
  void doAction(SystemPeripheral peripheral, Action action);
  State getState(SystemPeripheral peripheral);

  /**
   * Returns the instance of a system bus.
   * It may be null if the configuration hasn't included it.
   *
   * @param systemBus
   * @return
   */
  Bus* getBus(SystemBus systemBus) const
  {
    return (systemBus == SystemBus::NO_BUS) ? nullptr : systemBuses[systemBus];
  }

  /**
   * Returns the instance of a system peripheral.
   * It may be null if the configuration hasn't included it.
   *
   * @param systemBus
   * @return
   */
  Peripheral* getPeripheral(SystemPeripheral systemPeripheral) const
  {
    return systemPeripherals[systemPeripheral];
  }

  /**
   * Returns the instance of a system gpio.
   * It may be null if the configuration hasn't included it.
   *
   * @param systemBus
   * @return
   */
  Gpio* getGpio(GpioInterface systemGpio) const
  {
    return (systemGpio == GpioInterface::NO_GPIO) ? nullptr : systemGpios[systemGpio];
  }

  /**
   * Returns the instance of a system gpio associated with the requested event
   * @param event
   * @return
   */
  Gpio* getGpioForEvent(uint16_t event) const
  {
    for (Gpio *gpio : systemGpios)
    {
      if (gpio && (gpio->getExtEvent() == event))
      {
        return gpio;
      }
    }

    return nullptr;
  }

  /**
   * Returns the instance of a system audio source
   * It may be null if the configuration hasn't included it.
   *
   * @param systemBus
   * @return
   */
  System::AudioSource<uint16_t>* getAudioSource(SystemAudioSource systemAudioSrc) const
  {
    return systemAudioSources[systemAudioSrc];
  }

  /**
   * Returns the instance of a system audio sink
   * It may be null if the configuration hasn't included it.
   *
   * @param systemBus
   * @return
   */
  System::AudioSink<uint16_t>* getAudioSink(SystemAudioSink systemAudioSink) const
  {
    return systemAudioSinks[systemAudioSink];
  }
};

}
