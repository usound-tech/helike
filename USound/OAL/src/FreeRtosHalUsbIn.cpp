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
//  Description: Hardware abstraction layer - USB In FreeRtos port
//  Filename: FreeRtosHalUsbIn.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "Controllers/Audio/pub/AudioService.hpp"
#include "FreeRtosHal.hpp"
#include "Controllers/System/pub/SystemConfiguration.hpp"
#include "Utilities/Fifo/pub/Fifo.hpp"
#include "Interfaces/pub/SystemControl.hpp"

namespace System
{
#define CHANNEL_COUNT 2

/**
 * FreeRtos compatible Usb wrapper constructor.
 * @param handle
 */
FreeRtosUsbIn::FreeRtosUsbIn(void *handle, UsbConfiguration *usbConfiguration) :
    usbInFifo(nullptr, 0),
    usbConfiguration(usbConfiguration),
    handle(static_cast<USBD_HandleTypeDef*>(handle))
{

}

/**
 * Initialises Usb wrapper.
 */
void FreeRtosUsbIn::init()
{
  uint32_t usbBufferLen = (((usbConfiguration->frequency * usbConfiguration->bufferingTime) + 999) / 1000) * CHANNEL_COUNT;

  usbBuffer = new int16_t[usbBufferLen];
  memset(usbBuffer, 0, usbBufferLen * sizeof(uint16_t));

  usbInFifo.reset(usbBuffer, usbBufferLen);

  handle->priv = this;
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
Status FreeRtosUsbIn::read(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t *size, uint32_t timeout)
{
  *size = (uint16_t) usbInFifo.popBuffer((int16_t*) pData, *size);
  readCyclesCompleted++;
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
Status FreeRtosUsbIn::write(uint32_t deviceAddress, uint16_t registerAddress, uint16_t memAddSize, uint8_t *pData, uint16_t size, uint32_t timeout)
{
// UNSUPPORTED
  return Status::STATUS_OK;
}

/**
 * Starts the SAI DMA
 * @return
 */
Status FreeRtosUsbIn::enable()
{
// UNSUPPORTED
  return Status::STATUS_OK;
}

/**
 * Stops the SAI DMA
 * @return
 */
Status FreeRtosUsbIn::disable()
{
// UNSUPPORTED
  return Status::STATUS_OK;
}

/**
 * This callback runs inside interrupt context and notifies the Audio controller that more data
 * can be processed.
 *
 * @param dmaType
 */
void FreeRtosUsbIn::rxDone(int16_t *usbBuffer, uint32_t rxBytes)
{
  usbInFifo.pushBuffer(usbBuffer, rxBytes / sizeof(int16_t));

  globalServices->getAudioService()->notifyMoreDataAvailable();
}

/**
 * Returns the number of audio out cycles
 *
 * @param reset if 1 it resets the read cycles
 * @return
 */
uint32_t FreeRtosUsbIn::getReadCycles(bool resetCycles)
{
  return globalServices->getAudioService()->getAudioOutCycles(resetCycles);
}

/**
 * Sets audio engine volume
 *
 * @param level the audio level (0-0xFF)
 * @return
 */
void FreeRtosUsbIn::setVolume(uint32_t level)
{
  globalServices->getAudioService()->setVolume(level, AudioChangeSrc::ACS_USB);
}

/**
 * Sets audio engine mute
 *
 * @param mute true when mute is enabled
 * @return
 */
void FreeRtosUsbIn::setMute(bool mute)
{
  globalServices->getAudioService()->setMute(mute, AudioChangeSrc::ACS_USB);
}

/**
 * External callback that is invoked every time we get new data from the usb
 * @param pdev
 * @param usbBuffer
 * @param rxBytes
 */
extern "C" void USB_IN_RxData(USBD_HandleTypeDef *pdev, int16_t *usbBuffer, uint32_t rxBytes)
{
  FreeRtosUsbIn *usbIn = static_cast<FreeRtosUsbIn*>(pdev->priv);
  usbIn->rxDone(usbBuffer, rxBytes);
}

/**
 * Returns the number of audio out cycles
 * @return
 */
extern "C" uint32_t USB_IN_GetSaiReadCycles(USBD_HandleTypeDef *pdev, uint32_t reset)
{
  FreeRtosUsbIn *usbIn = static_cast<FreeRtosUsbIn*>(pdev->priv);
  return usbIn->getReadCycles(reset == 1);
}

/**
 * Sets audio engine mute
 * @return
 */
extern "C" void USB_IN_SetMute(USBD_HandleTypeDef *pdev, uint32_t mute)
{
  FreeRtosUsbIn *usbIn = static_cast<FreeRtosUsbIn*>(pdev->priv);
  usbIn->setMute(mute == 1);
}

/**
 * Sets audio engine mute
 * @return
 */
extern "C" void USB_IN_SetVolume(USBD_HandleTypeDef *pdev, uint32_t level)
{
  FreeRtosUsbIn *usbIn = static_cast<FreeRtosUsbIn*>(pdev->priv);
  usbIn->setVolume(level);
}

}
