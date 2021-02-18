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
//  Filename: ConsoleController.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include <Controllers/System/pub/SystemController.hpp>
#include <Controllers/Console/pub/Console.hpp>
#include "Utilities/Logger/pub/Logger.h"
#include "main.h"
#include "OAL/pub/Oal.hpp"
#include "Terminal.hpp"
#include <cstring>

#if UART_CONSOLE_ENABLED == 1

namespace Controller
{

/**
 * Bounce function that allows rewiring of the callback to the class member.
 * @param argument the pointer to the console instance
 */
void Console::consoleTask(void *argument)
{
  Console *console = static_cast<Console*>(argument);
  console->processRxData();
}

/**
 * Console constructor
 * @param consoleConfig
 */
Console::Console(const System::ConsoleConfiguration *consoleConfig, System::SystemBus systemBus) :
    consoleConfig(consoleConfig),
    printCb(std::bind(&Console::print, this, std::placeholders::_1)),
    getsCb(std::bind(&Console::gets, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)),
    systemBus(systemBus),
    terminal(consoleConfig->hasHistory, printCb, getsCb),
    uartBus(nullptr)
{

}

/**
 * Initialises the console instance and starts the console thread
 */
void Console::init()
{
  terminal.init();

  System::SystemController *systemController = globalServices->getSystemController();
  uartBus = systemController->getBus(systemBus);

  auto oal = globalServices->getOal();
  oal->startTask((char*) "console_ctrl", 1024, System::OalTaskPriority::PRIO_MEDIUM, Console::consoleTask, (void*) this);
}

/**
 * This is the main task loop of the thread
 */
void Console::processRxData()
{
  auto oal = globalServices->getOal();
  oal->delay(1000);

  //TODO: Print console number and version
  char title_buffer[64];
  auto filterConfig = globalServices->getSystemConfiguration()->getFilterConfiguration();
  snprintf(title_buffer, sizeof(title_buffer), "\nHELIKE Terminal (v%d.%d.%d) - %s\n", MAJOR_REV, MINOR_REV, SUB_MINOR_REV, filterConfig->name.c_str());
  print(title_buffer);

#if CONFIG_POS == 1
  print("Fauna POS configuration");
#endif

  terminal.insertChar(KEY_DC2);

  while (1)
  {
    uint8_t nextChar;
    uint16_t readSize;

    readSize = 1;
    uartBus->read(0, 0, 0, &nextChar, &readSize, -1);
    if (readSize == 1)
    {
      terminal.insertChar(nextChar);
    }
  }
}

/**
 * Writes data to the uart assiciated with the console
 * @param data
 */
void Console::print(const char *data)
{
  uartBus->write(0, 0, 0, (uint8_t*) data, strlen(data), -1);
}

/**
 * Waits for data from the uart, for at most "timeout" millis.
 * It may return less bytes than requested, so check the value of readSize to
 * get the number of the retrieved bytes after the call.
 * @param data
 * @param readSize
 * @param timeout
 */
void Console::gets(char *data, uint16_t *readSize, uint32_t timeout)
{
  uartBus->read(0, 0, 0, (uint8_t*) data, readSize, timeout);
}

}

/**
 * This function redirects the printf output to UART
 *
 * @param file
 * @param ptr
 * @param len
 * @return
 */
extern "C" int _write(int file, char *ptr, int len)
{
//TODO: Reinstate code
#if 0
  static uint8_t _uartBug[256];

  // 0x01 flag corresponds to TX busy
  while (HAL_UART_GetState(&huart1) & 0x01)
  {
    HAL_Delay(10);
  }

  len = len > 256 ? 256 : len;
  memcpy(_uartBug, ptr, len);

  HAL_UART_Transmit(&huart1, _uartBug, len, 100);
  return len;
#endif
  return 0;
}
#endif

