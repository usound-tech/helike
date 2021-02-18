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
//  Description: The telemetry controller that handles remote commands
//  Filename: Telemetry.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 16-September-2020
//
//====================================================================

#pragma once

#include "Interfaces/Usb/src/Core/Inc/usbd_def.h"
#include <Controllers/System/pub/SystemController.hpp>
#include "main.h"
#include "OAL/pub/Oal.hpp"
#include <cstring>


namespace Controller
{

#define HID_CMD(X)          (HidCtlCmd)((X) & 0x0F)
#define HID_SUB_CMD(X)      (((X) >> 4) & 0x0F)

#define GET_TELEMETRY       0x01

#define TELEMETRY_REPLY_ERROR_FLAG         0xFF
#define TELEMETRY_REPLY_BUSY_FLAG          0xFE
#define TELEMETRY_REPLY_NO_RESULT_FLAG     0xFD
#define TELEMETRY_REPLY_UNKNOWN            0xFC

enum HidCtlCmd
{
  HID_FILTER_CMD,
  HID_BIST_CMD,
  HID_BSON_CMD
};

struct __attribute__((packed)) TelemetryCmd
{
  uint8_t res;
  uint8_t cmd;
  uint8_t data[30];
};

enum TelemetryBistCmdCode
{
  TELEMETRY_SET_GPIO,
  TELEMETRY_GET_GPIO,
  TELEMETRY_SET_REG,
  TELEMETRY_GET_REG,
  TELEMETRY_SET_I2C_REG,
  TELEMETRY_GET_I2C_REG,
  TELEMETRY_SET_SPI_REG,
  TELEMETRY_GET_SPI_REG,
  TELEMETRY_GET_STATUS,
  TELEMETRY_SET_GPIO_PORT,
  TELEMETRY_GET_GPIO_PORT
};

enum TelemetryFilterCmdCode
{
  TELEMETRY_INIT_BSON = 0x03,
  TELEMETRY_TX_BSON = 0x04,
  TELEMETRY_FIN_BSON = 0x05,
  TELEMETRY_FIN_AND_PERSIST_BSON = 0x06
};

/**
 * This class acts as the endpoint from all user remote commands (e.g. over USB, console, etc...)
 */
class Telemetry: public GlobalServiceConsumer
{
private:
  void *controlMessageQueue = nullptr;
  void *replyMessageQueue = nullptr;
  bool cmdInProgress = false;
  uint8_t cmdResults = 0;
  uint8_t *bsonBuffer = nullptr;
  uint32_t bsonChecksum = 0;
  uint32_t bsonSize = 0;

  uint8_t setGpio(TelemetryCmd &cmd);
  uint8_t getGpio(TelemetryCmd &cmd);
  uint8_t setRegister(TelemetryCmd &cmd);
  uint8_t getRegister(TelemetryCmd &cmd);
  uint8_t setI2cRegister(TelemetryCmd &cmd);
  uint8_t getI2cRegister(TelemetryCmd &cmd);
  uint8_t getPeripheralStatus(System::SystemPeripheral systemPeripheral, uint8_t okResponse);
  GPIO_TypeDef* getGpioPort(uint8_t port);
  uint8_t getBistStatus(TelemetryCmd &cmd);
  void halfMemcpy(volatile uint16_t *dst, const uint16_t *src, uint8_t len);

  uint8_t initBsonTx(TelemetryCmd &cmd);
  uint8_t handleBsonTx(TelemetryCmd &cmd);
  uint8_t finalizeBsonTx(TelemetryCmd &cmd, bool persistData);

  uint8_t cmdHandler(TelemetryCmd &cmd);
  uint8_t filterHandler(TelemetryCmd &cmd);

public:
  Telemetry(USBD_HandleTypeDef *handle);
  static void telemetryTask(void *argument);
  void init();
  void processCommands();
  void addCommand(uint8_t *cmdData);
  int8_t getReply(uint8_t *dataOut);

  uint8_t cmdResult()
  {
    return cmdResults;
  }
};

}
