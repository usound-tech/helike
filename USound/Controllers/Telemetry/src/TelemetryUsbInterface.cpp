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
//  Description: The adapter between the USB HID and Telemetry
//  Filename: TelemetryUsbInterface.cpp
//  Author(s): Nik Kostaras (nk@socfpga.io)
//  Date: 6-May-2020
//
//====================================================================

#include "../pub/Telemetry.hpp"
#include "Interfaces/Usb/src/Class/CustomHID/Inc/usbd_customhid.h"

namespace Controller
{

/**
 * Enqueues a new Telemetry command
 * @return
 */
extern "C" void Telemetry_AddCommand(USBD_HandleTypeDef *pdev, uint8_t *dataIn)
{
  Telemetry *telemetry = static_cast<Telemetry*>(pdev->hid_priv);
  telemetry->addCommand(dataIn);
}

/**
 * Polls the Telemetry block for the completion event of a previous command
 * @param pdev
 * @param dataOut
 */
extern "C" void Telemetry_GetReply(USBD_HandleTypeDef *pdev, uint8_t *dataOut)
{
  Telemetry *telemetry = static_cast<Telemetry*>(pdev->hid_priv);
  telemetry->getReply(dataOut);
}

}
