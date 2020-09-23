/**
 ******************************************************************************
 * File Name          : SAI.c
 * Description        : This file provides code for the configuration
 *                      of the SAI instances.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "sai.h"

#include "gpio.h"
#include "dma.h"

/* USER CODE BEGIN 0 */
HAL_StatusTypeDef MX_SAI3_DeinitBlockA(void)
{
  return HAL_SAI_DeInit(&hsai_BlockA3);
}

HAL_StatusTypeDef MX_SAI3_DeinitBlockB(void)
{
  return HAL_SAI_DeInit(&hsai_BlockB3);
}

HAL_StatusTypeDef MX_SAI2_DeinitBlockA(void)
{
  return HAL_SAI_DeInit(&hsai_BlockA2);
}
/* USER CODE END 0 */

SAI_HandleTypeDef hsai_BlockA2;
SAI_HandleTypeDef hsai_BlockA3;
SAI_HandleTypeDef hsai_BlockB3;
DMA_HandleTypeDef hdma_sai2_a;
DMA_HandleTypeDef hdma_sai3_a;
DMA_HandleTypeDef hdma_sai3_b;

static void configureSaiMode(SAI_HandleTypeDef *handle, int saiMode)
{
  handle->Init.Protocol = SAI_FREE_PROTOCOL;
  handle->Init.DataSize = SAI_DATASIZE_16;
  handle->Init.FirstBit = SAI_FIRSTBIT_MSB;
  handle->Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
  handle->Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  handle->Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
  handle->Init.MonoStereoMode = SAI_STEREOMODE;
  handle->Init.CompandingMode = SAI_NOCOMPANDING;
  handle->Init.TriState = SAI_OUTPUT_NOTRELEASED;
  handle->Init.PdmInit.Activation = DISABLE;
  handle->Init.PdmInit.MicPairsNbr = 0;
  handle->Init.PdmInit.ClockEnable = 0;

  handle->FrameInit.FrameLength = 32;
  handle->FrameInit.ActiveFrameLength = 16;
  handle->FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
  handle->FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
  handle->FrameInit.FSOffset = SAI_FS_FIRSTBIT;
  handle->SlotInit.FirstBitOffset = 0;
  handle->SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
  handle->SlotInit.SlotNumber = 2;
  handle->SlotInit.SlotActive = 0x03;

  switch (saiMode)
  {
    case 0: //SAI_MODE_TX_MASTER,
      handle->Init.AudioMode = SAI_MODEMASTER_TX;
      handle->Init.Synchro = SAI_ASYNCHRONOUS;
      handle->Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
      handle->Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
      handle->Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
      handle->Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
      handle->Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
      handle->Init.Mckdiv = 0;
      handle->Init.MckOverSampling = SAI_MCK_OVERSAMPLING_DISABLE;
      break;

    case 1: //SAI_MODE_TX_SLAVE_INTERNAL,
      handle->Init.AudioMode = SAI_MODESLAVE_TX;
      handle->Init.Synchro = SAI_SYNCHRONOUS;
      handle->Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
      handle->Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
      break;

    case 2: //SAI_MODE_TX_SLAVE_EXTERNAL,
      handle->Init.AudioMode = SAI_MODESLAVE_TX;
      handle->Init.Synchro = SAI_ASYNCHRONOUS;
      handle->Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
      handle->Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY;
      break;

    case 3: //SAI_MODE_RX_SLAVE_INTERNAL,
      handle->Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;
      handle->Init.AudioMode = SAI_MODESLAVE_RX;
      handle->Init.Synchro = SAI_SYNCHRONOUS;
      handle->Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
      handle->Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
      handle->FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;
      break;

    default:
      case 4: //SAI_MODE_RX_SLAVE_EXTERNAL
      handle->Init.ClockStrobing = SAI_CLOCKSTROBING_RISINGEDGE;
      handle->Init.AudioMode = SAI_MODESLAVE_RX;
      handle->Init.Synchro = SAI_ASYNCHRONOUS;
      handle->Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
      handle->Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
      handle->FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;
      break;
  }
}

/* SAI2 init function */
HAL_StatusTypeDef MX_SAI2_InitBlockA(int saiMode)
{
  hsai_BlockA2.Instance = SAI2_Block_A;
  configureSaiMode(&hsai_BlockA2, saiMode);

  return HAL_SAI_Init(&hsai_BlockA2);
}

/* SAI3 init function */
HAL_StatusTypeDef MX_SAI3_InitBlockA(int saiMode)
{
  hsai_BlockA3.Instance = SAI3_Block_A;
  configureSaiMode(&hsai_BlockA3, saiMode);

  return HAL_SAI_Init(&hsai_BlockA3);
}

HAL_StatusTypeDef MX_SAI3_InitBlockB(int saiMode)
{
  hsai_BlockB3.Instance = SAI3_Block_B;
  configureSaiMode(&hsai_BlockB3, saiMode);

  return HAL_SAI_Init(&hsai_BlockB3);
}
static uint32_t SAI2_client = 0;
static uint32_t SAI3_client = 0;

void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  /* SAI2 */
  if (hsai->Instance == SAI2_Block_A)
  {
    /* SAI2 clock enable */
    if (SAI2_client == 0)
    {
      __HAL_RCC_SAI2_CLK_ENABLE();
    }
    SAI2_client++;

    /**SAI2_A_Block_A GPIO Configuration
     PD11     ------> SAI2_SD_A
     PD12     ------> SAI2_FS_A
     PD13     ------> SAI2_SCK_A
     */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF10_SAI2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Peripheral DMA init*/

    hdma_sai2_a.Instance = DMA1_Stream6;
    hdma_sai2_a.Init.Request = DMA_REQUEST_SAI2_A;
    hdma_sai2_a.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_sai2_a.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sai2_a.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sai2_a.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_sai2_a.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_sai2_a.Init.Mode = DMA_CIRCULAR;
    hdma_sai2_a.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sai2_a.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_sai2_a) != HAL_OK)
    {
      Error_Handler();
    }

    /* Several peripheral DMA handle pointers point to the same DMA handle.
     Be aware that there is only one channel to perform all the requested DMAs. */
    __HAL_LINKDMA(hsai, hdmarx, hdma_sai2_a);
    __HAL_LINKDMA(hsai, hdmatx, hdma_sai2_a);
  }
  /* SAI3 */
  if (hsai->Instance == SAI3_Block_A)
  {
    /* SAI3 clock enable */
    if (SAI3_client == 0)
    {
      __HAL_RCC_SAI3_CLK_ENABLE();
    }
    SAI3_client++;

    /**SAI3_A_Block_A GPIO Configuration
     PD15     ------> SAI3_MCLK_A
     PD0     ------> SAI3_SCK_A
     PD1     ------> SAI3_SD_A
     PD4     ------> SAI3_FS_A
     */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SAI3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_15;
    GPIO_InitStruct.Mode = (hsai->Init.MckOutput == SAI_MCK_OUTPUT_ENABLE) ? GPIO_MODE_AF_PP : GPIO_MODE_INPUT;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Peripheral DMA init*/

    hdma_sai3_a.Instance = DMA1_Stream2;
    hdma_sai3_a.Init.Request = DMA_REQUEST_SAI3_A;
    hdma_sai3_a.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_sai3_a.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sai3_a.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sai3_a.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_sai3_a.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_sai3_a.Init.Mode = DMA_CIRCULAR;
    hdma_sai3_a.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sai3_a.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_sai3_a) != HAL_OK)
    {
      Error_Handler();
    }

    /* Several peripheral DMA handle pointers point to the same DMA handle.
     Be aware that there is only one channel to perform all the requested DMAs. */
    __HAL_LINKDMA(hsai, hdmarx, hdma_sai3_a);
    __HAL_LINKDMA(hsai, hdmatx, hdma_sai3_a);
  }
  if (hsai->Instance == SAI3_Block_B)
  {
    /* SAI3 clock enable */
    if (SAI3_client == 0)
    {
      __HAL_RCC_SAI3_CLK_ENABLE();
    }
    SAI3_client++;

    /**SAI3_B_Block_B GPIO Configuration
     PD9     ------> SAI3_SD_B
     */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SAI3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* Peripheral DMA init*/

    hdma_sai3_b.Instance = DMA1_Stream3;
    hdma_sai3_b.Init.Request = DMA_REQUEST_SAI3_B;
    hdma_sai3_b.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_sai3_b.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_sai3_b.Init.MemInc = DMA_MINC_ENABLE;
    hdma_sai3_b.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_sai3_b.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_sai3_b.Init.Mode = DMA_CIRCULAR;
    hdma_sai3_b.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_sai3_b.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_sai3_b) != HAL_OK)
    {
      Error_Handler();
    }

    /* Several peripheral DMA handle pointers point to the same DMA handle.
     Be aware that there is only one channel to perform all the requested DMAs. */
    __HAL_LINKDMA(hsai, hdmarx, hdma_sai3_b);
    __HAL_LINKDMA(hsai, hdmatx, hdma_sai3_b);
  }
}

void HAL_SAI_MspDeInit(SAI_HandleTypeDef *hsai)
{

  /* SAI2 */
  if (hsai->Instance == SAI2_Block_A)
  {
    SAI2_client--;
    if (SAI2_client == 0)
    {
      /* Peripheral clock disable */
      __HAL_RCC_SAI2_CLK_DISABLE();
    }

    /**SAI2_A_Block_A GPIO Configuration
     PD11     ------> SAI2_SD_A
     PD12     ------> SAI2_FS_A
     PD13     ------> SAI2_SCK_A
     */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);

    HAL_DMA_DeInit(hsai->hdmarx);
    HAL_DMA_DeInit(hsai->hdmatx);
  }
  /* SAI3 */
  if (hsai->Instance == SAI3_Block_A)
  {
    SAI3_client--;
    if (SAI3_client == 0)
    {
      /* Peripheral clock disable */
      __HAL_RCC_SAI3_CLK_DISABLE();
    }

    /**SAI3_A_Block_A GPIO Configuration
     PD15     ------> SAI3_MCLK_A
     PD0     ------> SAI3_SCK_A
     PD1     ------> SAI3_SD_A
     PD4     ------> SAI3_FS_A
     */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_15 | GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4);

    HAL_DMA_DeInit(hsai->hdmarx);
    HAL_DMA_DeInit(hsai->hdmatx);
  }
  if (hsai->Instance == SAI3_Block_B)
  {
    SAI3_client--;
    if (SAI3_client == 0)
    {
      /* Peripheral clock disable */
      __HAL_RCC_SAI3_CLK_DISABLE();
    }

    /**SAI3_B_Block_B GPIO Configuration
     PD9     ------> SAI3_SD_B
     */
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_9);

    HAL_DMA_DeInit(hsai->hdmarx);
    HAL_DMA_DeInit(hsai->hdmatx);
  }
}

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
