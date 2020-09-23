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


namespace System
{

/**
 * Defines the peripheral configuration options
 */
enum PeripheralConfiguration
{
  PERIPH_ENABLE,
  PERIPH_DISABLE
};

/**
 * Defines the type of SAI interface
 */
enum SaiInterface
{
  TWEETER,
  WOOFER,
  IN
};

/**
 * Defines the type of Uart interface
 */
enum UartInterface
{
  UART_DEBUG,
  UART_CTRL
};

/**
 * Defines the type of Dac interface
 */
enum DacInterface
{
  DAC_TWEETER_IFACE,
  DAC_WOOFER_IFACE
};

/**
 * Defines the type of Dac interface
 */
enum AmpInterface
{
  AMP_TWEETER_R,
  AMP_TWEETER_L,
  AMP_WOOFER_R,
  AMP_WOOFER_L,
};

/**
 * Defines the type of the Gpio interface
 */
enum GpioInterface
{
  NO_GPIO = -1,
  GPIO_DAC_TWEETER = 0,
  GPIO_DAC_WOOFER,
  GPIO_AMP_TWEETER_R,
  GPIO_AMP_TWEETER_L,
  GPIO_AMP_WOOFER_R,
  GPIO_AMP_WOOFER_L,
  GPIO_LED_1,
  GPIO_LED_2,
  GPIO_LED_3,
  GPIO_LED_4,
  GPIO_JOYSTICK_NORTH,
  GPIO_JOYSTICK_SOUTH,
  GPIO_JOYSTICK_WEST,
  GPIO_JOYSTICK_EAST,
  GPIO_JOYSTICK_CENTER,
  GPIO_DIPSWITCH_1,
  GPIO_DIPSWITCH_2,
  GPIO_DIPSWITCH_3,
  GPIO_DIPSWITCH_4,
  GPIO_DIPSWITCH_CFG_1,
  GPIO_DIPSWITCH_CFG_2,
  GPIO_DIPSWITCH_CFG_3,
  GPIO_DIPSWITCH_CFG_4,
  GPIO_COUNT
};

/**
 * Defines the type of I2C interface
 */
enum I2cInterface
{
  I2C_TWEETER_DAC,
  I2C_WOOFER_DAC,
  I2C_WOOFER_AMP_R,
  I2C_WOOFER_AMP_L,
  I2C_MCLK_PLL
};

/**
 * Defines the audio configuration
 */
enum AudioMode
{
  AM_MP3,
  AM_I2S_SLAVE,
  AM_USB
};

}
