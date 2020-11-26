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
//  Filename: Telemetry.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "cmsis_os.h"
#include "Controllers/System/pub/SystemController.hpp"
#include "Controllers/System/pub/FilterReader.hpp"
#include "Controllers/Audio/pub/AudioService.hpp"
#include "Controllers/Filesystem/pub/Filesystem.hpp"
#include "../pub/Telemetry.hpp"
#include "main.h"
#include "OAL/pub/Oal.hpp"
#include <cstring>

#define DAC_TWEETER_STATUS    0x01
#define DAC_WOOFER_STATUS     0x02
#define WOOFER_AMP_R_STATUS   0x04
#define WOOFER_AMP_L_STATUS   0x08
#define MCLK_STATUS           0x10
#define SDCARD_STATUS         0x20

namespace Controller
{
/**
 * Bounce function that allows rewiring of the callback to the class member.
 * @param argument the pointer to the console instance
 */
void Telemetry::telemetryTask(void *argument)
{
  Telemetry *telemetry = static_cast<Telemetry*>(argument);
  telemetry->processCommands();
}

/**
 * Telemetry constructor
 */
Telemetry::Telemetry(USBD_HandleTypeDef *handle)
{
  handle->hid_priv = this;
}

/**
 * Initialises the console instance and starts the console thread
 */
void Telemetry::init()
{
  cmdInProgress = false;

  auto oal = globalServices->getOal();
  controlMessageQueue = oal->createMessageQueue(4, sizeof(TelemetryCmd));
  replyMessageQueue = oal->createMessageQueue(4, sizeof(TelemetryCmd));
  oal->startTask((char*) "telemetry_service", 1024, System::OalTaskPriority::PRIO_MEDIUM, Telemetry::telemetryTask, (void*) this);
}

/**
 * This is the main task loop of the thread
 */
void Telemetry::processCommands()
{
  auto oal = globalServices->getOal();
  TelemetryCmd cmd;

  while (1)
  {
    if (!oal->popMessageFromQueue(controlMessageQueue, &cmd, osWaitForever))
    {
      continue;
    }

    cmdInProgress = true;

    switch (HID_CMD(cmd.cmd))
    {
      case HID_FILTER_CMD:
        filterHandler(cmd);
        break;

      case HID_BIST_CMD:
        cmdResults = cmdHandler(cmd);
        break;

      case HID_BSON_CMD:
        break;
    }

    cmdInProgress = false;
  }
}

/**
 * Helper method to return the port object.
 * @param port
 * @return
 */
GPIO_TypeDef* Telemetry::getGpioPort(uint8_t port)
{
  switch (port)
  {
    case 0:
      return GPIOA;
    case 1:
      return GPIOB;
    case 2:
      return GPIOC;
    case 3:
      return GPIOD;
    case 4:
      return GPIOE;
    case 5:
      return GPIOF;
    case 6:
      return GPIOG;
    case 7:
      return GPIOH;
    case 8:
      return GPIOI;
    case 9:
      return GPIOJ;
    case 10:
      return GPIOK;
  }

  return NULL;
}

/**
 * Sets the value of a GPIO.
 * The command format is: [4b: port][4b: gpio][1b: value][2b: mode]
 *
 * @param cmd
 * @return
 */
uint8_t Telemetry::setGpio(TelemetryCmd &cmd)
{
  uint8_t port = cmd.data[0] & 0x0F;
  uint16_t gpio = 1 << ((cmd.data[0] >> 4) & 0x0F);
  GPIO_PinState value = (cmd.data[1] & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET;
  uint8_t mode = (cmd.data[1] >> 1) & 0x03;

  GPIO_TypeDef *port_base = getGpioPort(port);
  if (port_base == NULL)
  {
    return -1;
  }

  if (mode != 0)
  {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    GPIO_InitStruct.Pin = gpio;
    GPIO_InitStruct.Mode = (mode == 1) ? GPIO_MODE_OUTPUT_PP : GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(port_base, &GPIO_InitStruct);
  }

  HAL_GPIO_WritePin(port_base, gpio, value);

  return 0;
}

/**
 * Returns the value of a gpio line.
 * Command format: [4b: port][4b: gpio]
 * @param cmd
 * @return
 */
uint8_t Telemetry::getGpio(TelemetryCmd &cmd)
{
  uint8_t port = cmd.data[0] & 0x0F;
  uint16_t gpio = 1 << ((cmd.data[0] >> 4) & 0x0F);

  GPIO_TypeDef *port_base = getGpioPort(port);
  if (port_base == NULL)
  {
    return -1;
  }

  TelemetryCmd reply;
  memset(&reply.data, 0, 30);

  reply.res = 2;
  reply.cmd = GET_TELEMETRY;
  reply.data[3] = HAL_GPIO_ReadPin(port_base, gpio);

  globalServices->getOal()->sendMessageToQueue(replyMessageQueue, &reply, 0);

  return 0;
}

/**
 * Helper method to copy halfwords between two non-overlapping buffers
 * @param dst
 * @param src
 * @param len
 */
void Telemetry::halfMemcpy(volatile uint16_t *dst, const uint16_t *src, uint8_t len)
{
  len >>= 1;

  for (int i = 0; i < len; i++)
  {
    dst[i] = src[i];
  }
}

/**
 * Sets an MCU register to the provided value.
 * Command format: [32b: addr][32b: value][2b: mode]
 *
 * @param cmd
 * @return
 */
uint8_t Telemetry::setRegister(TelemetryCmd &cmd)
{
  volatile uint32_t *addr;
  uint32_t value;
  uint32_t mode = cmd.data[8] & 0x03;

  halfMemcpy((volatile uint16_t*) &addr, (uint16_t*) &cmd.data[0], 4);
  halfMemcpy((volatile uint16_t*) &value, (uint16_t*) &cmd.data[4], 4);

  switch (mode)
  {
    case 0:
      *addr = value;
      break;

    case 1:
      *addr |= value;
      break;

    case 2:
      *addr &= ~value;
      break;

    default:
      return -1;
  }

  return 0;
}

/**
 * Reads an MCU register value.
 * Command format: [32b: addr]
 * @param cmd
 * @return
 */
uint8_t Telemetry::getRegister(TelemetryCmd &cmd)
{
  volatile uint32_t *addr;
  uint32_t value;

  halfMemcpy((volatile uint16_t*) &addr, (uint16_t*) &cmd.data[0], 4);
  value = *addr;

  TelemetryCmd reply;
  memset(&reply.data, 0, 30);

  reply.res = 2;
  reply.cmd = GET_TELEMETRY;
  halfMemcpy((volatile uint16_t*) &reply.data[4], (uint16_t*) &value, 4);

  globalServices->getOal()->sendMessageToQueue(replyMessageQueue, &reply, 0);
  return 0;
}

/**
 * Sets an I2C register.
 * Command format: [8b: dev addr][8b: reg addr][8b: byte count][data...]
 *
 * @param cmd
 * @return
 */
uint8_t Telemetry::setI2cRegister(TelemetryCmd &cmd)
{
  uint8_t bus_id = cmd.data[0];
  uint32_t dev_addr = cmd.data[1];
  uint16_t reg_addr = cmd.data[2] | (((uint16_t) cmd.data[3]) << 8);
  uint16_t count = cmd.data[4] & 0x7F;
  uint16_t addrSize = ((cmd.data[4] & 0x80) == 0) ? I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT;

  System::SystemBus systemBus = bus_id == 0 ? System::SystemBus::I2C_TWEETER : System::SystemBus::I2C_WOOFER;
  auto bus = globalServices->getSystemController()->getBus(systemBus);

  if (bus->write(dev_addr, reg_addr, addrSize, &cmd.data[5], count, 100) != System::Status::STATUS_OK)
  {
    return -1;
  }

  return 0;
}

/**
 * Reads an I2C register.
 * Command format: [8b: dev addr][8b: reg addr][8b: byte count]
 *
 * @param cmd
 * @return
 */
uint8_t Telemetry::getI2cRegister(TelemetryCmd &cmd)
{
  uint8_t bus_id = cmd.data[0];
  uint32_t dev_addr = cmd.data[1];
  uint16_t reg_addr = cmd.data[2] | (((uint16_t) cmd.data[3]) << 8);
  uint16_t count = cmd.data[4] & 0x7F;
  uint16_t addrSize = ((cmd.data[4] & 0x80) == 0) ? I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT;
  uint8_t retVal = 0;

  TelemetryCmd reply;
  memset(&reply.data, 0, 30);

  reply.res = 2;
  reply.cmd = GET_TELEMETRY;

  System::SystemBus systemBus = bus_id == 0 ? System::SystemBus::I2C_TWEETER : System::SystemBus::I2C_WOOFER;
  auto bus = globalServices->getSystemController()->getBus(systemBus);

  if (bus->read(dev_addr, reg_addr, addrSize, &reply.data[4], &count, 100) != System::Status::STATUS_OK)
  {
    reply.cmd = TELEMETRY_REPLY_ERROR_FLAG;
    retVal = -1;
  }

  globalServices->getOal()->sendMessageToQueue(replyMessageQueue, &reply, 0);
  return retVal;
}

/**
 * Helper method to get the peripheral BIST status
 * @param systemPeripheral
 * @param okResponse
 * @return
 */
uint8_t Telemetry::getPeripheralStatus(System::SystemPeripheral systemPeripheral, uint8_t okResponse)
{
  System::Peripheral *peripheral = globalServices->getSystemController()->getPeripheral(systemPeripheral);
  if (peripheral)
  {
    System::State state = peripheral->getState();
    if (state == System::State::UNINITIALISED || state == System::State::ERROR)
    {
      return 0;
    }
    else
    {
      return okResponse;
    }
  }

  return 0;
}

/**
 * Returns the BIST result
 *
 * @param cmd
 * @return
 */
uint8_t Telemetry::getBistStatus(TelemetryCmd &cmd)
{
  uint8_t result = 0;

  result |= getPeripheralStatus(System::SystemPeripheral::DAC_TWEETER, DAC_TWEETER_STATUS);
  result |= getPeripheralStatus(System::SystemPeripheral::DAC_WOOFER, DAC_WOOFER_STATUS);
  result |= getPeripheralStatus(System::SystemPeripheral::WOOFER_AMP_R, WOOFER_AMP_R_STATUS);
  result |= getPeripheralStatus(System::SystemPeripheral::WOOFER_AMP_L, WOOFER_AMP_L_STATUS);
  result |= getPeripheralStatus(System::SystemPeripheral::MCLK_PLL, MCLK_STATUS);

  result |= (globalServices->getFilesystem()->isMounted() ? SDCARD_STATUS : 0);

  TelemetryCmd reply;
  memset(&reply.data, 0, 30);

  reply.res = 2;
  reply.cmd = GET_TELEMETRY;

  reply.data[4] = result;
  reply.data[5] = 0xAA;  // Helike device type
  reply.data[6] = MAJOR_REV;
  reply.data[7] = MINOR_REV;
  reply.data[8] = SUB_MINOR_REV;

  globalServices->getOal()->sendMessageToQueue(replyMessageQueue, &reply, 0);
  return 0;
}

uint8_t
Telemetry::cmdHandler(TelemetryCmd &cmd)
{
  switch (HID_SUB_CMD(cmd.cmd))
  {
    case TelemetryBistCmdCode::TELEMETRY_SET_GPIO:
      return setGpio(cmd);

    case TelemetryBistCmdCode::TELEMETRY_GET_GPIO:
      return getGpio(cmd);

      //[4b: port][4b: mode][16b: mask]
    case TelemetryBistCmdCode::TELEMETRY_SET_GPIO_PORT:
      //NOT SUPPORTED
      break;

    case TelemetryBistCmdCode::TELEMETRY_GET_GPIO_PORT:
      //NOT SUPPORTED
      break;

    case TelemetryBistCmdCode::TELEMETRY_SET_REG:
      return setRegister(cmd);

    case TelemetryBistCmdCode::TELEMETRY_GET_REG:
      return getRegister(cmd);

    case TelemetryBistCmdCode::TELEMETRY_SET_I2C_REG:
      return setI2cRegister(cmd);

    case TelemetryBistCmdCode::TELEMETRY_GET_I2C_REG:
      return getI2cRegister(cmd);

      //[4b: count tx][8b: count tx][data...]
    case TelemetryBistCmdCode::TELEMETRY_SET_SPI_REG:
      // NOT SUPPORTED
      return 0;

      //[4b: count tx][8b: count tx][data...]
    case TelemetryBistCmdCode::TELEMETRY_GET_SPI_REG:
      // NOT SUPPORTED
      return 0;

    case TelemetryBistCmdCode::TELEMETRY_GET_STATUS:
      return getBistStatus(cmd);
  }

  return 0;
}

/**
 * Initialises the bson buffer and prepares the reception of the filter config
 * @param cmd
 * @return
 */
uint8_t
Telemetry::initBsonTx(TelemetryCmd &cmd)
{
  if (bsonBuffer != nullptr)
  {
    delete[] bsonBuffer;
  }

  memcpy(&bsonSize, &cmd.data[0], sizeof(uint32_t));
  memcpy(&bsonChecksum, &cmd.data[4], sizeof(uint32_t));

  // Reserve a bit more space to allow multiples of 28 bytes to be stored in the buffer
  bsonBuffer = new uint8_t[bsonSize + 27];
  return 0;
}

/**
 * Stores the transferred chunk to the bson buffer, at the right offset
 * @param cmd
 * @return
 */
uint8_t
Telemetry::handleBsonTx(TelemetryCmd &cmd)
{
  if (bsonBuffer == nullptr)
  {
    return -1;
  }

  uint16_t offset;
  memcpy(&offset, &cmd.data[0], sizeof(uint16_t));
  memcpy(&bsonBuffer[offset * 28], &cmd.data[2], 28);

  return 0;
}

/**
 * Verifies the integrity of the transferred bson buffer and updates the filter configuration
 * @param cmd
 * @return
 */
uint8_t
Telemetry::finalizeBsonTx(TelemetryCmd &cmd)
{
  if (bsonBuffer == nullptr)
  {
    return -1;
  }

  uint32_t checksum = 0;
  for (uint32_t i = 0; i < bsonSize; i++)
  {
    checksum += bsonBuffer[i];
  }

  TelemetryCmd reply;
  memset(&reply.data, 0, 30);

  reply.res = 2;
  reply.cmd = GET_TELEMETRY;

  if (checksum != bsonChecksum)
  {
    reply.data[2] = TELEMETRY_REPLY_ERROR_FLAG;
    globalServices->getOal()->sendMessageToQueue(replyMessageQueue, &reply, 0);
    return -1;
  }

  globalServices->getOal()->sendMessageToQueue(replyMessageQueue, &reply, 0);

  auto filterConfig = globalServices->getSystemConfiguration()->getFilterConfiguration();
  System::FilterReader filterReader(filterConfig);
  filterReader.extractConfig(bsonBuffer);

  globalServices->getAudioService()->reconfigureFilters();

  delete[] bsonBuffer;
  bsonBuffer = nullptr;

  return 0;
}

/**
 * Processes the filter upda
 * @param cmd
 * @return
 */
uint8_t
Telemetry::filterHandler(TelemetryCmd &cmd)
{
  switch (HID_SUB_CMD(cmd.cmd))
  {
    case TelemetryFilterCmdCode::TELEMETRY_INIT_BSON:
      return initBsonTx(cmd);

    case TelemetryFilterCmdCode::TELEMETRY_TX_BSON:
      return handleBsonTx(cmd);

    case TelemetryFilterCmdCode::TELEMETRY_FIN_BSON:
      return finalizeBsonTx(cmd);

    default:
      return -1;
  }

  return 0;
}


/**
 * Enqueues a new Telemetry command
 * @param cmdData
 */
void Telemetry::addCommand(uint8_t *cmdData)
{
  globalServices->getOal()->sendMessageToQueue(controlMessageQueue, cmdData, 0);
}

/**
 * @brief Manage the CUSTOM HID class In Event
 */
int8_t
Telemetry::getReply(uint8_t *dataOut)
{
  auto oal = globalServices->getOal();
  TelemetryCmd reply;

  if (!oal->popMessageFromQueueFromISR(replyMessageQueue, &reply, 0))
  {
    dataOut[0] = 2;

    if (cmdInProgress)
    {
      dataOut[4] = TELEMETRY_REPLY_BUSY_FLAG;
    }
    else
    {
      dataOut[4] = TELEMETRY_REPLY_NO_RESULT_FLAG;
    }

    return 0;
  }

  switch (reply.cmd)
  {
    case GET_TELEMETRY:
      memcpy(dataOut, &reply, sizeof(TelemetryCmd));
      break;

    default:
      case 0xFF:
      memset(&dataOut[0], 0, 12);
      dataOut[0] = 2;
      dataOut[4] = TELEMETRY_REPLY_UNKNOWN;
  }

  return 0;
}

}
