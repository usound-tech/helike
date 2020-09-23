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
//  Description: Hardware abstraction layer - I2C FreeRtos port
//  Filename: FreeRtosHalI2c.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "FreeRtosHal.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include <cstring>

namespace System
{

/**
 * FreeRtos compatible I2c wrapper constructor.
 * @param handle
 */
FreeRtosI2c::FreeRtosI2c(void *handle, I2cInterface i2cInterface) :
    handle(static_cast<I2C_HandleTypeDef*>(handle)),
    i2cDmaBuffer(nullptr),
    i2cInterface(i2cInterface)
{
  this->mutexHandle = xSemaphoreCreateMutexStatic(&mutexBuffer);
}

/**
 * Initialises I2C wrapper and allocates DMA channels
 */
void FreeRtosI2c::init()
{
  i2cDmaBuffer = globalServices->getMemAllocator()->alloc(32);
  this->handle->priv = this;
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
Status FreeRtosI2c::read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout)
{
  if (xSemaphoreTake(mutexHandle, timeout) != pdPASS)
  {
    return (timeout == 0) ? Status::STATUS_ERROR : Status::STATUS_TIMEOUT;
  }

  taskHandle = xTaskGetCurrentTaskHandle();

  HAL_StatusTypeDef status = HAL_I2C_Mem_Read_IT(static_cast<I2C_HandleTypeDef*>(handle), static_cast<uint16_t>(deviceAddress), registerAddress, memAddSize,
      (uint8_t*) i2cDmaBuffer, *size);

  if (status == HAL_OK && timeout > 0)
  {
    ulTaskNotifyTake(pdTRUE, timeout);
  }

  memcpy(pData, (const void*) i2cDmaBuffer, *size);

  xSemaphoreGive(mutexHandle);

  return static_cast<Status>(status);
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
 * @return
 */
Status FreeRtosI2c::write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout)
{
  if (xSemaphoreTake(mutexHandle, timeout) != pdPASS)
  {
    return (timeout == 0) ? Status::STATUS_ERROR : Status::STATUS_TIMEOUT;
  }

  taskHandle = xTaskGetCurrentTaskHandle();
  memcpy((uint8_t*) i2cDmaBuffer, pData, size);

  HAL_StatusTypeDef status = HAL_I2C_Mem_Write_IT(static_cast<I2C_HandleTypeDef*>(handle), static_cast<uint16_t>(deviceAddress), registerAddress, memAddSize,
      (uint8_t*) i2cDmaBuffer, size);

  if (status == HAL_OK && timeout > 0)
  {
    ulTaskNotifyTake(pdTRUE, timeout);
  }

  xSemaphoreGive(mutexHandle);

  return static_cast<Status>(status);
}

/**
 * Enables the I2C interface.
 * Not applicable to I2C bus
 * @return
 */
Status FreeRtosI2c::enable()
{
  return Status::STATUS_OK;
}

/**
 * Disables the I2C interface.
 * Not applicable to I2C bus
 * @return
 */
Status FreeRtosI2c::disable()
{
  return Status::STATUS_OK;
}

/**
 * This function runs from an interrupt context when the I2C Rx DMA END event has arrived
 */
void FreeRtosI2c::dmaDone()
{
  if (taskHandle)
  {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(taskHandle, &xHigherPriorityTaskWoken);
    taskHandle = nullptr;
  }
}


/**
 * This callback function runs inside interrupt context when the I2C TX dma is complete
 * @param hi2c the I2C interface handle
 */
extern "C" void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  FreeRtosI2c *i2c = static_cast<FreeRtosI2c*>(hi2c->priv);
  i2c->dmaDone();
}

/**
 * This callback function runs inside interrupt context when the I2C RX dma is complete
 * @param hi2c the I2C interface handle
 */
extern "C" void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  FreeRtosI2c *i2c = static_cast<FreeRtosI2c*>(hi2c->priv);
  i2c->dmaDone();
}

}
