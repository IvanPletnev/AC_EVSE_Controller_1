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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sim7600.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */
extern volatile uint16_t secCounter;
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
#define RELAY1_Pin GPIO_PIN_0
#define RELAY1_GPIO_Port GPIOC
#define RELAY2_Pin GPIO_PIN_1
#define RELAY2_GPIO_Port GPIOC
#define PWRKEY_Pin GPIO_PIN_2
#define PWRKEY_GPIO_Port GPIOC
#define RXTX_Pin GPIO_PIN_3
#define RXTX_GPIO_Port GPIOC
#define PILOT_ADC1_Pin GPIO_PIN_0
#define PILOT_ADC1_GPIO_Port GPIOA
#define PILOT_ADC2_Pin GPIO_PIN_1
#define PILOT_ADC2_GPIO_Port GPIOA
#define PROX_ADC1_Pin GPIO_PIN_4
#define PROX_ADC1_GPIO_Port GPIOA
#define PROX_ADC2_Pin GPIO_PIN_5
#define PROX_ADC2_GPIO_Port GPIOA
#define L1_P_Pin GPIO_PIN_6
#define L1_P_GPIO_Port GPIOA
#define L2_P_Pin GPIO_PIN_7
#define L2_P_GPIO_Port GPIOA
#define L3_P_Pin GPIO_PIN_4
#define L3_P_GPIO_Port GPIOC
#define RGB_Pin GPIO_PIN_14
#define RGB_GPIO_Port GPIOB
#define SD_DET_Pin GPIO_PIN_15
#define SD_DET_GPIO_Port GPIOB
#define GFCI_Pin GPIO_PIN_9
#define GFCI_GPIO_Port GPIOA
#define PWM1_Pin GPIO_PIN_15
#define PWM1_GPIO_Port GPIOA
#define PWM2_Pin GPIO_PIN_3
#define PWM2_GPIO_Port GPIOB
#define STATUS_LED3_Pin GPIO_PIN_4
#define STATUS_LED3_GPIO_Port GPIOB
#define STATUS_LED2_Pin GPIO_PIN_5
#define STATUS_LED2_GPIO_Port GPIOB
#define STATUS_LED1_Pin GPIO_PIN_9
#define STATUS_LED1_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
