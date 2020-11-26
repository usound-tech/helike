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
//  Description: Hardware abstraction layer - FreeRtos port
//  Filename: FreeRtosHal.hpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#pragma once

#include "../pub/Hal.hpp"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "stm32h7xx_hal.h"
#include "Controllers/Service/pub/Services.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "Interfaces/Usb/src/Core/Inc/usbd_def.h"
#include <functional>

namespace System
{

/**
 * This class wraps the FreeRtos HAL I2C operations.
 */
class FreeRtosI2c: public Bus, public GlobalServiceConsumer
{
private:
  I2C_HandleTypeDef *handle;
  StaticSemaphore_t mutexBuffer;
  SemaphoreHandle_t mutexHandle;
  volatile uint8_t *i2cDmaBuffer;
  I2cInterface i2cInterface;
  volatile TaskHandle_t taskHandle;

public:
  FreeRtosI2c(void *handle, I2cInterface i2cInterface);

  void init() override;
  Status read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout) override;
  Status write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout) override;

  Status enable() override;
  Status disable() override;

  void dmaDone();
};


/**
 * This class wraps the FreeRtos HAL Uart operations.
 */
class FreeRtosUart: public Bus, public GlobalServiceConsumer
{
private:
  UART_HandleTypeDef *handle;

  StaticSemaphore_t txMutexBuffer;
  SemaphoreHandle_t txMutexHandle;
  StaticSemaphore_t rxMutexBuffer;
  SemaphoreHandle_t rxMutexHandle;

  volatile uint8_t *uartRxBuffer;
  volatile uint8_t *uartTxBuffer;
  UartInterface uartInterface;
  volatile TaskHandle_t rxTaskHandle;
  volatile TaskHandle_t txTaskHandle;
  Fifo<uint8_t> rxDataFifo;

public:
  FreeRtosUart(void *handle, UartInterface uartInterface);

  void init() override;
  Status read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout) override;
  Status write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout) override;

  Status enable() override;
  Status disable() override;

  void notifyDataReceived(uint16_t length);
  void notifyDataTransmitted();
};

/**
 * This interface defines the common operations shared across all the system gpios
 */
class FreeRtosGpio: public Gpio, public GlobalServiceConsumer
{
protected:
  GpioConfiguration *gpioConfig;
  std::function<void(uint32_t level, GpioEventType evType)> eventHandler;

public:
  static void triggerInputEvent(uint16_t eventId);

  FreeRtosGpio(GpioConfiguration *gpioConfig);

  void set(uint32_t value) override;
  uint32_t get() override;
  uint16_t getExtEvent() override;
  void setEventHandler(std::function<void(uint32_t, GpioEventType)> handler) override;
  void runEvent() override;
  void init() override;
};

enum GpioLPState
{
  UNKNOWN,
  UP,
  UP_DEBOUNCE,
  DOWN,
  DOWN_DEBOUNCE
};

/**
 * This interface defines the common operations shared across all the system gpios, with added long-press repeat capabilities
 */
class FreeRtosGpioLongPress: public FreeRtosGpio
{
private:
  osTimerId_t timer = nullptr;
  GpioLPState state = GpioLPState::UNKNOWN;

private:
  static void timeoutEventCb(void *arg);
  void timeoutEvent();

public:
  FreeRtosGpioLongPress(GpioConfiguration *gpioConfig);

  void runEvent() override;
  void init() override;
};

/**
 * Defines the dma type
 */
enum SaiDmaType
{
  HALF,   //!< Occurs when the DMA half done event is emitted
  FULL    //!< Occurs when the DMA full done event is emitted
};


/**
 * This class wraps the FreeRtos HAL Sai Out operations.
 */
class FreeRtosSaiOut: public Bus, public GlobalServiceConsumer
{
private:
  PpFifo<uint16_t> saiOutFifo;
  SAI_HandleTypeDef *handle;
  SaiConfiguration *saiConfiguration;

  uint8_t *saiDmaBuffer;        //!< The DMA buffer to be sent out over the SAI interface
  uint32_t dmaBufferLen;
  bool isDmaHalf = false;

public:
  FreeRtosSaiOut(void *handle, SaiConfiguration *saiConfiguration);

  void init() override;
  Status read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout) override;
  Status write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout) override;

  Status enable() override;
  Status disable() override;

  void dmaDone(SaiDmaType dmaType);
};

/**
 * This class wraps the FreeRtos HAL Sai In operations.
 */
class FreeRtosSaiIn: public Bus, public GlobalServiceConsumer
{
private:
  PpFifo<uint16_t> saiInFifo;
  SAI_HandleTypeDef *handle;
  SaiConfiguration *saiConfiguration;

  uint8_t *saiDmaBuffer;        //!< The DMA buffer to be sent out over the SAI interface
  uint32_t dmaBufferLen;
  bool isDmaHalf = false;

public:
  FreeRtosSaiIn(void *handle, SaiConfiguration *saiConfiguration);

  void init() override;
  Status read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout) override;
  Status write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout) override;

  Status enable() override;
  Status disable() override;

  void dmaDone(SaiDmaType dmaType);
};

/**
 * This class wraps the FreeRtos HAL Usb In operations.
 */
class FreeRtosUsbIn: public Bus, public GlobalServiceConsumer
{
private:
  Fifo<int16_t> usbInFifo;
  UsbConfiguration *usbConfiguration;
  int16_t *usbBuffer = nullptr;
  USBD_HandleTypeDef *handle;
  uint32_t readCyclesCompleted = 0;

public:
  FreeRtosUsbIn(void *handle, UsbConfiguration *usbConfiguration);

  void init() override;
  Status read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout) override;
  Status write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout) override;

  Status enable() override;
  Status disable() override;

  void rxDone(int16_t *usb_buffer, uint32_t rxBytes);
  uint32_t getReadCycles(bool reset);
  void setVolume(uint32_t level);
  void setMute(bool mute);
};


}
