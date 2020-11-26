/**
 ******************************************************************************
 * @file    usbd_audio.c
 * @author  MCD Application Team
 * @version V2.4.2
 * @date    11-December-2015
 * @brief   This file provides the Audio core functions.
 *
 * @verbatim
 *
 *          ===================================================================
 *                                AUDIO Class  Description
 *          ===================================================================
 *           This driver manages the Audio Class 1.0 following the "USB Device Class Definition for
 *           Audio Devices V1.0 Mar 18, 98".
 *           This driver implements the following aspects of the specification:
 *             - Device descriptor management
 *             - Configuration descriptor management
 *             - Standard AC Interface Descriptor management
 *             - 1 Audio Streaming Interface (with single channel, PCM, Stereo mode)
 *             - 1 Audio Streaming Endpoint
 *             - 1 Audio Terminal Input (1 channel)
 *             - Audio Class-Specific AC Interfaces
 *             - Audio Class-Specific AS Interfaces
 *             - AudioControl Requests: only SET_CUR and GET_CUR requests are supported (for Mute)
 *             - Audio Feature Unit (limited to Mute control)
 *             - Audio Synchronization type: Asynchronous
 *             - Single fixed audio sampling rate (configurable in usbd_conf.h file)
 *          The current audio class version supports the following audio features:
 *             - Pulse Coded Modulation (PCM) format
 *             - sampling rate: 48KHz.
 *             - Bit resolution: 16
 *             - Number of channels: 2
 *             - No volume control
 *             - Mute/Unmute capability
 *             - Asynchronous Endpoints
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
#include "../Inc/usbd_audio.h"
#include "../../../USB_DEVICE/App/usbd_desc.h"
#include "../../../Core/Inc/usbd_ctlreq.h"
#include <stdint.h>
#include "arm_math.h"
#include "main.h"

#define AUDIO_SAMPLE_FREQ(frq)      (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_OUT_PACKET_NUM_THREE_QUARTERS (3 * AUDIO_OUT_PACKET_NUM / 4)
#define DMA_TIMEOUT_IN_MS(MS) (((MS) + AUDIO_OUT_PACKET_NUM - 1) / AUDIO_OUT_PACKET_NUM)

//! The maximum size of data we can receive per transaction.
//! It includes an extra sample for all channels, in case the rate adjustment from the OS sends us an extra byte
#define EP_OUT_REQ_LEN (AUDIO_USB_OUT_PACKET + BYTES_PER_USB_SAMPLE)

//! The feedback endpoint buffer size
#define EP_FEEDBACK_REQ_LEN 4


#define FRONT_LEFT          0x01
#define FRONT_RIGHT         0x02
#define FRONT_FRONT_CENTER  0x04
#define LOW_FREQ            0x08
#define BACK_LEFT           0x10
#define BACK_RIGHT          0x20
#define SIDE_LEFT           0x02
#define SIDE_RIGHT          0x04

#define USB_STEREO FRONT_LEFT | FRONT_RIGHT, 0x00, 0x00, 0x00


#define MUTE_BITS     0x03
#define VOLUME_BITS   0x0C
#define VOL_MUTE_CTRL (MUTE_BITS | VOLUME_BITS), 0x00, 0x00, 0x00
#define MUTE_CTRL     MUTE_BITS, 0x00, 0x00, 0x00
#define NO_CTRL       0x00, 0x00, 0x00, 0x00

// USB OUT parameters
#define USB_AUDIO_CHANNELS                                  2 //! Stereo input
#define SAMPLES_PER_USB_CHUNK                              48 //! The average number of samples the USB pushes to audio on every transaction (per channel)
#define CHUNK_TRANSACTIONS_PER_MS                           1 //! USB transfers new chunks every 1ms
#define AUDIO_BYTES_PER_USB_SAMPLE                          2 //! 16bit samples
#define BYTES_PER_USB_SAMPLE \
  (USB_AUDIO_CHANNELS * AUDIO_BYTES_PER_USB_SAMPLE)           //! Number of bytes for one sample of all channels

#define EP_SIZE (USB_AUDIO_CHANNELS * (SAMPLES_PER_USB_CHUNK+1) * AUDIO_BYTES_PER_USB_SAMPLE)
#define EP_SIZE_CFG ((uint8_t)EP_SIZE), ((uint8_t)(EP_SIZE>>8))

//USB OUT audio parameters
#define DEFAULT_OUT_VOLUME                               0xFF //! Audio Out default volume
#define OUT_DEFAULT_FREQUENCY                           48000 //! 40 kHz


// USB IN parameters
#define MIC_EP_SIZE ((16+8) * 2) // 16 samples per ms * 2 bytes per sample
#define MIC_EP_SIZE_CFG ((uint8_t)MIC_EP_SIZE), ((uint8_t)(MIC_EP_SIZE>>8))

extern void USB_IN_RxData(USBD_HandleTypeDef *pdev, int16_t *usb_buffer, uint32_t rxBytes);
extern uint32_t USB_IN_GetSaiReadCycles(USBD_HandleTypeDef *pdev, uint32_t reset);
extern void USB_IN_SetMute(USBD_HandleTypeDef *pdev, uint32_t mute);
extern void USB_IN_SetVolume(USBD_HandleTypeDef *pdev, uint32_t level);

static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t* USBD_AUDIO_GetCfgDesc(uint16_t *length);
//static uint8_t  *USBD_AUDIO_GetDeviceQualifierDesc (uint16_t *length);
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev);
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_AUDIO_IsoInIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
//static void AUDIO_REQ_GetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);


USBD_ClassTypeDef USBD_AUDIO = {
    USBD_AUDIO_Init,
    USBD_AUDIO_DeInit,
    USBD_AUDIO_Setup,
    USBD_AUDIO_EP0_TxReady,
    USBD_AUDIO_EP0_RxReady,
    USBD_AUDIO_DataIn,
    USBD_AUDIO_DataOut,
    USBD_AUDIO_SOF,
    USBD_AUDIO_IsoInIncomplete,
    USBD_AUDIO_IsoOutIncomplete,
    USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetCfgDesc,
    USBD_AUDIO_GetCfgDesc,
    NULL	//USBD_AUDIO_GetDeviceQualifierDesc,
    };

/* USB AUDIO device Configuration Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_CfgDesc[] __ALIGN_END = {
    /* Audio class Interface Association Descriptor */
    0x08,                                 // bLength: Interface Descriptor size
    USB_DESC_TYPE_IAD,                    // bDescriptorType: IAD
    0x00,                                 // bFirstInterface - starting of interface
    0x02,                                 // bInterfaceCount - interfaces under this IAD class
    USB_DEVICE_CLASS_AUDIO,               // bFunctionClass: UAC
    AUDIO_SUBCLASS_AUDIOCONTROL,          // bFunctionSubClass
    0x20,                                 // bFunctionProtocol
    0x00,                                 // iFunction
    /* 08 byte*/

    /* USB Speaker Standard interface descriptor */
    AUDIO_INTERFACE_DESC_SIZE,            // bLength
    USB_DESC_TYPE_INTERFACE,              // bDescriptorType
    0x00,                                 // bInterfaceNumber
    0x00,                                 // bAlternateSetting
    0x00,                                 // bNumEndpoints
    USB_DEVICE_CLASS_AUDIO,               // bInterfaceClass
    AUDIO_SUBCLASS_AUDIOCONTROL,          // bInterfaceSubClass
    0x20,                                 // bInterfaceProtocol
    0x00,                                 // iInterface
    /* 09 byte*/

    /* USB Speaker Class-specific AC Interface Descriptor */
    9,                                    // bLength
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      // bDescriptorType
    AUDIO_CONTROL_HEADER,                 // bDescriptorSubtype
    0x00, 0x02,                           // bcdADC - 2.00
    0x04,                                 // bCatagory - Headset
    55, 0x00,                             // wTotalLength
    0x00,                                 // bmControls
    /* 9 byte*/

    /* USB Speaker Clock Source Descriptor */
    8,                                    // bLength: 8
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      // bDescriptorType
    0x0A,                                 // bDescriptorSubtype - CLOCK_SOURCE
    0x0A,                                 // bClockID
    0x05,                                 // bmAttributes - Internal clock, synced to SOF
    0x01,                                 // bmControls
    0x00,                                 // bAssocTerminal
    0x00,                                 // iClockSource (String Index)
    /* 8 byte*/

    /* USB Speaker Input Terminal Descriptor 1 */
    0x11,                                 // bLength
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      // bDescriptorType
    AUDIO_CONTROL_INPUT_TERMINAL,         // bDescriptorSubtype
    AUDIO_OUT_ID,                         // bTerminalID
    0x01, 0x01,                           // wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101
    0x00,                                 // bAssocTerminal
    0x0A,                                 // bCSourceID: ID of Clock Entity
    USB_AUDIO_CHANNELS,                   // bNrChannels
    USB_STEREO,                           // bmChannelConfig - no spatial info
    0x00,                                 // iChannelNames
    0x00, 0x00,                           // bmControls
    0x00,                                 // iTerminal
    /* 17 byte*/

    /* USB Speaker Audio Feature Unit Descriptor */
    18,                                   // bLength
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      // bDescriptorType
    AUDIO_CONTROL_FEATURE_UNIT,           // bDescriptorSubtype
    AUDIO_OUT_STREAMING_CTRL,             // bUnitID
    AUDIO_OUT_ID,                         // bSourceID
    VOL_MUTE_CTRL,                        // bmaControls(Master)
    NO_CTRL,                              // bmaControls(1)
    NO_CTRL,                              // bmaControls(2)
    0x00,                                 // iTerminal
    /* 18 bytes */

    /*USB Speaker Output Terminal Descriptor */
    0x0C,                                 // bLength
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      // bDescriptorType
    AUDIO_CONTROL_OUTPUT_TERMINAL,        // bDescriptorSubtype
    0x03,                                 // bTerminalID
    0x01, 0x03,                           // wTerminalType  0x0301
    0x00,                                 // bAssocTerminal
    AUDIO_OUT_STREAMING_CTRL,             // bSourceID
    0x0A,                                 // bCSourceID
    0x00, 0x00,                           // bmControls
    0x00,                                 // iTerminal
    /* 12 byte*/

    /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Zero Bandwidth */
    /* Interface 1, Alternate Setting 0                                             */
    AUDIO_INTERFACE_DESC_SIZE,            // bLength
    USB_DESC_TYPE_INTERFACE,              // bDescriptorType
    0x01,                                 // bInterfaceNumber
    0x00,                                 // bAlternateSetting
    0x00,                                 // bNumEndpoints
    USB_DEVICE_CLASS_AUDIO,               // bInterfaceClass
    AUDIO_SUBCLASS_AUDIOSTREAMING,        // bInterfaceSubClass
    0x20,                                 // bInterfaceProtocol - AF_VERSION_02_00
    0x00,                                 // iInterface
    /* 09 byte*/

    /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Operational */
    /* Interface 1, Alternate Setting 1                                           */
    AUDIO_INTERFACE_DESC_SIZE,            // bLength
    USB_DESC_TYPE_INTERFACE,              // bDescriptorType
    0x01,                                 // bInterfaceNumber
    0x01,                                 // bAlternateSetting
    0x02,                                 // bNumEndpoints
    USB_DEVICE_CLASS_AUDIO,               // bInterfaceClass
    AUDIO_SUBCLASS_AUDIOSTREAMING,        // bInterfaceSubClass
    0x20,                                 // bInterfaceProtocol - AF_VERSION_02_00
    0x00,                                 // iInterface
    /* 09 byte*/

    /* USB Speaker Class Specific AS Interface Descriptor */
    0x10,                                 // bLength: 16
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      // bDescriptorType: 0x24
    AUDIO_STREAMING_GENERAL,              // bDescriptorSubType
    AUDIO_OUT_ID,                         // bTerminalLink (Linked to USB input terminal)
    0x00,                                 // bmControls
    0x01,                                 // bFormatType
    0x01, 0x00, 0x00, 0x00,               // bmFormats - PCM
    USB_AUDIO_CHANNELS,                   // bNrChannels
    USB_STEREO,                           // bmChannelConfig - no spatial info
    0x00,                                 // iChannelNames
    /* 16 byte*/

    /* USB Speaker Type I Format Type Descriptor */
    0x06,                                 // bLength
    AUDIO_INTERFACE_DESCRIPTOR_TYPE,      // bDescriptorType
    AUDIO_STREAMING_FORMAT_TYPE,          // bDescriptorSubtype
    AUDIO_FORMAT_TYPE_I,                  // bFormatType
    AUDIO_BYTES_PER_USB_SAMPLE,           // bSubslotSize - bytes per subslot
    (AUDIO_BYTES_PER_USB_SAMPLE * 8),     // bBitResolution - bits per subslot
    /* 6 byte*/

    /* Standard AS Isochronous Audio Data Endpoint Descriptor */
    0x07,                                 // bLength
    USB_DESC_TYPE_ENDPOINT,               // bDescriptorType
    AUDIO_OUT_EP,                         // bEndpointAddress (D7: 0:out, 1:in)
    USBD_EP_TYPE_ISOC | 0x04,             // bmAttributes iso + async
    EP_SIZE_CFG,                          // wMaxPacketSize
    1,                                    // bInterval - windows needs this to be 1 for UAC 2.0
    /* 7 byte*/

    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor */
    0x08,                                 // bLength
    AUDIO_ENDPOINT_DESCRIPTOR_TYPE,       // bDescriptorType
    AUDIO_ENDPOINT_GENERAL,               // bDescriptorSubtype
    0x00,                                 // bmAttributes
    0x00,                                 // bmControls (Bitmap: Pitch control, over/underun etc)
    0x00,                                 // bLockDelayUnits: Decoded PCM samples
    0, 0,                                 // bLockDelay - 0 PCM samples
    /* 8 byte*/

    /* Standard AS Isochronous Feedback Endpoint Descriptor */
    0x07,                                 // bLength
    USB_DESC_TYPE_ENDPOINT,               // bDescriptorType
    AUDIO_SYNCH_EP,                       // bEndpointAddress (D7: 0:out, 1:in)
    USBD_EP_TYPE_ISOC | 0x10,             // bmAttributes iso + feedback
    0x04, 0x00,                           // wMaxPacketSize
    1,                                    // bInterval - 4 is 1ms, 8 is 16ms
    /* 7 byte*/
    };

#if 0
/* USB Standard Device Descriptor */
__ALIGN_BEGIN static uint8_t USBD_AUDIO_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END=
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
#endif

volatile uint32_t SOF_num_feedback = 0;
volatile uint32_t lockedSampling = 0;
volatile uint32_t lockedCount = 0;
volatile uint32_t usb_cycles = 0;   // counts cycles of 8ms
volatile uint32_t first_sync_with_sai = 0;   // detects first sai dma end
static uint32_t mute = 0;
static uint16_t curvol = DEFAULT_OUT_VOLUME;

int16_t usb_buffer[EP_OUT_REQ_LEN];

/**
 * @brief  USBD_AUDIO_Init
 *         Initialize the AUDIO interface
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_AUDIO_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  /* Open EP OUT */
  USBD_LL_OpenEP(pdev, AUDIO_OUT_EP, USBD_EP_TYPE_ISOC, EP_OUT_REQ_LEN);

  USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, (uint8_t*) usb_buffer, EP_OUT_REQ_LEN);
  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_Init
 *         DeInitialize the AUDIO layer
 * @param  pdev: device instance
 * @param  cfgidx: Configuration index
 * @retval status
 */
static uint8_t USBD_AUDIO_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_LL_CloseEP(pdev, AUDIO_OUT_EP);
  USBD_LL_CloseEP(pdev, AUDIO_SYNCH_EP);

  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_Setup
 *         Handle the AUDIO specific requests
 * @param  pdev: instance
 * @param  req: usb requests
 * @retval status
 */
static uint8_t USBD_AUDIO_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  uint16_t len;
  uint8_t *pbuf;
  uint8_t ret = USBD_OK;

  uint8_t control_selector = HIBYTE(req->wValue);
  uint8_t channel = LOBYTE(req->wValue);
  uint8_t entity_id = HIBYTE(req->wIndex);
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pAudioClassData;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS:
      switch (req->bRequest)
      {
        case AUDIO_REQ_CUR:
          if (req->bmRequest & 0x80) // Get request
          {
            switch (entity_id)
            {
              case AUDIO_OUT_STREAMING_CTRL:
                // Speaker function unit
                if (control_selector == 0x01)
                {
                  uint32_t muteVal = !!(mute & (1 << channel));
                  USBD_CtlSendData(pdev, (uint8_t*) &muteVal, req->wLength);
                }
                else if (control_selector == 0x02)
                {
                  USBD_CtlSendData(pdev, (uint8_t*) &curvol, req->wLength);
                }
                break;

              case 0x0A: {
                // Speaker clock unit
                static uint32_t curfreq = OUT_DEFAULT_FREQUENCY;
                USBD_CtlSendData(pdev, (uint8_t*) &curfreq, req->wLength);
                break;
              }
            }
          }
          else
          {
            AUDIO_REQ_SetCurrent(pdev, req);
          }
          break;

        case AUDIO_REQ_RANGE:
          if (req->bmRequest & 0x80) // Get request
          {
            switch (entity_id)
            {
              case AUDIO_OUT_STREAMING_CTRL: {
                // Function unit for speakers

                static uint16_t vol[] = { 0x0001, -4 * 256, 4 * 256, 8 };
                uint16_t retLen = MIN(sizeof(vol), req->wLength);
                USBD_CtlSendData(pdev, (uint8_t*) &vol, retLen);
                break;
              }

              case 0x0A: {
                // Clock unit for speakers

                static uint16_t freq[] = { 0x0001, 48000, 0, 48000, 0, 0, 0 };
                uint16_t retLen = MIN(sizeof(freq), req->wLength);
                USBD_CtlSendData(pdev, (uint8_t*) &freq, retLen);
                break;
              }
            }
          }
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest)
      {
        case USB_REQ_GET_DESCRIPTOR:
          if ((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE)
          {
            pbuf = USBD_AUDIO_CfgDesc + 17;
            len = MIN(USB_AUDIO_DESC_SIZ, req->wLength);
            USBD_CtlSendData(pdev, pbuf, len);
          }
          break;

        case USB_REQ_GET_INTERFACE:
          USBD_CtlSendData(pdev, (uint8_t*) &(haudio->alt_setting), 1);
          break;

        case USB_REQ_SET_INTERFACE:
          if ((uint8_t) (req->wIndex) < USBD_MAX_NUM_INTERFACES)
          {
            haudio->alt_setting = (uint8_t) (req->wValue);
            lockedSampling = (int32_t) (48 << 13);
            lockedCount = 0;
            usb_cycles = 0;

            if (haudio->alt_setting)
            {
              USB_IN_GetSaiReadCycles(pdev, 1);
              first_sync_with_sai = 1;
              SOF_num_feedback = 0;

              USBD_LL_OpenEP(pdev, AUDIO_SYNCH_EP, USBD_EP_TYPE_ISOC, EP_FEEDBACK_REQ_LEN);
            }
            else
            {
              USBD_LL_CloseEP(pdev, AUDIO_SYNCH_EP);
            }
          }
          else
          {
            // Call the error management function (command will be nacked)
            USBD_CtlError(pdev, req);
          }
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
      }
  }
  return ret;
}

/**
 * @brief  USBD_AUDIO_GetCfgDesc
 *         return configuration descriptor
 * @param  speed : current device speed
 * @param  length : pointer data length
 * @retval pointer to descriptor buffer
 */
static uint8_t* USBD_AUDIO_GetCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_AUDIO_CfgDesc);
  return USBD_AUDIO_CfgDesc;
}

/**
 * @brief  USBD_AUDIO_DataIn
 *         handle data IN Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  // flags that the feedback was sent and resets the counter for the next tx
  switch (epnum & 0x7F)
  {
    case 1:
      SOF_num_feedback = 0;
      break;
  }

  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_EP0_RxReady
 *         handle EP0 Rx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_AUDIO_EP0_RxReady(USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pAudioClassData;

  switch (haudio->control.cmd)
  {
    case AUDIO_REQ_SET_CUR:
      switch (haudio->control.unit)
      {
        case AUDIO_OUT_STREAMING_CTRL:
          if (haudio->control.control_selector == 0x01)
          {
            if (haudio->control.data[0])
            {
              mute |= (1 << haudio->control.channel);
            }
            else
            {
              mute &= ~(1 << haudio->control.channel);
            }

            USB_IN_SetMute(pdev, mute != 0);
          }
          else if (haudio->control.control_selector == 0x02)
          {
            curvol = haudio->control.data[0] | ((uint16_t) haudio->control.data[1] << 8);

            // Received value is between -1024 to 1024, so we convert to 0 to 255
            uint16_t val = (((curvol + 1024) & 0xFFFF) >> 3);
            USB_IN_SetVolume(pdev, MIN(255, val));
          }
          break;
      }
      break;
  }
  haudio->control.cmd = 0;
  haudio->control.len = 0;

  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_EP0_TxReady
 *         handle EP0 TRx Ready event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_AUDIO_EP0_TxReady(USBD_HandleTypeDef *pdev)
{
  /* Only OUT control data are processed */
  return USBD_OK;
}


/**
 * @brief  USBD_AUDIO_SOF
 *         handle SOF event
 * @param  pdev: device instance
 * @retval status
 */
static uint8_t USBD_AUDIO_SOF(USBD_HandleTypeDef *pdev)
{
  USBD_AUDIO_HandleTypeDef *haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pAudioClassData;

  if (SOF_num_feedback < 20)
  {
    SOF_num_feedback++;
  }

  if (haudio->alt_setting == 1)
  {
    lockedCount++;

    if (first_sync_with_sai)
    {
      uint32_t sai_samples = USB_IN_GetSaiReadCycles(pdev, 0);
      if (sai_samples)
      {
        lockedCount = 0;
        first_sync_with_sai = 0;
        usb_cycles = 0;
      }
    }

    if (lockedCount >= (1000))    // 1 seconds
    {
      lockedCount = 0;

      uint32_t dma_cycles = USB_IN_GetSaiReadCycles(pdev, 0);    // Each cycle corresponds to 4ms (plus the drift)
      usb_cycles += 250;

      float32_t fract_cycles = ((dma_cycles * 48.0f) / usb_cycles) * 8192.0f;
      lockedSampling = (uint32_t) fract_cycles;

    }

    USBD_SendData(pdev, AUDIO_SYNCH_EP, (uint8_t*) &lockedSampling, 4);
  }

  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_IsoOutIncomplete
 *         handle data ISO OUT Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  if (epnum == AUDIO_OUT_EP)
  {
    USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, (uint8_t*) usb_buffer, EP_OUT_REQ_LEN);
  }

  return USBD_OK;
}

/**
 * @brief  USBD_AUDIO_IsoInIncomplete
 *         handle data ISO IN Incomplete event
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_IsoInIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USB_OTG_GlobalTypeDef *USBx_BASE = ((PCD_HandleTypeDef*) (pdev->pData))->Instance;

  switch (epnum)
  {
    case 1:
      if ((USBx_DEVICE->DSTS & (1 << 8)) == 0)
      {
        USBx_INEP(epnum)->DIEPCTL |= USB_OTG_DIEPCTL_SODDFRM;
      }
      else
      {
        USBx_INEP(epnum)->DIEPCTL |= USB_OTG_DIEPCTL_SD0PID_SEVNFRM;
      }
      USBx_INEP(epnum)->DIEPCTL |= (USB_OTG_DIEPCTL_CNAK | USB_OTG_DIEPCTL_EPENA);

      break;
  }

  return USBD_OK;
}


/**
 * @brief  Handle data OUT Stage
 * @param  pdev: device instance
 * @param  epnum: endpoint index
 * @retval status
 */
static uint8_t USBD_AUDIO_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  if (epnum == AUDIO_OUT_EP)
  {
    uint32_t rxBytes = USBD_GetRxCount(pdev, epnum);
    USB_IN_RxData(pdev, usb_buffer, rxBytes);

    USBD_LL_PrepareReceive(pdev, AUDIO_OUT_EP, (uint8_t*) usb_buffer, EP_OUT_REQ_LEN);
  }

  return USBD_OK;
}

/**
 * @brief  Handles the SET_CUR Audio control request.
 *
 * @param  pdev: instance
 * @param  req: setup class request
 */
static void AUDIO_REQ_SetCurrent(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_AUDIO_HandleTypeDef *haudio;
  haudio = (USBD_AUDIO_HandleTypeDef*) pdev->pAudioClassData;

  if (req->wLength)
  {
    haudio->control.cmd = AUDIO_REQ_SET_CUR;                    // Set the request value
    haudio->control.len = req->wLength;                         // Set the request data length
    haudio->control.unit = HIBYTE(req->wIndex);                 // Set the request target unit
    haudio->control.control_selector = HIBYTE(req->wValue);
    haudio->control.channel = LOBYTE(req->wValue);

    // Prepare the reception of the buffer over EP0
    USBD_CtlPrepareRx(pdev, haudio->control.data, req->wLength);
  }
}

/**
 * @brief  USBD_AUDIO_RegisterInterface
 * @param  fops: Audio interface callback
 * @retval status
 */
uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_AUDIO_ItfTypeDef *fops)
{
  if (fops != NULL)
  {
    pdev->pAudioUserData = fops;
  }
  return 0;
}

