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
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "usart.h"

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

extern uint8_t terminal_buf[];
extern uint8_t communication_buf[];
extern uint8_t uart1_irq_sel;
extern uint8_t print_trans_data;
extern uint32_t tim_counter_max;
extern uint32_t tim2_counter;

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
uint32_t send_cmd(uint8_t ch, char *arg);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define IN_ALARM_Pin GPIO_PIN_1
#define IN_ALARM_GPIO_Port GPIOI
#define Ready_Pin GPIO_PIN_14
#define Ready_GPIO_Port GPIOH
#define HARD_RESET_Pin GPIO_PIN_0
#define HARD_RESET_GPIO_Port GPIOI
#define SPI5_CS_Pin GPIO_PIN_6
#define SPI5_CS_GPIO_Port GPIOF
#define Tx_Modulation_Pin GPIO_PIN_6
#define Tx_Modulation_GPIO_Port GPIOH
#define MOD_ABS_Pin GPIO_PIN_8
#define MOD_ABS_GPIO_Port GPIOH
#define SPI1_CS_Pin GPIO_PIN_4
#define SPI1_CS_GPIO_Port GPIOA
#define BUTTON_N_Pin GPIO_PIN_7
#define BUTTON_N_GPIO_Port GPIOH
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
