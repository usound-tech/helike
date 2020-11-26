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
//  Description: Os abstraction layer - factory methods
//  Filename: Oal.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../pub/Oal.hpp"
#include "../pub/Hal.hpp"
#include "FreeRtosOal.hpp"
#include "FreeRtosHal.hpp"
#include "Interfaces/pub/SystemControl.hpp"

namespace System
{

/**
 * Returns an instance of the current Oal
 */
Oal* OalFactory::getOal()
{
  return new FreeRtosOal();
}

/**
 * Returns an instance of the FreeRtos I2C wrapper
 */
Bus* HalFactory::getI2c(void *handle, I2cInterface i2cInterface)
{
  FreeRtosI2c *bus = new FreeRtosI2c(handle, i2cInterface);
  bus->init();

  return bus;
}

/**
 * Returns an instance of the FreeRtos Uart wrapper
 */
Bus* HalFactory::getUart(void *handle, UartInterface uartInterface)
{
  FreeRtosUart *bus = new FreeRtosUart(handle, uartInterface);
  bus->init();

  return bus;
}

/**
 * Returns an instance of the FreeRtos Gpio wrapper
 */
Gpio* HalFactory::getGpio(GpioConfiguration *gpioConfig)
{
  FreeRtosGpio *gpio = new FreeRtosGpio(gpioConfig);
  gpio->init();

  return gpio;
}

/**
 * Returns an instance of the FreeRtos Gpio wrapper, with long-press capabilities
 */
Gpio* HalFactory::getLongPressGpio(GpioConfiguration *gpioConfig)
{
  FreeRtosGpioLongPress *gpio = new FreeRtosGpioLongPress(gpioConfig);
  gpio->init();

  return gpio;
}

/**
 * Returns an instance of the FreeRtos Sai wrapper
 */
Bus* HalFactory::getSaiOut(void *handle, SaiConfiguration *saiConfig)
{
  FreeRtosSaiOut *sai = new FreeRtosSaiOut(handle, saiConfig);
  sai->init();

  return sai;
}

/**
 * Returns an instance of the FreeRtos Sai wrapper
 */
Bus* HalFactory::getSaiIn(void *handle, SaiConfiguration *saiConfig)
{
  FreeRtosSaiIn *sai = new FreeRtosSaiIn(handle, saiConfig);
  sai->init();

  return sai;
}

/**
 * Returns an instance of the FreeRtos Usb wrapper
 */
Bus* HalFactory::getUsbIn(void *handle, UsbConfiguration *usbConfig)
{
  FreeRtosUsbIn *usb = new FreeRtosUsbIn(handle, usbConfig);
  usb->init();

  return usb;
}

}
