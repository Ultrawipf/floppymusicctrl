/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ADR_IN_Pin GPIO_PIN_7
#define ADR_IN_GPIO_Port GPIOB
#define ADR_IN_EXTI_IRQn EXTI4_15_IRQn
#define FDD_DIN_Pin GPIO_PIN_9
#define FDD_DIN_GPIO_Port GPIOB
#define FDD_TRK0_Pin GPIO_PIN_15
#define FDD_TRK0_GPIO_Port GPIOC
#define FDD_TRK0_EXTI_IRQn EXTI4_15_IRQn
#define FDD_WEN_Pin GPIO_PIN_0
#define FDD_WEN_GPIO_Port GPIOA
#define FDD_DOUT_Pin GPIO_PIN_1
#define FDD_DOUT_GPIO_Port GPIOA
#define FDD_MOT_Pin GPIO_PIN_2
#define FDD_MOT_GPIO_Port GPIOA
#define FDD_HEAD_Pin GPIO_PIN_3
#define FDD_HEAD_GPIO_Port GPIOA
#define FDD_DIR_Pin GPIO_PIN_6
#define FDD_DIR_GPIO_Port GPIOA
#define ENABLE_Pin GPIO_PIN_7
#define ENABLE_GPIO_Port GPIOA
#define FDD_STEP_Pin GPIO_PIN_8
#define FDD_STEP_GPIO_Port GPIOA
#define EXTCLK_Pin GPIO_PIN_12
#define EXTCLK_GPIO_Port GPIOA
#define SWD_IDX_Pin GPIO_PIN_13
#define SWD_IDX_GPIO_Port GPIOA
#define ADR_OUT_Pin GPIO_PIN_6
#define ADR_OUT_GPIO_Port GPIOB
void   MX_TIM1_Init(void);
void   MX_TIM3_Init(void);
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
