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
//  Description: System configuration is responsible for loading the boot
//              configuration and persisting any changes.
//  Filename: SystemConfiguration.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include <utility>
#include "OAL/pub/Hal.hpp"
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/ModuleConfig.hpp"
#include "Interfaces/pub/SystemControl.hpp"
#include "stm32h7xx_hal.h"
#include "SystemInterfaces.hpp"
#include "arm_math.h"

#define MASTER_EQ_STAGES 8
#define XOVER_EQ_STAGES 8
#define PREDISTORTION_STAGES 8

namespace System
{


/**
 * Defines the system configuration attributes of a DAC interface
 */
struct DacConfiguration
{
public:
  uint32_t address;             //!< The dec address (useful when DAC is attached to an I2C bus
  SystemBus bus;                //!< The associated system bus id
  GpioInterface ctrlGpio;       //!< The associated gpio line

  std::pair<uint8_t, uint8_t> registerValues[16];   //!< Register value overrides
  uint32_t registerValueOverrideCount = 0;          //!< Number of register override entries

  DacConfiguration(uint32_t address, SystemBus bus, GpioInterface ctrlGpio) :
      address(address),
      bus(bus),
      ctrlGpio(ctrlGpio)
  {
  }

};

/**
 * Defines the system configuration attributes of a GPIO interface
 */
struct GpioConfiguration
{
public:
  void *port;             //!< The gpio port
  uint32_t pin;           //!< The gpio pin
  uint16_t inputEvent;    //!< The associated EXTI event (0 if not interrupt driven
  bool isLongKeyPress;    //!< When true, it supports repeated soft events as long as the key is pressed

  GpioConfiguration(void *port, uint32_t pin, uint16_t extiEvent, bool isLongKeyPress) :
      port(port),
      pin(pin),
      inputEvent(extiEvent),
      isLongKeyPress(isLongKeyPress)
  {
  }
};

/**
 * Defines the system configuration attributes of an Amp interface
 */
struct AmpConfiguration
{
public:
  uint32_t address;             //!< The dec address (useful when DAC is attached to an I2C bus
  SystemBus bus;                //!< The associated system bus id
  GpioInterface ctrlGpio;       //!< The associated gpio line

  std::pair<uint8_t, uint8_t> registerValues[16];   //!< Register value overrides
  uint32_t registerValueOverrideCount = 0;                    //!< Number of register override entries

  AmpConfiguration(uint32_t address, SystemBus bus, GpioInterface ctrlGpio) :
      address(address),
      bus(bus),
      ctrlGpio(ctrlGpio)
  {
  }


};

/**
 * Defines the system configuration attributes of the Mclk PLL interface
 */
struct MclkPllConfiguration
{
public:
  uint32_t address;             //!< The dec address (useful when DAC is attached to an I2C bus
  SystemBus bus;                //!< The associated system bus id

  MclkPllConfiguration(uint32_t address, SystemBus bus) :
      address(address),
      bus(bus)
  {
  }
};

/**
 * Defines the system configuration attributes of a console
 */
struct ConsoleConfiguration
{
public:
  bool hasHistory;                //!< If console supports history
  UartInterface uartInterface;    //!< The associated UART interface

  ConsoleConfiguration(bool hasHistory, UartInterface uartInterface) :
      hasHistory(hasHistory),
      uartInterface(uartInterface)
  {
  }
};

/**
 * Defines the system configuration attributes of a DRC block
 */
struct DrcConfiguration
{
public:
  float32_t attackDuration = 0.2f;                      // 200 ms
  float32_t releaseDuration = 1.0f;                     // 1 sec
  float32_t compressionThresholdFullScaleDb = 20.0f;    //!< Threshold for compression, relative to digital full scale
  float32_t compressionRatio = 2.0f;                    //<! Compression ratio
  float32_t sampleRateHz = 48000;                       //!< The audio sampling rate
  float32_t postGain = 0.0f;                            //!< Make-up gain to adjust the DRC result
  bool enabled = false;                                 //!< If false, the DRC block is bypassed
};

enum PredistortionCoeffcientType
{
  UPSAMPLING,
  DOWNSAMPLING,
  MAX_PREDISTORTION_COEFF_TYPES
};

/**
 * Defines the pre-distortion configuration attributes
 */
struct PredistortionConfiguration
{
public:
  float32_t coeffs[PredistortionCoeffcientType::MAX_PREDISTORTION_COEFF_TYPES][PREDISTORTION_STAGES * 5];  //!< Predistortion coefficients
  float32_t *inputRange = nullptr;
  float32_t *outputRange = nullptr;
  uint32_t rangeSize = 0;
  bool enabled = false;

};

enum MasterEqCoeffcientType
{
  LEFT,
  RIGHT,
  MAX_MASTER_EQ_COEFF_TYPES
};

enum XoverEqCoeffcientType
{
  LEFT_WOOFER,
  LEFT_TWEETER,
  RIGHT_WOOFER,
  RIGHT_TWEETER,
  MAX_XOVER_EQ_COEFF_TYPES
};


/**
 * Defines the system configuration attributes of the filter pipeline (per target speaker)
 */
struct FilterConfiguration
{
public:
  std::string name;
  bool masterEqEnabled = true;
  bool xoverEqEnabled = true;
  float32_t masterEqCoeffs[MasterEqCoeffcientType::MAX_MASTER_EQ_COEFF_TYPES][MASTER_EQ_STAGES * 5];    //!< Left/right channel coefficients
  float32_t xoverEqCoeffs[XoverEqCoeffcientType::MAX_XOVER_EQ_COEFF_TYPES][XOVER_EQ_STAGES * 5];      //!< Left/right channel coefficients for tweeter/woofer
  DrcConfiguration levelerDrcConfig;
  DrcConfiguration limiterDrcConfig;

#if PREDISTORTION_MODULE_ENABLED == 1
  PredistortionConfiguration predistortionConfig;
#endif
};

/**
 * Defines the SAI mode of operation
 */
enum SaiMode
{
  SAI_MODE_TX_MASTER,
  SAI_MODE_TX_SLAVE_INTERNAL,
  SAI_MODE_TX_SLAVE_EXTERNAL,
  SAI_MODE_RX_SLAVE_INTERNAL,
  SAI_MODE_RX_SLAVE_EXTERNAL
};

/**
 * Defines the system configuration attributes of a Sai bus
 */
struct SaiConfiguration
{
public:
  uint32_t bufferingTime;       //!< Milliseconds of dma buffering
  uint32_t frequency;
  void *handle;
  SaiInterface saiInterface;    //!< The associated Sai interface
  SaiMode saiMode;

  SaiConfiguration(uint32_t bufferingTime, uint32_t frequency, void *handle, SaiInterface saiInterface, SaiMode saiMode) :
      bufferingTime(bufferingTime),
      frequency(frequency),
      handle(handle),
      saiInterface(saiInterface),
      saiMode(saiMode)
  {
  }
};

/**
 * Defines the system configuration attributes of a Usb bus
 */
struct UsbConfiguration
{
public:
  uint32_t bufferingTime;       //!< Milliseconds of dma buffering
  uint32_t frequency;

  UsbConfiguration(uint32_t bufferingTime, uint32_t frequency) :
      bufferingTime(bufferingTime),
      frequency(frequency)
  {
  }
};

/**
 * This class manages the status and operational integrity of all the system peripherals.
 */
class SystemConfiguration: public GlobalServiceConsumer
{
private:
  DacConfiguration *dacConfig[2];
  ConsoleConfiguration *consoleConfig[2];
  GpioConfiguration *gpioConfig[23];
  AmpConfiguration *ampConfig[4];
  SaiConfiguration *saiConfig[3];
  MclkPllConfiguration *mclkConfig = nullptr;
  FilterConfiguration *filterConfig = nullptr;
  UsbConfiguration *usbConfig = nullptr;

  uint32_t peripheralAvailability;
  uint32_t audioBufferingTime;        //!< Audio buffering time in milliseconds
  AudioMode audioMode;
  uint32_t targetSpeakerSwitches = 0;

public:
  SystemConfiguration();

  void init();
  void updateConfigFromDipSwitches(uint32_t dipSwitchState);
  void updateFilterConfiguration();

  void* getHandleForSystemBus(SystemBus bus) const;

  bool isPeripheralAvailable(SystemPeripheral peripheral);
  bool isAudioSourceAvailable(SystemAudioSource audioSrc);
  bool isAudioSinkAvailable(SystemAudioSink audioSink);

  I2C_HandleTypeDef* getHandleForI2cInterface(I2cInterface interface) const;
  UART_HandleTypeDef* getHandleForUartInterface(UartInterface interface) const;
  HAL_StatusTypeDef configureSaiOutInterface(SaiInterface interface, PeripheralConfiguration configuration) const;

  DacConfiguration* getDacInterfaceConfiguration(DacInterface interface) const;
  ConsoleConfiguration* getConsoleInterfaceConfiguration(UartInterface interface) const;
  AmpConfiguration* getAmpInterfaceConfiguration(AmpInterface interface) const;
  GpioConfiguration* getGpioInterfaceConfiguration(GpioInterface interface) const;
  SaiConfiguration* getSaiInterfaceConfiguration(SaiInterface interface) const;
  MclkPllConfiguration* getMclkPllConfiguration() const;
  UsbConfiguration* getUsbConfiguration() const;

  FilterConfiguration* getFilterConfiguration() const;

  uint32_t getBufferingTIme() const
  {
    return audioBufferingTime;
  }

  /**
   * Returns the configured audio mode
   */
  AudioMode getAudioMode()
  {
    return audioMode;
  }

  uint32_t getTargetSpeakerSwitches()
  {
    return targetSpeakerSwitches;
  }
};

}
