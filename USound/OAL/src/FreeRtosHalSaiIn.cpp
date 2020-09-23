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
//  Description: Hardware abstraction layer - SAI In FreeRtos port
//  Filename: FreeRtosHalSaiIn.cpp
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
FreeRtosSaiIn::FreeRtosSaiIn(void *handle, SaiConfiguration *saiConfiguration) :
    saiInFifo(nullptr, 0),
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
void FreeRtosSaiIn::init()
{
  // We need to allocate 2 buffers for DMA (ping-pong)
  dmaBufferLen = (((saiConfiguration->frequency * saiConfiguration->bufferingTime) + 999) / 1000) * 2 * CHANNEL_COUNT;
  saiDmaBuffer = globalServices->getMemAllocator()->alloc(dmaBufferLen * sizeof(uint16_t));

  memset(saiDmaBuffer, 0, dmaBufferLen * sizeof(uint16_t));

  saiInFifo.reset((uint16_t*) saiDmaBuffer, dmaBufferLen);

  // Register priv data for DMA callback
  handle = static_cast<SAI_HandleTypeDef*>(saiConfiguration->handle);
  handle->priv = this;

  MX_SAI2_InitBlockA((int) saiConfiguration->saiMode);
}

/**
 * It reads data from the SAI buffer.
 *
 * @param deviceAddress
 * @param registerAddress
 * @param memAddSize
 * @param pData
 * @param size
 * @param timeout
 * @return
 */
Status FreeRtosSaiIn::read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout)
{
  saiInFifo.popBuffer((uint16_t*) pData, *size);
  return Status::STATUS_OK;
}

/**
 * It writes data to the SAI buffer.
 *
 * @param deviceAddress
 * @param registerAddress
 * @param memAddSize
 * @param pData
 * @param size
 * @param timeout
 * @return
 */
Status FreeRtosSaiIn::write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout)
{
  // UNSUPPORTED
  return Status::STATUS_OK;
}

/**
 * Starts the SAI DMA
 * @return
 */
Status FreeRtosSaiIn::enable()
{
  __HAL_SAI_ENABLE(handle);

  if (HAL_SAI_Receive_DMA(handle, saiDmaBuffer, dmaBufferLen) != HAL_OK)
  {
    return Status::STATUS_ERROR;
  }

  return Status::STATUS_OK;
}

/**
 * Stops the SAI DMA
 * @return
 */
Status FreeRtosSaiIn::disable()
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
void FreeRtosSaiIn::dmaDone(SaiDmaType dmaType)
{
  saiInFifo.advanceRxToHalf(dmaType == SaiDmaType::HALF);

  if (dmaType == SaiDmaType::FULL)
  {
    isDmaHalf = false;
    saiInFifo.advanceBuffer(0);
  }
  else
  {
    isDmaHalf = true;
    saiInFifo.advanceBuffer(dmaBufferLen / 2);
  }
  globalServices->getAudioService()->notifyMoreDataAvailable();
}

/**
 * Manages the DMA Rx full Transfer complete event.
 */
extern "C" void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef *hsai)
{
  FreeRtosSaiIn *audioSaiIn = static_cast<FreeRtosSaiIn*>(hsai->priv);
  audioSaiIn->dmaDone(SaiDmaType::FULL);
}

/**
 * Manages the DMA Rx half Transfer complete event.
 */
extern "C" void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
  FreeRtosSaiIn *audioSaiIn = static_cast<FreeRtosSaiIn*>(hsai->priv);
  audioSaiIn->dmaDone(SaiDmaType::HALF);
}

}
