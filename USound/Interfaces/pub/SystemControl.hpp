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
//  Description: System control defines the global types for controlling the various
//               peripherals.
//  Filename: SystemControl.h
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================


#pragma once
#include "Utilities/Fifo/pub/Fifo.hpp"
#include <exception>
#include <string>
#include <functional>

namespace System
{

/**
 * The list of the system peripherals
 */
enum SystemPeripheral
{
  BOOT_PINS,        //!< The boot configuration dip switches
  LEDS,             //!< The boot configuration dip switches
  SD_CARD,          //!< The sd card interface
  DAC_TWEETER,      //!< DAC_TWEETER
  DAC_WOOFER,       //!< DAC_WOOFER
  WOOFER_AMP_R,     //!< DYNAMIC_AMP_1
  WOOFER_AMP_L,     //!< DYNAMIC_AMP_2
  TWEETER_AMP_R,    //!< MEMS_AMP_1
  TWEETER_AMP_L,    //!< MEMS_AMP_2
  MCLK_PLL,         //!< The MCLK PLL
  PERIPHERAL_COUNT  //!< The number of the registered peripherals
};

/**
 * The list of the system buses that connect the peripherals to the MCU
 */
enum SystemBus
{
  NO_BUS = -1,      //!< No bus is allocated for the peripheral
  I2C_TWEETER = 0,  //!< The i2c interface that is connected to the tweeter dac
  I2C_WOOFER,       //!< The i2c interface that is connected to the woofer dac
  DEBUG_UART,       //!< The uart interface that is not connected to the host
  CTRL_UART,        //!< The uart interface that is connected to the host platform
  USB,              //!< The USB interface
  SAI_TWEETER,      //!< The tweeter SAI Out bus
  SAI_WOOFER,       //!< The woofer SAI Out bus
  SAI_IN,           //!< The SAI In bus
  BUS_COUNT         //!< The number of the registered buses
};

/**
 * The list of the system buses that connect the peripherals to the MCU
 */
enum SystemConsole
{
  CONSOLE_CTRL,
  CONSOLE_DEBUG,
  CONSOLE_COUNT         //!< The max number of consoles
};

/**
 * The list of the system audio source peripherals
 */
enum SystemAudioSource
{
  AUDIO_SRC_FILE,          //!< The audio file player/decoder
  AUDIO_SRC_GENERATOR,
  AUDIO_SRC_SAI,          //!< Slave I2S input
  AUDIO_SRC_COUNT,        //!< The max number of audio sources
  AUDIO_SRC_NONE = -1     //!< No interface selected
};

/**
 * The list of the system audio sink peripherals
 */
enum SystemAudioSink
{
  AUDIO_SINK_LOCAL,        //!< The local audio output (SAI + DAC + AMPs)
  AUDIO_SINK_COUNT,        //!< The max number of audio sinks
  AUDIO_SINK_NONE = -1     //!< No interface selected
};

/**
 * The peripheral operations.
 */
enum Action
{
  ENABLE,  //!< Enables the peripheral
  DISABLE, //!< Disables the peripheral
  START,   //!< Makes the peripheral active (only some peripherals support this)
  STOP,    //!< Makes the peripheral inactive (only some peripherals support this)
  RESET,   //!< Resets the peripheral configuration and sets the state to enabled
};

/**
 * The peripheral current status.
 */
enum State
{
  UNINITIALISED,  //!< Peripheral is not initalised yet
  ENABLED,  //!< Peripheral is configured and idle
  DISABLED, //!< Peripheral is powered down
  ACTIVE,   //!< Peripheral is running
  PAUSED,   //!< Peripheral has been put on hold
  ERROR,    //!< Peripheral is in error state
};

/**
 * The return type of the HAL operations.
 */
enum Status
{
  STATUS_OK = 0x00, /**< STATUS_OK */
  STATUS_ERROR = 0x01, /**< STATUS_ERROR */
  STATUS_BUSY = 0x02, /**< STATUS_BUSY */
  STATUS_TIMEOUT = 0x03/**< STATUS_TIMEOUT */
};

/**
 * The type of the GPIO event.
 */
enum GpioEventType
{
  GEVT_SINGLE,  //!< Single click event
  GEVT_SOFT     //!< Soft click event

};

const char* toString(State state);

/**
 * This interface defines the common operations shared across all the system peripherals
 */
class Peripheral
{
protected:
  State state;

public:
  virtual void init() = 0;
  virtual void deinit() = 0;
  virtual void doAction(System::Action action) = 0;

  State getState()
  {
    return state;
  }

  virtual ~Peripheral()
  {
  }
};

/**
 * This interface defines the common operations shared across all the system buses
 */
class Bus
{
public:

  virtual Status read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout) = 0;
  virtual Status write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout) = 0;

  virtual Status enable() = 0;
  virtual Status disable() = 0;

  virtual void init() = 0;

  virtual ~Bus()
  {
  }
};

/**
 * This interface defines the common operations shared across all the system gpios
 */
class Gpio
{
public:

  virtual void set(uint32_t value) = 0;
  virtual uint32_t get() = 0;
  virtual uint16_t getExtEvent() = 0;
  virtual void runEvent() = 0;

  virtual void init() = 0;
  virtual void setEventHandler(std::function<void(uint32_t level, GpioEventType evType)> handler) = 0;

  virtual ~Gpio()
  {
  }
};

/**
 * This exception type is thrown from the peripherals
 */
class PeripheralException: public std::exception
{
protected:
  std::string exceptionMsg;

public:
  PeripheralException(std::string &&exceptionStr) :
      exceptionMsg(std::move(exceptionStr))
  {
  }

  const char* what() const throw ()
  {
    return exceptionMsg.c_str();
  }
};

/**
 * This abstract class defines an audio sink interface
 * @tparam T
 */
template<typename T>
class AudioSink: public Peripheral
{
public:
  //TODO: The interface shouldn't be here. Change this function so that the AudioSink serves only one interface
  virtual void enqueueData(T *data, uint32_t length, uint32_t interface) = 0;
  virtual uint32_t getFrequency() = 0;
  virtual void mute(bool enable) = 0;
  virtual void setVolume(uint8_t vol) = 0;

  virtual ~AudioSink()
  {
  }
};

/**
 * This abstract class defines an audio source interface
 * @tparam T
 */
template<typename T>
class AudioSource: public Peripheral
{
public:
  virtual T* getData(uint32_t length) = 0;
  virtual void consumedData(uint32_t length) = 0;
  virtual uint32_t getFrequency() = 0;
  virtual void notifyDataAvailable() = 0;

  virtual bool skipNext() = 0;
  virtual bool skipPrev() = 0;

  virtual ~AudioSource()
  {
  }
};

}
