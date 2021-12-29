/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "stm32f1xx_it.h"
#include "gpio.h"
#include "usart.h"
#include "i2c.h"
#include "spi.h"

#include "fonts.h"
#include "ssd1306.h"
#include "ds1820.h"
#include "nrf24.h"

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#define RX
//#define TX

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
uint8_t m_displayCounter;

nrf24_t NRF24Ctx = {
#ifdef RX
		.RXAddress = {0xD7,0xD7,0xD7,0xD7,0xD7},
		.TXAddress = {0xE7,0xE7,0xE7,0xE7,0xE7},
#endif
#ifdef TX
		.RXAddress = {0xE7,0xE7,0xE7,0xE7,0xE7},
		.TXAddress = {0xD7,0xD7,0xD7,0xD7,0xD7},
#endif
		.PayloadSize = 8,
		.Channel = 2
};

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void UpdateDisplay(void);

/* Private user code ---------------------------------------------------------*/
void sendDebug(char* format,...)
{
  char sendBuf[128];
  va_list args;
  /* initialize valist for num number of arguments */
  va_start(args, format);
  vsnprintf(sendBuf,sizeof(sendBuf) - 1, format, args);
  /* clean memory reserved for valist */
  va_end(args);
  USARTWriteBuffer(USART2,(uint8_t*)sendBuf,strlen(sendBuf));
}

void nRF24_DumpConfig(void) {
  uint8_t i, j;
  uint8_t aw;

  // Dump nRF24L01+ configuration
  // CONFIG
  NRF24ReadRegister(0x00,&i,1);
  DEBUG("[0x%02X] 0x%02X MASK:0x%02X CRC:0x%02X PWR:%s MODE:P%s\r\n", 0x00, i,
          i >> 4, (i & 0x0c) >> 2, (i & 0x02) ? "ON" : "OFF",
          (i & 0x01) ? "RX" : "TX");
  // EN_AA
  NRF24ReadRegister(0x01,&i,1);
  DEBUG("[0x%02X] 0x%02X ENAA: ", 0x01, i);
  for (j = 0; j < 6; j++) {
      DEBUG("[P%1u%s]%s", j, (i & (1 << j)) ? "+" : "-",
              (j == 5) ? "\r\n" : " ");
  }
  // EN_RXADDR
  NRF24ReadRegister(0x02,&i,1);
  DEBUG("[0x%02X] 0x%02X EN_RXADDR: ", 0x02, i);
  for (j = 0; j < 6; j++) {
      DEBUG("[P%1u%s]%s", j, (i & (1 << j)) ? "+" : "-",
              (j == 5) ? "\r\n" : " ");
  }
  // SETUP_AW
  NRF24ReadRegister(0x03,&i,1);
  aw = (i & 0x03) + 2;
  DEBUG("[0x%02X] 0x%02X EN_RXADDR=0x%02X (address width = %d)\r\n", 0x03, i,
          i & 0x03, aw);
  // SETUP_RETR
  NRF24ReadRegister(0x04,&i,1);
  DEBUG("[0x%02X] 0x%02X ARD=0x%02X ARC=0x%02X (retr.delay=%d[us], count=%d)\r\n",
          0x04, i, i >> 4, i & 0x0F, ((i >> 4) * 250) + 250, i & 0x0F);
  // RF_CH
  NRF24ReadRegister(0x05,&i,1);
  DEBUG("[0x%02X] 0x%02X %d[GHz]\r\n", 0x05, i, 2400 + i);
  // RF_SETUP
  NRF24ReadRegister(0x06,&i,1);
  DEBUG("[0x%02X] 0x%02X CONT_WAVE:%s PLL_LOCK:%s DataRate=", 0x06, i,
          (i & 0x80) ? "ON" : "OFF", (i & 0x80) ? "ON" : "OFF");
  switch ((i & 0x28) >> 3) {
  case 0x00:
      DEBUG("1M");
      break;
  case 0x01:
      DEBUG("2M");
      break;
  case 0x04:
      DEBUG("250k");
      break;
  default:
      DEBUG("???");
      break;
  }
  DEBUG("pbs RF_PWR=");
  switch ((i & 0x06) >> 1) {
  case 0x00:
      DEBUG("-18");
      break;
  case 0x01:
      DEBUG("-12");
      break;
  case 0x02:
      DEBUG("-6");
      break;
  case 0x03:
      DEBUG("0");
      break;
  default:
      DEBUG("???");
      break;
  }
  DEBUG("dBm\r\n");
  // STATUS
  NRF24ReadRegister(0x07,&i,1);
  DEBUG("[0x%02X] 0x%02X IRQ:0x%02X RX_PIPE:%d TX_FULL:%s\r\n", 0x07, i,
          (i & 0x70) >> 4, (i & 0x0E) >> 1, (i & 0x01) ? "YES" : "NO");
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
#ifdef USE_SPI1
  MX_SPI1_Init();
#else
  MX_SPI2_Init();
#endif
  DS1820_Init();
  SSD1306_Init();

  DEBUG("NRF24L01 start!\r\n");
  /* Set the device addresses */
  NRF24Init();

  // ---TEST--- //
  //nRF24_DumpConfig();

  NRF24Config(&NRF24Ctx);

  NRF24SetRxAddress(&NRF24Ctx);
  NRF24SetTxAddress(&NRF24Ctx);

  SSD1306_GotoXY(0, 0);
  SSD1306_Puts("HELLO", &Font_11x18, 1);
  SSD1306_GotoXY(10, 30);
  SSD1306_Puts("  WORLD :)", &Font_11x18, 1);
  SSD1306_UpdateScreen();

  /* Infinite loop */
  while (1)
  {
#if defined RX
    if(NRF24DataReady())
    {
      HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      NRF24GetData(&NRF24Ctx);
      DEBUG("Received data:\r\n");
      for(uint8_t i = 0; i < NRF24Ctx.PayloadSize; i++)
      {
          DEBUG("%2X ",NRF24Ctx.RXData[i]);
      }
      DEBUG("\r\n");
    }
#endif

	/*Task 10ms*/
    if(Task10ms)
    {
      Task10ms = false;
    }
    /*Task 100ms*/
    if(Task100ms)
    {
      Task100ms = false;

      MeasureTemperatures();

    }
    /*Task 1 second*/
    if(Task1s)
    {
      Task1s = false;
#if defined TX
      /* Prepare data for sending */
      memset(NRF24Ctx.TXData, 0, sizeof(NRF24Ctx.TXData));
      for(uint8_t i = 0; i < NRF24Ctx.PayloadSize; i++)
      {
          NRF24Ctx.TXData[i] = i;
      }
      /* Automatically goes to TX mode */
      NRF24Send(&NRF24Ctx);

      /* Wait for transmission to end */
      while(NRF24IsSending());

      /* Make analysis on last tranmission attempt */
      uint8_t sendStatus = NRF24LastMessageStatus();

      if(sendStatus == NRF24_TRANSMISSON_OK)
      {
          DEBUG("Transmition OK!\r\n");
          HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
      }
      else if(sendStatus == NRF24_MESSAGE_LOST)
      {
          DEBUG("Transmition ERROR!\r\n");
      }
      /* Retranmission count indicates the tranmission quality */
      sendStatus = NRF24RetransmissionCount();
      DEBUG("Retransmition count: %u!\r\n",sendStatus);
#endif

      UpdateDisplay();
    }
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  /* Configure the Systick interrupt time */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  /* Configure the Systick */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

void UpdateDisplay(void) {
//	if (m_displayCounter < 3) {
//		DisplayTime();
//	} else {
		DisplayTemperatures();
//	}
	m_displayCounter++;
	if (m_displayCounter >= 6)
		m_displayCounter = 0;
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

