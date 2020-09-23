//====================================================================
//
// COPYRIGHT 2018 All rights reserved.
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
//  Description: usb composite audio & hid class
//  Filename: usbd_composite.c
//  Author(s): Kumar Bala (kb@socfpga.io)
//  Date: 20-October-2018
//
//====================================================================

#include "../Inc/usbd_composite.h"
#include "../../../USB_DEVICE/Target/usbd_conf.h"
#include "../../AUDIO/Inc/usbd_audio.h"
#include "../../CustomHID/Inc/usbd_customhid.h"

static uint8_t USBD_COMPOSITE_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_COMPOSITE_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_COMPOSITE_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t* USBD_COMPOSITE_GetCfgDesc(uint16_t *length);
static uint8_t* USBD_COMPOSITE_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t USBD_COMPOSITE_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_COMPOSITE_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_COMPOSITE_SOF(USBD_HandleTypeDef *pdev);
static uint8_t USBD_COMPOSITE_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_COMPOSITE_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);

USBD_ClassTypeDef USBD_COMPOSITE =
    {
        USBD_COMPOSITE_Init,
        USBD_COMPOSITE_DeInit,
        USBD_COMPOSITE_Setup,
        USBD_COMPOSITE_EP0_TxReady,
        USBD_COMPOSITE_EP0_RxReady,
        USBD_COMPOSITE_DataIn,
        USBD_COMPOSITE_DataOut,
        USBD_COMPOSITE_SOF,
        USBD_COMPOSITE_IsoINIncomplete,
        USBD_COMPOSITE_IsoOutIncomplete,
        USBD_COMPOSITE_GetCfgDesc,
        USBD_COMPOSITE_GetCfgDesc,
        USBD_COMPOSITE_GetCfgDesc,
        USBD_COMPOSITE_GetDeviceQualifierDesc,
    };

__ALIGN_BEGIN static uint8_t USBD_Composite_CfgDesc[] __ALIGN_END =
    {
        0x09, /* bLength */
        USB_DESC_TYPE_CONFIGURATION, /* bDescriptorType */
        0x00, //LOBYTE(USB_AUDIO_CONFIG_DESC_SIZ),    /* wTotalLength  109 bytes*/
        0x00, //HIBYTE(USB_AUDIO_CONFIG_DESC_SIZ),
        0x04, /* bNumInterfaces */
        0x01, /* bConfigurationValue */
        0x00, /* iConfiguration */
        0xC0, /* bmAttributes  BUS Powred*/
        0x32, /* bMaxPower = 100 mA*/
    /* 09 byte*/
    };

/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_Composite_DeviceQualifierDesc[] __ALIGN_END =
    {
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
        0x00,
        0x02,
        0x00,
        0x00,
        0x00,
        0x40,
        0x01,
        0x00,
    };

static uint8_t USBD_COMPOSITE_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_StatusTypeDef status;

  //TODO: Signal system status controller that everything is ok
  //BIST_set_status(USB_STATUS);

  status = USBD_AUDIO.Init(pdev, cfgidx);
  if (status != USBD_OK)
  {
    return status;
  }

  return USBD_CUSTOM_HID.Init(pdev, cfgidx);
}

static uint8_t USBD_COMPOSITE_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_AUDIO.DeInit(pdev, cfgidx);

  USBD_CUSTOM_HID.DeInit(pdev, cfgidx);

  return USBD_OK;
}

static uint8_t USBD_COMPOSITE_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  switch (LOBYTE(req->wIndex))
  {
  case 0 ... 2:
    USBD_AUDIO.Setup(pdev, req);
  break;

  case 3:
    USBD_CUSTOM_HID.Setup(pdev, req);
  break;
  }

  return USBD_OK;
}

static uint8_t* USBD_COMPOSITE_GetCfgDesc(uint16_t *total_length)
{
  uint16_t length;
  static uint8_t composite_cfg[512];

  *total_length = length = sizeof(USBD_Composite_CfgDesc);
  memcpy(&composite_cfg[0], USBD_Composite_CfgDesc, length);

  uint8_t *audio_cfg_descr = USBD_AUDIO.GetFSConfigDescriptor(&length);
  memcpy(&composite_cfg[*total_length], audio_cfg_descr, length);
  *total_length += length;

  uint8_t *hid_cfg_descr = USBD_CUSTOM_HID.GetFSConfigDescriptor(&length);
  memcpy(&composite_cfg[*total_length], hid_cfg_descr, length);
  *total_length += length;

  composite_cfg[2] = LOBYTE(*total_length);
  composite_cfg[3] = HIBYTE(*total_length);

  return composite_cfg;
}

static uint8_t* USBD_COMPOSITE_GetDeviceQualifierDesc(uint16_t *length)
{
  *length = sizeof(USBD_Composite_DeviceQualifierDesc);
  return USBD_Composite_DeviceQualifierDesc;
}

static uint8_t USBD_COMPOSITE_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  switch (epnum | 0x80)
  {
  case AUDIO_SYNCH_EP:
    case AUDIO_IN_EP:
    if (USBD_AUDIO.DataIn)
      return USBD_AUDIO.DataIn(pdev, epnum);
  break;

  case CUSTOM_HID_EPIN_ADDR:
    if (USBD_CUSTOM_HID.DataIn)
      return USBD_CUSTOM_HID.DataIn(pdev, epnum);
  break;
  }

  return USBD_OK;
}

static uint8_t USBD_COMPOSITE_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  switch (epnum)
  {
  case AUDIO_OUT_EP:
    if (USBD_AUDIO.DataOut)
      return USBD_AUDIO.DataOut(pdev, epnum);
  break;

  case CUSTOM_HID_EPOUT_ADDR:
    if (USBD_CUSTOM_HID.DataOut)
      return USBD_CUSTOM_HID.DataOut(pdev, epnum);
  break;
  }

  return USBD_OK;
}

static uint8_t USBD_COMPOSITE_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO.EP0_RxReady(pdev);

  USBD_CUSTOM_HID.EP0_RxReady(pdev);

  return USBD_OK;
}

static uint8_t USBD_COMPOSITE_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
  return USBD_OK;
}

static uint8_t USBD_COMPOSITE_SOF(USBD_HandleTypeDef *pdev)
{
  if (USBD_AUDIO.SOF)
    USBD_AUDIO.SOF(pdev);

  if (USBD_CUSTOM_HID.SOF)
    USBD_CUSTOM_HID.SOF(pdev);

  return USBD_OK;
}

static uint8_t USBD_COMPOSITE_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  switch (epnum)
  {
  case 1 ... 2:
    if (USBD_AUDIO.IsoINIncomplete)
      return USBD_AUDIO.IsoINIncomplete(pdev, epnum);
  break;

  case CUSTOM_HID_EPOUT_ADDR:
    if (USBD_CUSTOM_HID.IsoINIncomplete)
      return USBD_CUSTOM_HID.IsoINIncomplete(pdev, epnum);
  break;
  }

  return USBD_OK;
}

static uint8_t USBD_COMPOSITE_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  switch (epnum)
  {
  case AUDIO_OUT_EP:
    if (USBD_AUDIO.IsoOUTIncomplete)
      return USBD_AUDIO.IsoOUTIncomplete(pdev, epnum);
  break;

  case CUSTOM_HID_EPOUT_ADDR:
    if (USBD_CUSTOM_HID.IsoOUTIncomplete)
      return USBD_CUSTOM_HID.IsoOUTIncomplete(pdev, epnum);
  break;
  }
  return USBD_OK;
}
