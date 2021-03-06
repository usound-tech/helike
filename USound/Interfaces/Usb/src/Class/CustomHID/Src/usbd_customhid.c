/**
 ******************************************************************************
 * @file    usbd_customhid.c
 * @author  MCD Application Team
 * @version V2.4.2
 * @date    11-December-2015
 * @brief   This file provides the CUSTOM_HID core functions.
 *
 * @verbatim
 *
 *          ===================================================================
 *                                CUSTOM_HID Class  Description
 *          ===================================================================
 *           This module manages the CUSTOM_HID class V1.11 following the "Device Class Definition
 *           for Human Interface Devices (CUSTOM_HID) Version 1.11 Jun 27, 2001".
 *           This driver implements the following aspects of the specification:
 *             - The Boot Interface Subclass
 *             - Usage Page : Generic Desktop
 *             - Usage : Vendor
 *             - Collection : Application
 *
 * @note     In HS mode and when the DMA is used, all variables and data structures
 *           dealing with the DMA during the transaction process should be 32-bit aligned.
 *
 *
 *  @endverbatim
 *
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>

#include "../Inc/usbd_customhid.h"
#include "../../../USB_DEVICE/App/usbd_desc.h"
#include "../../../Core/Inc/usbd_ctlreq.h"

static uint8_t USBD_CUSTOM_HID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CUSTOM_HID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CUSTOM_HID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t* USBD_CUSTOM_HID_GetCfgDesc(uint16_t *length);
static uint8_t USBD_CUSTOM_HID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CUSTOM_HID_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CUSTOM_HID_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_CUSTOM_HID_SOF(USBD_HandleTypeDef *pdev);

USBD_ClassTypeDef USBD_CUSTOM_HID =
    {
        USBD_CUSTOM_HID_Init,
        USBD_CUSTOM_HID_DeInit,
        USBD_CUSTOM_HID_Setup,
        NULL, /*EP0_TxSent*/
        USBD_CUSTOM_HID_EP0_RxReady, /*EP0_RxReady*//* STATUS STAGE IN */
        USBD_CUSTOM_HID_DataIn, /*DataIn*/
        USBD_CUSTOM_HID_DataOut,
        USBD_CUSTOM_HID_SOF,
        NULL,
        NULL,
        USBD_CUSTOM_HID_GetCfgDesc,
        USBD_CUSTOM_HID_GetCfgDesc,
        USBD_CUSTOM_HID_GetCfgDesc,
        NULL,
    };

__ALIGN_BEGIN uint8_t CustomHID_ReportDesc[] __ALIGN_END = {
    0x06, 0xFF, 0x00, /* USAGE_PAGE (Vendor Page: 0xFF00) */
    0x09, 0x01, /* USAGE (Demo Kit) */
    0xa1, 0x01, /* COLLECTION (Application) */
    /* 6 */

    /* REPORT-IN */
    0x85, 0x01, /* REPORT_ID (1) */
    0x09, 0x01, /* USAGE (REPORT IN) */
    0x15, 0x00, /* LOGICAL_MINIMUM (0) */
    0x25, 0xFF, /* LOGICAL_MAXIMUM (255) */
    0x75, 0x08, /* REPORT_SIZE (8 bits) */
    0x95, 31, /* REPORT_COUNT (31) */
    0xB1, 0x82, /* FEATURE (Data,Var,Abs,Vol) */

    0x85, 0x01, /* REPORT_ID (1) */
    0x09, 0x01, /* USAGE (REPORT IN) */
    0x91, 0x82, /* OUTPUT (Data,Var,Abs,Vol) */
    /* 26 */

    /* REPORT-OUT */
    0x85, 0x02, /* REPORT_ID (2) */
    0x09, 0x02, /* USAGE (REPORT OUT) */
    0x15, 0x00, /* LOGICAL_MINIMUM (0) */
    0x25, 0xFF, /* LOGICAL_MAXIMUM (255) */
    0x75, 0x08, /* REPORT_SIZE (8) */
    0x95, 31, /* REPORT_COUNT (31) */
    0xb1, 0x82, /* FEATURE (Data,Var,Abs,Vol) */

    0x09, 0x02, /* USAGE (REPORT OUT) */
    0x75, 0x08, /* REPORT_SIZE (8) */
    0x95, 31, /* REPORT_COUNT (31) */
    0x81, 0x82, /* INPUT (Data,Var,Abs,Vol) */

    0xc0 /* END_COLLECTION */
};

/* USB CUSTOM_HID device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_CfgDesc[] __ALIGN_END =
    {
        /* HID class Interface Association Descriptor */
        0x08,                       // bLength: Interface Descriptor size
        USB_DESC_TYPE_IAD,          // bDescriptorType: IAD
        0x02,                       // bFirstInterface - starting of interface
        0x01,                       // bInterfaceCount - interfaces under this IAD class
        0x03,                       // bFunctionClass: HID
        0x00,                       // bFunctionSubClass
        0x00,                       // bFunctionProtocol
        0x06,                       // iFunction
        /* 08 byte*/

        /* CUSTOM HID interface Descriptor */
        0x09,                       // bLength: Interface Descriptor size
        USB_DESC_TYPE_INTERFACE,    // bDescriptorType: Interface descriptor type
        0x02,                       // bInterfaceNumber: Number of Interface
        0x00,                       // bAlternateSetting: Alternate setting
        0x01,                       // bNumEndpoints
        0x03,                       // bInterfaceClass: CUSTOM_HID
        0x00,                       // bInterfaceSubClass : 1=BOOT, 0=no boot
        0x00,                       // nInterfaceProtocol : 0=none, 1=keyboard, 2=mouse
        0x06,                       // iInterface: Index of string descriptor
        /* 09 bytes */

        /* CUSTOM_HID Descriptor */
        0x09,                       // bLength: CUSTOM_HID Descriptor size
        CUSTOM_HID_DESCRIPTOR_TYPE, // bDescriptorType: CUSTOM_HID
        0x11,                       // bCUSTOM_HIDUSTOM_HID: CUSTOM_HID Class Spec release number
        0x01,
        0x00,                       // bCountryCode: Hardware target country
        0x01,                       // bNumDescriptors: Number of CUSTOM_HID class descriptors to follow
        0x22,                       // bDescriptorType
        sizeof(CustomHID_ReportDesc),  // wItemLength: Total length of Report descriptor
        0x00,
        /* 09 bytes */

#if 0
        /* Custom HID endpoints Descriptor */
        0x07,                       // bLength: Endpoint Descriptor size
        USB_DESC_TYPE_ENDPOINT,     // bDescriptorType:
        CUSTOM_HID_EPIN_ADDR,       // bEndpointAddress: Endpoint Address (IN)
        0x03,                       // bmAttributes: Intr endpoint
        CUSTOM_HID_EPIN_SIZE,       // wMaxPacketSize: 8 Byte max
        0x00,
        0x06,                       // bInterval: Polling Interval (4 ms)
        /* 07 bytes */
#endif
        0x07,                       // bLength: Endpoint Descriptor size
        USB_DESC_TYPE_ENDPOINT,     // bDescriptorType:
        CUSTOM_HID_EPOUT_ADDR,      // bEndpointAddress: Endpoint Address (OUT)
        0x02,                       // bmAttributes: Bulk endpoint
        CUSTOM_HID_EPOUT_SIZE,      // wMaxPacketSize: 8 Bytes max
        0x00,
        0x06,                       // bInterval: Polling Interval (4 ms)
    /* 07 bytes */
    };

extern void Telemetry_AddCommand(USBD_HandleTypeDef *pdev, uint8_t *dataIn);
extern void Telemetry_GetReply(USBD_HandleTypeDef *pdev, uint8_t *dataOut);

/**
 * @brief  USBD_CUSTOM_HID_Init
 *         Initialize the CUSTOM_HID interface
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_CUSTOM_HID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0;
  USBD_CUSTOM_HID_HandleTypeDef *hhid;

  USBD_LL_OpenEP(pdev, CUSTOM_HID_EPOUT_ADDR, USBD_EP_TYPE_BULK, CUSTOM_HID_EPOUT_SIZE);
  USBD_LL_FlushEP(pdev, CUSTOM_HID_EPOUT_ADDR);

  pdev->pHidClassData = USBD_malloc(sizeof(USBD_CUSTOM_HID_HandleTypeDef));
  if (pdev->pHidClassData == NULL)
  {
    ret = 1;
  }
  else
  {
    hhid = (USBD_CUSTOM_HID_HandleTypeDef*) pdev->pHidClassData;
    hhid->state = CUSTOM_HID_IDLE;

    /* Prepare Out endpoint to receive 1st packet */
    USBD_LL_PrepareReceive(pdev, CUSTOM_HID_EPOUT_ADDR, hhid->Report_buf, USBD_CUSTOMHID_REPORT_BUF_SIZE);
  }

  return ret;
}

/**
 * @brief  USBD_CUSTOM_HID_Init
 *         DeInitialize the CUSTOM_HID layer
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_CUSTOM_HID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_LL_CloseEP(pdev, CUSTOM_HID_EPOUT_ADDR);

  /* Free allocated memory */
  if (pdev->pHidClassData != NULL)
  {
    USBD_free(pdev->pHidClassData);
    pdev->pHidClassData = NULL;
  }
  return USBD_OK;
}

/**
 * @brief  USBD_CUSTOM_HID_Setup
 *         Handle the CUSTOM_HID specific requests
 * @param  pdev: instance
 * @param  req: usb requests
 * @retval status
 */
static uint8_t USBD_CUSTOM_HID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  uint16_t len = 0;
  uint8_t *pbuf = NULL;
  USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef*) pdev->pHidClassData;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS:
      switch (req->bRequest)
      {
        case CUSTOM_HID_REQ_SET_PROTOCOL:
          hhid->Protocol = (uint8_t) (req->wValue);
          break;

        case CUSTOM_HID_REQ_GET_PROTOCOL:
          USBD_CtlSendData(pdev, (uint8_t*) &hhid->Protocol, 1);
          break;

        case CUSTOM_HID_REQ_SET_IDLE:
          hhid->IdleState = (uint8_t) (req->wValue >> 8);

          USBD_LL_FlushEP(pdev, CUSTOM_HID_EPIN_ADDR);
          break;

        case CUSTOM_HID_REQ_GET_IDLE:
          USBD_CtlSendData(pdev, (uint8_t*) &hhid->IdleState, 1);
          break;

        case CUSTOM_HID_REQ_SET_REPORT:
          hhid->IsReportAvailable = 1;
          USBD_CtlPrepareRx(pdev, hhid->Report_buf, (uint8_t) (req->wLength));
          break;

        case CUSTOM_HID_REQ_GET_REPORT:
          Telemetry_GetReply(pdev, hhid->Report_buf);
          len = MIN(USBD_CUSTOMHID_REPORT_BUF_SIZE, req->wLength);
          USBD_CtlSendData(pdev, (uint8_t*) &hhid->Report_buf, len);
          break;

        default:
          USBD_CtlError(pdev, req);
          return USBD_FAIL;
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_DESCRIPTOR:
          if (req->wValue >> 8 == CUSTOM_HID_REPORT_DESC)
          {
            uint32_t desc_len = sizeof(CustomHID_ReportDesc);

            pbuf = CustomHID_ReportDesc;
            len = MIN(desc_len, req->wLength);
          }
          else if (req->wValue >> 8 == CUSTOM_HID_DESCRIPTOR_TYPE)
          {
            pbuf = USBD_CUSTOM_HID_CfgDesc + 17;
            len = MIN(USB_CUSTOM_HID_DESC_SIZ, req->wLength);
          }

          USBD_CtlSendData(pdev, pbuf, len);
          break;

        case USB_REQ_GET_INTERFACE:
          USBD_CtlSendData(pdev, (uint8_t*) &hhid->AltSetting, 1);
          break;

        case USB_REQ_SET_INTERFACE:
          hhid->AltSetting = (uint8_t) (req->wValue);
          break;
      }
  }
  return USBD_OK;
}

/**
 * @brief  USBD_CUSTOM_HID_SendReport
 *         Send CUSTOM_HID Report
 * @param  pdev: device instance
 * @param  buff: pointer to report
 * @retval status
 */
uint8_t USBD_CUSTOM_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len)
{
  USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef*) pdev->pHidClassData;

  if (pdev->dev_state == USBD_STATE_CONFIGURED)
  {
    if (hhid->state == CUSTOM_HID_IDLE)
    {
      hhid->state = CUSTOM_HID_BUSY;
      USBD_LL_Transmit(pdev, CUSTOM_HID_EPIN_ADDR, report, len);
    }
  }
  return USBD_OK;
}

/**
 * @brief  USBD_CUSTOM_HID_GetCfgDesc
 *         return configuration descriptor
 * @param  speed : current device speed
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t* USBD_CUSTOM_HID_GetCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_CUSTOM_HID_CfgDesc);
  return USBD_CUSTOM_HID_CfgDesc;
}

/**
 * @brief  USBD_CUSTOM_HID_DataIn
 *         handle data IN Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_CUSTOM_HID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  /* Ensure that the FIFO is empty before a new transfer, this condition could
   be caused by  a new transfer before the end of the previous transfer */
  ((USBD_CUSTOM_HID_HandleTypeDef*) pdev->pHidClassData)->state = CUSTOM_HID_IDLE;

  return USBD_OK;
}

/**
 * @brief  USBD_CUSTOM_HID_DataOut
 *         handle data OUT Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_CUSTOM_HID_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef*) pdev->pHidClassData;

  Telemetry_AddCommand(pdev, hhid->Report_buf);

  USBD_LL_PrepareReceive(pdev, CUSTOM_HID_EPOUT_ADDR, hhid->Report_buf, USBD_CUSTOMHID_REPORT_BUF_SIZE);

  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_SOF
 *         handle SOF event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_CUSTOM_HID_SOF(USBD_HandleTypeDef *pdev)
{
  return 0;
}

/**
 * @brief  Handles control request data.
 * @param  pdev: device instance
 * @retval status
 */
uint8_t USBD_CUSTOM_HID_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef*) pdev->pHidClassData;

  if (hhid->IsReportAvailable == 1)
  {
    Telemetry_AddCommand(pdev, hhid->Report_buf);
    hhid->IsReportAvailable = 0;
  }

  return USBD_OK;
}


/**
 * @brief  USBD_CUSTOM_HID_RegisterInterface
 * @param  pdev: device instance
 * @param  fops: CUSTOMHID Interface callback
 * @retval status
 */
uint8_t USBD_CUSTOM_HID_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_CUSTOM_HID_ItfTypeDef *fops)
{
  uint8_t ret = USBD_FAIL;

  if (fops != NULL)
  {
    pdev->pHidUserData = fops;
    ret = USBD_OK;
  }

  return ret;
}
