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
//  Description: The console interface controller
//  Filename: ConsoleController.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "Controllers/Service/pub/Services.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "../src/Terminal.hpp"

namespace Controller
{

class Console: public GlobalServiceConsumer
{
private:
  const System::ConsoleConfiguration *consoleConfig;
  std::function<void(const char*)> printCb;
  std::function<void(char*, uint16_t*, uint32_t)> getsCb;
  System::SystemBus systemBus;
  Terminal terminal;
  System::Bus *uartBus;



public:
  static void consoleTask(void *argument);

  Console(const System::ConsoleConfiguration *consoleConfig, System::SystemBus systemBus);

  void init();
  void processRxData();
  void print(const char *data);
  void gets(char *data, uint16_t *readSize, uint32_t timeout);
};

}
