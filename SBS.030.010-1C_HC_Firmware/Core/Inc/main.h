/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_BlackPill_Pin GPIO_PIN_13
#define LED_BlackPill_GPIO_Port GPIOC
#define VUpstream_Anlg_Pin GPIO_PIN_0
#define VUpstream_Anlg_GPIO_Port GPIOA
#define LTC3901_Vcc_Anlg_Pin GPIO_PIN_1
#define LTC3901_Vcc_Anlg_GPIO_Port GPIOA
#define LT8316_Vout_Anlg_Pin GPIO_PIN_2
#define LT8316_Vout_Anlg_GPIO_Port GPIOA
#define LTC3901_ME_Anlg_Pin GPIO_PIN_3
#define LTC3901_ME_Anlg_GPIO_Port GPIOA
#define LTC3901_MF_Anlg_Pin GPIO_PIN_4
#define LTC3901_MF_Anlg_GPIO_Port GPIOA
#define LT8316_Gate_Anlg_Pin GPIO_PIN_5
#define LT8316_Gate_Anlg_GPIO_Port GPIOA
#define ID5_Pin GPIO_PIN_6
#define ID5_GPIO_Port GPIOA
#define ID4_Pin GPIO_PIN_7
#define ID4_GPIO_Port GPIOA
#define ID3_Pin GPIO_PIN_0
#define ID3_GPIO_Port GPIOB
#define ID2_Pin GPIO_PIN_1
#define ID2_GPIO_Port GPIOB
#define ID1_Pin GPIO_PIN_2
#define ID1_GPIO_Port GPIOB
#define ID10_Pin GPIO_PIN_10
#define ID10_GPIO_Port GPIOB
#define BeamOn_Pin GPIO_PIN_12
#define BeamOn_GPIO_Port GPIOB
#define LED_G_Pin GPIO_PIN_13
#define LED_G_GPIO_Port GPIOB
#define LED_R_Pin GPIO_PIN_14
#define LED_R_GPIO_Port GPIOB
#define nLTC3901_Pwr_Enable_Pin GPIO_PIN_15
#define nLTC3901_Pwr_Enable_GPIO_Port GPIOB
#define LT8316_HV_Pwr_Enable_Pin GPIO_PIN_8
#define LT8316_HV_Pwr_Enable_GPIO_Port GPIOA
#define SDRA_DRV_Pin GPIO_PIN_4
#define SDRA_DRV_GPIO_Port GPIOB
#define SDRB_DRV_Pin GPIO_PIN_5
#define SDRB_DRV_GPIO_Port GPIOB
#define LTC3901_ME_Timer_Pin GPIO_PIN_6
#define LTC3901_ME_Timer_GPIO_Port GPIOB
#define LTC3901_MF_Timer_Pin GPIO_PIN_7
#define LTC3901_MF_Timer_GPIO_Port GPIOB
#define LT8316_Gate_Timer_Pin GPIO_PIN_8
#define LT8316_Gate_Timer_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
