/**
 ******************************************************************************
 * @file    usbd_audio.h
 * @author  MCD Application Team
 * @version V2.4.2
 * @date    11-December-2015
 * @brief   header file for the usbd_audio.c file.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USB_AUDIO_H
#define __USB_AUDIO_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include  "../../../Core/Inc/usbd_ioreq.h"
//#include "Audio.h"

/** @addtogroup STM32_USB_DEVICE_LIBRARY
 * @{
 */

/** @defgroup USBD_AUDIO
 * @brief This file is the Header file for usbd_audio.c
 * @{
 */

/** @defgroup USBD_AUDIO_Exported_Defines
 * @{
 */
#define AUDIO_OUT_EP                                  0x01
#define AUDIO_SYNCH_EP                                0x81
#define AUDIO_IN_EP                                   0x82
#define USB_AUDIO_CONFIG_DESC_SIZ                     (109 + 1 + 42)
#define AUDIO_INTERFACE_DESC_SIZE                     9
#define USB_AUDIO_DESC_SIZ                            0x09
#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE             0x09
#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE            0x07

#define AUDIO_DESCRIPTOR_TYPE                         0x21
#define USB_DEVICE_CLASS_AUDIO                        0x01
#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01
#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02
#define AUDIO_PROTOCOL_UNDEFINED                      0x00
#define AUDIO_STREAMING_GENERAL                       0x01
#define AUDIO_STREAMING_FORMAT_TYPE                   0x02

/* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25

/* Audio Control Interface Descriptor Subtypes */
#define AUDIO_CONTROL_HEADER                          0x01
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06

#define AUDIO_INPUT_TERMINAL_DESC_SIZE                0x0C
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               0x09
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           0x07

#define AUDIO_CONTROL_MUTE                            0x0001

#define AUDIO_FORMAT_TYPE_I                           0x01
#define AUDIO_FORMAT_TYPE_III                         0x03

#define AUDIO_ENDPOINT_GENERAL                        0x01

#define AUDIO_REQ_GET_CUR                             0x81
#define AUDIO_REQ_SET_CUR                             0x01
#define AUDIO_REQ_CUR                                 0x01
#define AUDIO_REQ_RANGE                               0x02

#define AUDIO_OUT_STREAMING_CTRL                      0x02
#define AUDIO_IN_STREAMING_CTRL                       0x12

#define AUDIO_OUT_ID                                  0x01
#define AUDIO_IN_ID                                   0x10


/* Number of audio bytes per stream per 1ms */
#define AUDIO_OUT_PACKET                              (uint32_t)((USBD_AUDIO_FREQ * AUDIO_STREAM_WIDTH) / 1000)

/* Number of audio bytes of all streams per 125us */
#define AUDIO_USB_OUT_PACKET                          (uint32_t)(BYTES_PER_USB_SAMPLE * SAMPLES_PER_USB_CHUNK)

/* Audio Commands enumeration */
typedef enum
{
  AUDIO_CMD_START = 1,
  AUDIO_CMD_PLAY,
  AUDIO_CMD_STOP,
  AUDIO_CMD_NONE = 0xFF,
} AUDIO_CMD_TypeDef;

typedef enum
{
  AUDIO_OFFSET_NONE = 0,
  AUDIO_OFFSET_HALF,
  AUDIO_OFFSET_FULL,
  AUDIO_OFFSET_UNKNOWN,
}
AUDIO_OffsetTypeDef;
/**
 * @}
 */

/** @defgroup USBD_CORE_Exported_TypesDefinitions
 * @{
 */
typedef struct
{
  uint8_t cmd;
  uint8_t data[USB_MAX_EP0_SIZE];
  uint8_t len;
  uint8_t unit;
  uint8_t control_selector;
  uint8_t channel;
}
USBD_AUDIO_ControlTypeDef;

typedef struct
{
  __IO uint32_t alt_setting;
  __IO uint32_t alt_mic_setting;
  uint32_t total_streams;
  void *pAudioStreamData;
  AUDIO_OffsetTypeDef offset;
  uint8_t rd_enable;
  uint16_t rd_ptr;
  uint16_t wr_ptr;
  USBD_AUDIO_ControlTypeDef control;
  uint8_t *buffer;
}
USBD_AUDIO_HandleTypeDef __attribute__ ((aligned (4)));

typedef struct
{
  void* (*Init)(uint32_t id, uint32_t AudioFreq, uint32_t Volume, uint32_t options);
  int8_t (*DeInit)(void *StreamState, uint32_t options);
  int8_t (*AudioCmd)(void *StreamState, uint8_t *pbuf, uint32_t size, uint8_t cmd);
  int8_t (*VolumeCtl)(void *StreamState, uint32_t channel, uint8_t vol);
  int8_t (*MuteCtl)(void *StreamState, uint32_t channel, uint8_t cmd);
  int8_t (*PeriodicTC)(uint8_t cmd);
  int8_t (*GetState)(void);
} USBD_AUDIO_ItfTypeDef;
/**
 * @}
 */

/** @defgroup USBD_CORE_Exported_Macros
 * @{
 */

/**
 * @}
 */

/** @defgroup USBD_CORE_Exported_Variables
 * @{
 */

extern USBD_ClassTypeDef USBD_AUDIO;
#define USBD_AUDIO_CLASS    &USBD_AUDIO
/**
 * @}
 */

/** @defgroup USB_CORE_Exported_Functions
 * @{
 */
uint8_t USBD_AUDIO_RegisterInterface(USBD_HandleTypeDef *pdev,
    USBD_AUDIO_ItfTypeDef *fops);

void USBD_AUDIO_Sync(USBD_HandleTypeDef *pdev, uint32_t id, AUDIO_OffsetTypeDef offset);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* __USB_AUDIO_H */
/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
