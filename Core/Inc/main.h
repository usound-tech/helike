/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
#define DMA_MEMORY_BASE       ((uint8_t*)0x24000000)            //!< This memory region has been configured as uncacheable in MPU
#define DMA_MEMORY_MPU_CONF   MPU_REGION_SIZE_128KB             //!< The uncacheable memory size (MPU configuration)
#define DMA_MEMORY_SIZE       (1 << (DMA_MEMORY_MPU_CONF + 1))  //!< The size of the uncacheable memory area in bytes
/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
#define MAJOR_REV             3
#define MINOR_REV             3
#define SUB_MINOR_REV         0
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED1_Pin GPIO_PIN_2
#define LED1_GPIO_Port GPIOE
#define LED2_Pin GPIO_PIN_3
#define LED2_GPIO_Port GPIOE
#define LED3_Pin GPIO_PIN_4
#define LED3_GPIO_Port GPIOE
#define LED4_Pin GPIO_PIN_5
#define LED4_GPIO_Port GPIOE
#define TP11_Pin GPIO_PIN_6
#define TP11_GPIO_Port GPIOE
#define SPARE4_Pin GPIO_PIN_13
#define SPARE4_GPIO_Port GPIOC
#define TP12_Pin GPIO_PIN_14
#define TP12_GPIO_Port GPIOC
#define TP13_Pin GPIO_PIN_15
#define TP13_GPIO_Port GPIOC
#define DIPSWITCH_CFG4_Pin GPIO_PIN_2
#define DIPSWITCH_CFG4_GPIO_Port GPIOC
#define DIPSWITCH_CFG3_Pin GPIO_PIN_3
#define DIPSWITCH_CFG3_GPIO_Port GPIOC
#define nAMP2_Powerdown_Pin GPIO_PIN_4
#define nAMP2_Powerdown_GPIO_Port GPIOC
#define nAMP1_Powerdown_Pin GPIO_PIN_5
#define nAMP1_Powerdown_GPIO_Port GPIOC
#define nDAC1_Powerdown_Pin GPIO_PIN_0
#define nDAC1_Powerdown_GPIO_Port GPIOB
#define nDAC2_Powerdown_Pin GPIO_PIN_1
#define nDAC2_Powerdown_GPIO_Port GPIOB
#define SPARE3_Pin GPIO_PIN_2
#define SPARE3_GPIO_Port GPIOB
#define JOYSTICK_NORTH_Pin GPIO_PIN_7
#define JOYSTICK_NORTH_GPIO_Port GPIOE
#define JOYSTICK_NORTH_EXTI_IRQn EXTI9_5_IRQn
#define JOYSTICK_SOUTH_Pin GPIO_PIN_8
#define JOYSTICK_SOUTH_GPIO_Port GPIOE
#define JOYSTICK_SOUTH_EXTI_IRQn EXTI9_5_IRQn
#define JOYSTICK_WEST_Pin GPIO_PIN_9
#define JOYSTICK_WEST_GPIO_Port GPIOE
#define JOYSTICK_WEST_EXTI_IRQn EXTI9_5_IRQn
#define JOYSTICK_EAST_Pin GPIO_PIN_10
#define JOYSTICK_EAST_GPIO_Port GPIOE
#define JOYSTICK_EAST_EXTI_IRQn EXTI15_10_IRQn
#define JOYSTICK_CENTER_Pin GPIO_PIN_11
#define JOYSTICK_CENTER_GPIO_Port GPIOE
#define JOYSTICK_CENTER_EXTI_IRQn EXTI15_10_IRQn
#define DIPSWITCH1_Pin GPIO_PIN_12
#define DIPSWITCH1_GPIO_Port GPIOE
#define DIPSWITCH2_Pin GPIO_PIN_13
#define DIPSWITCH2_GPIO_Port GPIOE
#define DIPSWITCH3_Pin GPIO_PIN_14
#define DIPSWITCH3_GPIO_Port GPIOE
#define DIPSWITCH4_Pin GPIO_PIN_15
#define DIPSWITCH4_GPIO_Port GPIOE
#define DIPSWITCH_CFG1_Pin GPIO_PIN_14
#define DIPSWITCH_CFG1_GPIO_Port GPIOD
#define SPARE5_Pin GPIO_PIN_6
#define SPARE5_GPIO_Port GPIOC
#define TP29_Pin GPIO_PIN_7
#define TP29_GPIO_Port GPIOC
#define TP28_Pin GPIO_PIN_8
#define TP28_GPIO_Port GPIOC
#define TP27_Pin GPIO_PIN_9
#define TP27_GPIO_Port GPIOC
#define SPARE2_Pin GPIO_PIN_8
#define SPARE2_GPIO_Port GPIOA
#define SPARE1_Pin GPIO_PIN_10
#define SPARE1_GPIO_Port GPIOA
#define TP10_Pin GPIO_PIN_15
#define TP10_GPIO_Port GPIOA
#define TP9_Pin GPIO_PIN_10
#define TP9_GPIO_Port GPIOC
#define DIPSWITCH_CFG2_Pin GPIO_PIN_11
#define DIPSWITCH_CFG2_GPIO_Port GPIOC
#define TP8_Pin GPIO_PIN_12
#define TP8_GPIO_Port GPIOC
#define TP7_Pin GPIO_PIN_2
#define TP7_GPIO_Port GPIOD
#define TP6_Pin GPIO_PIN_3
#define TP6_GPIO_Port GPIOD
#define TP5_Pin GPIO_PIN_6
#define TP5_GPIO_Port GPIOD
#define TP4_Pin GPIO_PIN_5
#define TP4_GPIO_Port GPIOB
#define TP3_Pin GPIO_PIN_8
#define TP3_GPIO_Port GPIOB
#define TP2_Pin GPIO_PIN_9
#define TP2_GPIO_Port GPIOB
#define TP1_Pin GPIO_PIN_1
#define TP1_GPIO_Port GPIOE
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
