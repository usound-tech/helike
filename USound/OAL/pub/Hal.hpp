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
//  Description: Hardware abstraction layer
//  Filename: Hal.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include <stdint.h>
#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemInterfaces.hpp"
#include "Interfaces/pub/SystemControl.hpp"

namespace System
{
struct GpioConfiguration;
struct SaiConfiguration;

/**
 * Helper factory class to return the Hal instance
 */
class HalFactory: public GlobalServiceConsumer
{
public:
  Bus* getI2c(void *handle, I2cInterface i2cInterface);
  Bus* getUart(void *handle, UartInterface uartInterface);
  Bus* getSaiOut(void *handle, SaiConfiguration *saiConfig);
  Bus* getSaiIn(void *handle, SaiConfiguration *saiConfig);
  Gpio* getGpio(GpioConfiguration *gpioConfig);
  Gpio* getLongPressGpio(GpioConfiguration *gpioConfig);
};

}
