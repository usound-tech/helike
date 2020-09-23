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
//  Filename: SystemConfiguration.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../pub/SystemConfiguration.hpp"
#include "Interfaces/pub/SystemControl.hpp"
#include "filters/FilterReader.hpp"

#include "sai.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

namespace System
{
#define NO_EXTI_PIN 0

#if CONFIG_POS == 0
extern FilterConfiguration passthroughConfiguration;
#else
extern FilterConfiguration audioEyewearConfiguration;
#endif

/**
 * System configuration constructor
 */
SystemConfiguration::SystemConfiguration() :
    peripheralAvailability(0),
    audioBufferingTime(0),
    audioMode(AudioMode::AM_MP3)
{

}

/**
 * Reads the configuration from storage.
 */
void SystemConfiguration::init()
{
//TODO: Read the boot system config to know which peripherals to bring up

  peripheralAvailability = (1 << SystemPeripheral::DAC_TWEETER)
      | (1 << SystemPeripheral::DAC_WOOFER)
      | (1 << SystemPeripheral::TWEETER_AMP_R)
      | (1 << SystemPeripheral::WOOFER_AMP_R)
      | (1 << SystemPeripheral::WOOFER_AMP_L)
      | (1 << (SystemAudioSource::AUDIO_SRC_GENERATOR + 16))
      | (1 << (SystemAudioSink::AUDIO_SINK_LOCAL + 24));

  audioBufferingTime = 4; // 4ms buffering time

  dacConfig[DacInterface::DAC_TWEETER_IFACE] = new DacConfiguration(0x20, SystemBus::I2C_TWEETER, GpioInterface::GPIO_DAC_TWEETER);
  dacConfig[DacInterface::DAC_WOOFER_IFACE] = new DacConfiguration(0x22, SystemBus::I2C_WOOFER, GpioInterface::GPIO_DAC_WOOFER);

  consoleConfig[0] = new ConsoleConfiguration(true, UartInterface::UART_CTRL);
  consoleConfig[1] = new ConsoleConfiguration(false, UartInterface::UART_DEBUG);

  ampConfig[AmpInterface::AMP_TWEETER_R] = new AmpConfiguration(0, SystemBus::NO_BUS, GpioInterface::GPIO_AMP_TWEETER_R);
  ampConfig[AmpInterface::AMP_TWEETER_L] = nullptr;
  ampConfig[AmpInterface::AMP_WOOFER_R] = new AmpConfiguration(0xB0, SystemBus::I2C_TWEETER, GpioInterface::GPIO_AMP_WOOFER_R);
  ampConfig[AmpInterface::AMP_WOOFER_L] = new AmpConfiguration(0xB0, SystemBus::I2C_WOOFER, GpioInterface::GPIO_AMP_WOOFER_L);

  mclkConfig = new MclkPllConfiguration(0x9C, SystemBus::I2C_TWEETER);

  gpioConfig[GpioInterface::GPIO_DAC_TWEETER] = new GpioConfiguration(nDAC1_Powerdown_GPIO_Port, nDAC1_Powerdown_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_DAC_WOOFER] = new GpioConfiguration(nDAC2_Powerdown_GPIO_Port, nDAC2_Powerdown_Pin, NO_EXTI_PIN, false);

  gpioConfig[GpioInterface::GPIO_AMP_TWEETER_R] = new GpioConfiguration(nAMP1_Powerdown_GPIO_Port, nAMP1_Powerdown_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_AMP_TWEETER_L] = nullptr;
  gpioConfig[GpioInterface::GPIO_AMP_WOOFER_R] = new GpioConfiguration(nAMP2_Powerdown_GPIO_Port, nAMP2_Powerdown_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_AMP_WOOFER_L] = nullptr;

  gpioConfig[GpioInterface::GPIO_LED_1] = new GpioConfiguration(LED1_GPIO_Port, LED1_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_LED_2] = new GpioConfiguration(LED2_GPIO_Port, LED2_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_LED_3] = new GpioConfiguration(LED3_GPIO_Port, LED3_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_LED_4] = new GpioConfiguration(LED4_GPIO_Port, LED4_Pin, NO_EXTI_PIN, false);

  gpioConfig[GpioInterface::GPIO_JOYSTICK_NORTH] = new GpioConfiguration(JOYSTICK_NORTH_GPIO_Port, JOYSTICK_NORTH_Pin, GPIO_PIN_7, true);
  gpioConfig[GpioInterface::GPIO_JOYSTICK_SOUTH] = new GpioConfiguration(JOYSTICK_SOUTH_GPIO_Port, JOYSTICK_SOUTH_Pin, GPIO_PIN_8, true);
  gpioConfig[GpioInterface::GPIO_JOYSTICK_WEST] = new GpioConfiguration(JOYSTICK_WEST_GPIO_Port, JOYSTICK_WEST_Pin, GPIO_PIN_9, false);
  gpioConfig[GpioInterface::GPIO_JOYSTICK_EAST] = new GpioConfiguration(JOYSTICK_EAST_GPIO_Port, JOYSTICK_EAST_Pin, GPIO_PIN_10, false);
  gpioConfig[GpioInterface::GPIO_JOYSTICK_CENTER] = new GpioConfiguration(JOYSTICK_CENTER_GPIO_Port, JOYSTICK_CENTER_Pin, GPIO_PIN_11, false);

  gpioConfig[GpioInterface::GPIO_DIPSWITCH_1] = new GpioConfiguration(DIPSWITCH1_GPIO_Port, DIPSWITCH1_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_DIPSWITCH_2] = new GpioConfiguration(DIPSWITCH2_GPIO_Port, DIPSWITCH2_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_DIPSWITCH_3] = new GpioConfiguration(DIPSWITCH3_GPIO_Port, DIPSWITCH3_Pin, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_DIPSWITCH_4] = new GpioConfiguration(DIPSWITCH4_GPIO_Port, DIPSWITCH4_Pin, NO_EXTI_PIN, false);

  gpioConfig[GpioInterface::GPIO_DIPSWITCH_CFG_1] = new GpioConfiguration(GPIOD, GPIO_PIN_14, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_DIPSWITCH_CFG_2] = new GpioConfiguration(GPIOC, GPIO_PIN_11, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_DIPSWITCH_CFG_3] = new GpioConfiguration(GPIOC, GPIO_PIN_3, NO_EXTI_PIN, false);
  gpioConfig[GpioInterface::GPIO_DIPSWITCH_CFG_4] = new GpioConfiguration(GPIOC, GPIO_PIN_2, NO_EXTI_PIN, false);
}

/**
 * Returns true if the requested peripheral is available in the configuration
 * @param peripheral
 * @return
 */
bool SystemConfiguration::isPeripheralAvailable(SystemPeripheral peripheral)
{
  return !!(peripheralAvailability & (1 << peripheral));
}

/**
 * Returns true if the requested audio source peripheral is available in the configuration
 * @param peripheral
 * @return
 */
bool SystemConfiguration::isAudioSourceAvailable(SystemAudioSource audioSrc)
{
  return !!(peripheralAvailability & (1 << (audioSrc + 16)));
}

/**
 * Returns true if the requested audio sink peripheral is available in the configuration
 * @param peripheral
 * @return
 */
bool SystemConfiguration::isAudioSinkAvailable(SystemAudioSink audioSink)
{
  return !!(peripheralAvailability & (1 << (audioSink + 24)));
}

/**
 * Returns the SAI interface associated with the requested path
 *
 * @param interface selects the sai interface to configure
 * @param configuration selects the SAI configuration
 * @return
 */
HAL_StatusTypeDef SystemConfiguration::configureSaiOutInterface(SaiInterface interface, PeripheralConfiguration configuration) const
{
  /*
   if (interface == SaiInterface::TWEETER)
   {
   return (configuration == PeripheralConfiguration::PERIPH_ENABLE) ? MX_SAI3_InitBlockA() : MX_SAI3_DeinitBlockA();
   }
   else
   {
   return (configuration == PeripheralConfiguration::PERIPH_ENABLE) ? MX_SAI3_InitBlockB() : MX_SAI3_DeinitBlockB();
   }
   */
  return HAL_OK;
}

/**
 * Returns the Uart interface associated with the requested path
 * @param interface
 * @return
 */
UART_HandleTypeDef* SystemConfiguration::getHandleForUartInterface(UartInterface interface) const
{
  return (interface == UartInterface::UART_CTRL) ? &huart5 : &huart2;
}

/**
 * Returns the I2C interface associated with the requested path
 * @param interface
 * @return
 */
I2C_HandleTypeDef* SystemConfiguration::getHandleForI2cInterface(I2cInterface interface) const
{
  switch (interface)
  {
    case I2cInterface::I2C_WOOFER_DAC:
      case I2cInterface::I2C_WOOFER_AMP_L:
      return &hi2c4;

    case I2cInterface::I2C_TWEETER_DAC:
      case I2cInterface::I2C_WOOFER_AMP_R:
      return &hi2c2;

    case I2cInterface::I2C_MCLK_PLL:
      return &hi2c2;

    default:
      return nullptr;
  }
}

/**
 * Returns the bus interface associated with the requested bus
 * @param bus
 * @return
 */
void* SystemConfiguration::getHandleForSystemBus(SystemBus bus) const
{
  switch (bus)
  {
    case SystemBus::I2C_TWEETER:
      return &hi2c2;

    case SystemBus::I2C_WOOFER:
      return &hi2c4;

    case SystemBus::CTRL_UART:
      return &huart5;

    case SystemBus::DEBUG_UART:
      return &huart2;

    case SystemBus::SAI_TWEETER:
      return &hsai_BlockA3;

    case SystemBus::SAI_WOOFER:
      return &hsai_BlockB3;

    case SystemBus::SAI_IN:
      return (peripheralAvailability & (1 << (SystemAudioSource::AUDIO_SRC_SAI + 16))) ? &hsai_BlockA2 : nullptr;

    default:
      return nullptr;
  }
}

/**
 * Returns the DAC interface configuration
 * @param interface
 * @return
 */
DacConfiguration* SystemConfiguration::getDacInterfaceConfiguration(DacInterface interface) const
{
  return dacConfig[interface];
}

/**
 * Returns the console interface configuration
 * @param interface
 * @return
 */
ConsoleConfiguration* SystemConfiguration::getConsoleInterfaceConfiguration(UartInterface interface) const
{
  for (int i = 0; i < 2; i++)
  {
    if (consoleConfig[i]->uartInterface == interface)
    {
      return consoleConfig[i];
    }
  }

  return nullptr;
}

/**
 * Returns the gpio interface configuration
 * @param interface
 * @return
 */
GpioConfiguration* SystemConfiguration::getGpioInterfaceConfiguration(GpioInterface interface) const
{
  return (interface == GpioInterface::NO_GPIO) ? nullptr : gpioConfig[interface];
}

/**
 * Returns the AMP configuration
 * @param interface
 * @return
 */
AmpConfiguration* SystemConfiguration::getAmpInterfaceConfiguration(AmpInterface interface) const
{
  return ampConfig[interface];
}

/**
 * Returns the I2S MCLK PLL configuration (for the I2S slave mode)
 */
MclkPllConfiguration* SystemConfiguration::getMclkPllConfiguration() const
{
  return mclkConfig;
}

/**
 * Returns the SAI configuration.
 * @param interface
 * @return
 */
SaiConfiguration* SystemConfiguration::getSaiInterfaceConfiguration(SaiInterface interface) const
{
  return saiConfig[interface];
}

/**
 * Returns the DRC block configuration
 */
FilterConfiguration* SystemConfiguration::getFilterConfiguration() const
{
  return filterConfig;
}

/**
 * Adjusts the configuration according to the provided dip switch state
 */
void SystemConfiguration::updateConfigFromDipSwitches(uint32_t dipSwitchState)
{
  uint32_t audioModeSwitches = dipSwitchState & ((1 << GpioInterface::GPIO_DIPSWITCH_CFG_4) | (1 << GpioInterface::GPIO_DIPSWITCH_CFG_3));
  switch (audioModeSwitches)
  {
    case 0:
      peripheralAvailability |= (1 << (SystemAudioSource::AUDIO_SRC_FILE + 16));
      audioMode = AudioMode::AM_MP3;

      saiConfig[SaiInterface::TWEETER] = new SaiConfiguration(audioBufferingTime, 48000, &hsai_BlockA3, SaiInterface::TWEETER,
          SaiMode::SAI_MODE_TX_MASTER);
      saiConfig[SaiInterface::WOOFER] = new SaiConfiguration(audioBufferingTime, 48000, &hsai_BlockB3, SaiInterface::WOOFER,
          SaiMode::SAI_MODE_TX_SLAVE_INTERNAL);
      saiConfig[SaiInterface::IN] = nullptr;
      break;

    case (1 << GpioInterface::GPIO_DIPSWITCH_CFG_4):
      peripheralAvailability |= (1 << (SystemAudioSource::AUDIO_SRC_SAI + 16))
          | (1 << SystemPeripheral::MCLK_PLL);

      audioMode = AudioMode::AM_I2S_SLAVE;

      saiConfig[SaiInterface::TWEETER] = new SaiConfiguration(audioBufferingTime, 48000, &hsai_BlockA3, SaiInterface::TWEETER,
          SaiMode::SAI_MODE_TX_SLAVE_EXTERNAL);
      saiConfig[SaiInterface::WOOFER] = new SaiConfiguration(audioBufferingTime, 48000, &hsai_BlockB3, SaiInterface::WOOFER,
          SaiMode::SAI_MODE_TX_SLAVE_INTERNAL);
      saiConfig[SaiInterface::IN] = new SaiConfiguration(2, 48000, &hsai_BlockA2, SaiInterface::IN, SaiMode::SAI_MODE_RX_SLAVE_EXTERNAL);
      break;

    default:
      case (1 << GpioInterface::GPIO_DIPSWITCH_CFG_3):
      audioMode = AudioMode::AM_USB;
      break;
  }

  targetSpeakerSwitches = 0;

#if CONFIG_POS == 1
  filterConfig = &audioEyewearConfiguration;
#else
  filterConfig = &passthroughConfiguration;

  targetSpeakerSwitches |= ((dipSwitchState & (1 << GpioInterface::GPIO_DIPSWITCH_1)) ? 0x08 : 0);
  targetSpeakerSwitches |= ((dipSwitchState & (1 << GpioInterface::GPIO_DIPSWITCH_2)) ? 0x04 : 0);
  targetSpeakerSwitches |= ((dipSwitchState & (1 << GpioInterface::GPIO_DIPSWITCH_3)) ? 0x02 : 0);
  targetSpeakerSwitches |= ((dipSwitchState & (1 << GpioInterface::GPIO_DIPSWITCH_4)) ? 0x01 : 0);
#endif
}

void SystemConfiguration::updateFilterConfiguration()
{
#if CONFIG_POS == 0
  if (targetSpeakerSwitches)
  {
    FilterReader filterReader(filterConfig);
    filterReader.configFilter(targetSpeakerSwitches);
  }
#endif
}
}
