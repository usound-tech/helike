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
//  Description: Hardware abstraction layer - SAI Out FreeRtos port
//  Filename: FreeRtosHalSaiOut.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Controllers/Audio/pub/AudioService.hpp"
#include "FreeRtosHal.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Utilities/SimpleMemAllocator/pub/SimpleMemAllocator.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "sai.h"

namespace System
{
#define CHANNEL_COUNT 2

/**
 * FreeRtos compatible Sai wrapper constructor.
 * @param handle
 */
FreeRtosSaiOut::FreeRtosSaiOut(void *handle, SaiConfiguration *saiConfiguration) :
    saiOutFifo(nullptr, 0),
    handle(static_cast<SAI_HandleTypeDef*>(handle)),
    saiConfiguration(saiConfiguration),
    saiDmaBuffer(nullptr),
    dmaBufferLen(0)
{

}

/**
 * Initialises Sai wrapper and allocates DMA channels.
 * It starts an endless circular DMA on the TX DMA
 */
void FreeRtosSaiOut::init()
{
  // We need to allocate 2 buffers for DMA (ping-pong)
  dmaBufferLen = (((saiConfiguration->frequency * saiConfiguration->bufferingTime) + 999) / 1000) * 2 * CHANNEL_COUNT;
  saiDmaBuffer = globalServices->getMemAllocator()->alloc(dmaBufferLen * sizeof(uint16_t));

  memset(saiDmaBuffer, 0, dmaBufferLen * sizeof(uint16_t));

  saiOutFifo.reset((uint16_t*) saiDmaBuffer, dmaBufferLen);

  //TODO: Check if this should only be done in main, as it should be done prior to the DAC config
  //systemConfiguration->configureSaiOutInterface(saiInterface, System::PeripheralConfiguration::PERIPH_ENABLE);

  // Register priv data for DMA callback
  handle = static_cast<SAI_HandleTypeDef*>(saiConfiguration->handle);
  handle->priv = this;

  if (saiConfiguration->saiInterface == SaiInterface::TWEETER)
  {
    MX_SAI3_InitBlockA(saiConfiguration->saiMode);
  }
  else
  {
    MX_SAI3_InitBlockB(saiConfiguration->saiMode);
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
Status FreeRtosSaiOut::read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout)
{
  (void) deviceAddress;
  (void) registerAddress;
  (void) memAddSize;
//TODO: Unused for now. Reinstate for SAI slave interface
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
Status FreeRtosSaiOut::write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout)
{
  saiOutFifo.advanceToHalf(isDmaHalf);
  saiOutFifo.pushBuffer((const uint16_t*) pData, size);

  return Status::STATUS_OK;
}

/**
 * Starts the SAI DMA
 * @return
 */
Status FreeRtosSaiOut::enable()
{
  __HAL_SAI_ENABLE(handle);

  if (HAL_SAI_Transmit_DMA(handle, saiDmaBuffer, dmaBufferLen) != HAL_OK)
  {
    return Status::STATUS_ERROR;
  }

  return Status::STATUS_OK;
}

/**
 * Stops the SAI DMA
 * @return
 */
Status FreeRtosSaiOut::disable()
{
  if (HAL_SAI_DMAStop(handle) != HAL_OK)
  {
    return Status::STATUS_ERROR;
  }

  return Status::STATUS_OK;
}

/**
 * This callback runs inside interrupt context and notifies the Audio controller that more data
 * can be processed.
 *
 * @param dmaType
 */
void FreeRtosSaiOut::dmaDone(SaiDmaType dmaType)
{
  if (saiConfiguration->saiInterface == SaiInterface::TWEETER)
  {
    globalServices->getAudioService()->notifyMoreDataNeeded();
  }

  if (dmaType == SaiDmaType::FULL)
  {
    isDmaHalf = false;
  }
  else
  {
    isDmaHalf = true;
  }
}

extern "C" void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
  FreeRtosSaiOut *audioSaiOut = static_cast<FreeRtosSaiOut*>(hsai->priv);
  audioSaiOut->dmaDone(SaiDmaType::FULL);

//TODO: For UAC add rate calculation
}

/**
 * @brief  Manages the DMA full Transfer complete event.
 * @param  None
 * @retval None
 */
//void AUDIO_OUT_SAI_TransferComplete_CallBack(uint32_t id)
extern "C" void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
  FreeRtosSaiOut *audioSaiOut = static_cast<FreeRtosSaiOut*>(hsai->priv);
  audioSaiOut->dmaDone(SaiDmaType::HALF);
}

}
