/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "adc.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"
#include <string.h>
#include "bsp_adc_dma.h"
#include "bsp_tim.h"
#include "bsp_soft_i2c.h"
#include "bsp_ir_red_cal2.h"
#include "bsp_usart_dma.h"
#include "NLsocket.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
volatile uint8_t adc_data_ready = 0;
uint8_t fifo_buffer[6] = {0};
uint8_t reg_commands[8] = {0x04, 0x05, 0x06, 0x08, 0x0A, 0x0C, 0x0D, 0x09};
uint8_t commands[8]     = {0x00, 0x00, 0x00, 0x00, 0x07, 0x18, 0x18, 0x03};
NLsocket nlsct = NL_SOCKET_INIT(&huart1);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  while(
    nlsct.Basic
      .Send_AT_cmd(nlsct.uart_handle, nlsct.AT_buffer, (const uint8_t*)"AT+RST\r\n",strlen("AT+RST\r\n"),WAIT_TIME_MS) != HAL_OK
  )  {
    nlsct.Basic
      .Send_AT_cmd(nlsct.uart_handle, nlsct.AT_buffer, (const uint8_t*)"+++",strlen("+++"),WAIT_TIME_MS) ;
  };
  while(
    nlsct.Basic.Send_AT_cmd(
      nlsct.uart_handle,
      nlsct.AT_buffer,
      (const uint8_t*)"AT+TRANSENTER=1,1\r\n",
      strlen("AT+TRANSENTER=1,1\r\n"),
      WAIT_TIME_MS
    ) && strstr((const char*)nlsct.AT_buffer,"OK") == NULL
  );
  while(
    nlsct.Basic.Send_AT_cmd(nlsct.uart_handle,nlsct.AT_buffer,(const uint8_t*)"AT+SLEMODE?\r\n",strlen("AT+SLEMODE?\r\n"),WAIT_TIME_MS) != HAL_OK
    && strstr((const char*)nlsct.AT_buffer,"+SLEMODE:1") == NULL
  ){
    nlsct.Basic.Send_AT_cmd(nlsct.uart_handle,nlsct.AT_buffer,(const uint8_t*)"AT+SLEMODE=1\r\n",strlen("AT+SLEMODE=1\r\n"),WAIT_TIME_MS);
  };
  while(
    nlsct.Basic.Send_AT_cmd(
      nlsct.uart_handle,
      nlsct.AT_buffer,
      (const uint8_t*)"AT+SLECONNECT=94c960f03832\r\n",
      strlen("AT+SLECONNECT=94c960f03832\r\n"),
      WAIT_TIME_MS
    ) != HAL_OK
    && strstr((const char*)nlsct.AT_buffer,"OK") == NULL
  );
  HR_Device_Init(0x57, reg_commands, commands, 8);
  BSP_TIM_Init(&htim1);
  BSP_TIM_Start_IT();
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if (adc_data_ready == 1)
    {
      Task_Battery_Monitor();
      adc_data_ready = 0;
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        HAL_ADC_Stop_DMA(hadc);
        adc_data_ready = 1;
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
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
