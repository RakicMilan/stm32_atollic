/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
#define DEBUG(...) do{\
/*Clear woking buffer here*/\
sendDebug(__VA_ARGS__);\
}while(0);

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
//color macros
#define DebugMoveCursor(x,y){DEBUG("\033[2%d;%df",x,y);}
#define DebugMoveCursorUp(x){DEBUG("\033[%dA",x);}
#define DebugMoveCursorDw(x){printf("\033[%dB",x);}
#define DebugSaveCursorPos(){DEBUG("\033[s");}
#define DebugRecalCursorPos(){DEBUG("\033[u");}

#define DebugSetBold() {DEBUG("\033[1m");}
#define DebugSetNormal() {DEBUG("\033[0m");}
#define DebugSetBlink() {DEBUG("\033[5m");}
#define DebugSetInverse() {DEBUG("\033[7m");}

#define DebugClearScreen() {DEBUG("\033[2J");}
#define DebugChangeColorToGREEN() {DEBUG("\033[0;32m");}
#define DebugChangeColorToRED() {DEBUG("\033[0;31m");}
#define DebugChangeColorToBLUE() {DEBUG("\033[0;34m"); }
#define DebugChangeColorToYELLOW() {DEBUG("\033[0;33m");}
#define DebugChangeColorToCYIAN() {DEBUG("\033[0;36m"); }
#define DebugChangeColorToWHITE() {DEBUG("\033[0;37m"); }
#define DebugChangeColorToPURPLE() {DEBUG("\033[0;35m");}
#define DebugChangeBackColorToBLACK() {DEBUG("\033[40m");}

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

extern void _Error_Handler(char *, int);
extern void sendDebug(char* format,...);
#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define USE_SPI1
//#define USE_SPI2

#define DEBUG_TX_Pin GPIO_PIN_2
#define DEBUG_TX_GPIO_Port GPIOA
#define DEBUG_RX_Pin GPIO_PIN_3
#define DEBUG_RX_GPIO_Port GPIOA

#ifdef USE_SPI1
#define NRF24_CS_Pin GPIO_PIN_4
#define NRF24_CS_GPIO_Port GPIOA
#define NRF24_SCK_Pin GPIO_PIN_5
#define NRF24_SCK_GPIO_Port GPIOA
#define NRF24_MISO_Pin GPIO_PIN_6
#define NRF24_MISO_GPIO_Port GPIOA
#define NRF24_MOSI_Pin GPIO_PIN_7
#define NRF24_MOSI_GPIO_Port GPIOA
#else
#define NRF24_CS_Pin GPIO_PIN_12
#define NRF24_CS_GPIO_Port GPIOB
#define NRF24_SCK_Pin GPIO_PIN_13
#define NRF24_SCK_GPIO_Port GPIOB
#define NRF24_MISO_Pin GPIO_PIN_14
#define NRF24_MISO_GPIO_Port GPIOB
#define NRF24_MOSI_Pin GPIO_PIN_15
#define NRF24_MOSI_GPIO_Port GPIOB
#endif

#define NRF24_INT_Pin GPIO_PIN_0
#define NRF24_INT_GPIO_Port GPIOB
#define NRF24_INT_EXTI_IRQn EXTI0_IRQn
#define NRF24_CE_Pin GPIO_PIN_1
#define NRF24_CE_GPIO_Port GPIOB

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
