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
//  Description: Hardware abstraction layer - Uart FreeRtos port
//  Filename: FreeRtosHalUart.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "FreeRtosHal.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include <algorithm>
#include <cstring>

#define RX_DMA_BUFFER_SIZE 64
#define TX_DMA_BUFFER_SIZE 256

namespace System
{

/**
 * FreeRtos compatible I2c wrapper constructor.
 * @param handle
 */
FreeRtosUart::FreeRtosUart(void *handle, UartInterface uartInterface) :
    handle(static_cast<UART_HandleTypeDef*>(handle)),
    txMutexHandle(nullptr),
    rxMutexHandle(nullptr),
    uartRxBuffer(nullptr),
    uartTxBuffer(nullptr),
    uartInterface(uartInterface),
    rxTaskHandle(nullptr),
    txTaskHandle(nullptr),
    rxDataFifo(nullptr, 0)
{
  this->txMutexHandle = xSemaphoreCreateMutexStatic(&txMutexBuffer);
  this->rxMutexHandle = xSemaphoreCreateMutexStatic(&rxMutexBuffer);
}

/**
 * Initialises UART wrapper and allocates DMA channels.
 * It starts an endless circular DMA on the RX DMA
 */
void FreeRtosUart::init()
{
  uartRxBuffer = globalServices->getMemAllocator()->alloc(RX_DMA_BUFFER_SIZE);
  uartTxBuffer = globalServices->getMemAllocator()->alloc(TX_DMA_BUFFER_SIZE);

  handle->priv = this;

  rxDataFifo.reset((uint8_t*) uartRxBuffer, RX_DMA_BUFFER_SIZE);

  const char *interfaceName = (uartInterface == System::UartInterface::UART_CTRL) ? "CTRL" : "DEBUG";

  // Start circular Rx DMA
  SET_BIT(handle->Instance->CR1, USART_CR1_RTOIE);
  if (HAL_UART_EnableReceiverTimeout(handle) != HAL_OK)
  {
    throw std::string("Failed to enable uart receiver timeout on interface: ") + interfaceName;
  }

  HAL_UART_ReceiverTimeout_Config(handle, 115200 / 20);

  if (HAL_UART_Receive_DMA(handle, (uint8_t*) uartRxBuffer, (uint16_t) (RX_DMA_BUFFER_SIZE)) != HAL_OK)
  {
    throw std::string("Failed to start RX DMA on interface: ") + interfaceName;
  }
}

/**
 * It reads at least one register from a connected device via I2C.
 *
 * @param deviceAddress
 * @param registerAddress
 * @param memAddSize
 * @param pData
 * @param size
 * @param timeout
 * @return
 */
Status FreeRtosUart::read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout)
{
  (void) deviceAddress;
  (void) registerAddress;
  (void) memAddSize;

  if (xSemaphoreTake(rxMutexHandle, timeout) != pdPASS)
  {
    return (timeout == 0) ? Status::STATUS_ERROR : Status::STATUS_TIMEOUT;
  }

  uint32_t availableSamples;
  int retries = 2;

  while (retries > 0)
  {
    retries--;

    availableSamples = std::min(rxDataFifo.getSampleCount(), (uint32_t) *size);
    if (availableSamples)
    {
      rxDataFifo.popBuffer(pData, availableSamples);
      break;
    }
    else if ((timeout > 0) && (retries > 0))
    {
      rxTaskHandle = xTaskGetCurrentTaskHandle();
      ulTaskNotifyTake(pdTRUE, timeout);
    }
  }

  *size = availableSamples;
  xSemaphoreGive(rxMutexHandle);

  return Status::STATUS_OK;
}

/**
 * It writes at least one register to a connected device via I2C.
 *
 * @param deviceAddress
 * @param registerAddress
 * @param memAddSize
 * @param pData
 * @param size
 * @param timeout
 * @throws exception when the DMA cannot start
 * @return
 */
Status FreeRtosUart::write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout)
{
  if (xSemaphoreTake(txMutexHandle, timeout) != pdPASS)
  {
    return (timeout == 0) ? Status::STATUS_ERROR : Status::STATUS_TIMEOUT;
  }

  uint32_t srcOffset = 0;
  while (size)
  {
    uint16_t txSize;

    txSize = std::min(size, (uint16_t) TX_DMA_BUFFER_SIZE);

    memcpy((void*) uartTxBuffer, &pData[srcOffset], txSize);
    txTaskHandle = xTaskGetCurrentTaskHandle();

    if (HAL_UART_Transmit_DMA(handle, (uint8_t*) uartTxBuffer, txSize) != HAL_OK)
    {
      throw std::string("Failed to start TX DMA on interface: ") + std::to_string(uartInterface);
    }

    if (timeout > 0)
    {
      ulTaskNotifyTake(pdTRUE, timeout);
    }

    srcOffset += txSize;
    size -= txSize;
  }

  xSemaphoreGive(txMutexHandle);

  return Status::STATUS_OK;
}

/**
 * Enables the UART interface.
 * Not applicable to I2C bus
 * @return
 */
Status FreeRtosUart::enable()
{
  //TODO: Consider moving the DMA start here
  return Status::STATUS_OK;
}

/**
 * Disables the UART interface.
 * Not applicable to I2C bus
 * @return
 */
Status FreeRtosUart::disable()
{
  //TODO: Consider moving the DMA stop here
  return Status::STATUS_OK;
}

/**
 * This method is called from inside the interrupt context to notify the class that new data have been received.
 * @param length the length of the data
 */
void FreeRtosUart::notifyDataReceived(uint16_t length)
{
  rxDataFifo.advanceBuffer(length);

  if (rxTaskHandle)
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(rxTaskHandle, &xHigherPriorityTaskWoken);
    rxTaskHandle = nullptr;
  }
}

/**
 * This method is called from inside the interrupt context to notify the class that all the data have been transmitted.
 */
void FreeRtosUart::notifyDataTransmitted()
{
  if (txTaskHandle)
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(txTaskHandle, &xHigherPriorityTaskWoken);
    txTaskHandle = nullptr;
  }
}

/**
 * Callback that runs upon the UART rx timeout event. It runs inside interrupt context,
 * so no heavy processing should be done in this context.
 * @param huart
 */
extern "C" void UART_TimeoutCallbackFromISR(void *handle)
{
  UART_HandleTypeDef *huart = static_cast<UART_HandleTypeDef*>(handle);

  uint32_t isrflags = READ_REG(huart->Instance->ISR);

  if (!(isrflags & USART_ISR_RTOF))
  {
    return;
  }

  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_RTOF);

  uint16_t len = (RX_DMA_BUFFER_SIZE) - ((DMA_Stream_TypeDef*) huart->hdmarx->Instance)->NDTR;
  FreeRtosUart *uart = static_cast<FreeRtosUart*>(huart->priv);
  uart->notifyDataReceived(len);
}

/**
 * Callback that runs upon the UART tx DMA end event. It runs inside interrupt context,
 * so no heavy processing should be done in this context.
 * @param huart
 */
extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  FreeRtosUart *uart = static_cast<FreeRtosUart*>(huart->priv);
  uart->notifyDataTransmitted();
}

}
