/**
 * @file  bsp_adc_dma.h
 * @brief ADC DMA 采集及电池监测接口
 */
#ifndef BSP_ADC_DMA_H
#define BSP_ADC_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stm32g0xx_hal.h"
#include "adc.h"

/* Defines -------------------------------------------------------------------*/
#define BSP_RAM_RECE_SIZE       20
#define LED_GREEN_GPIO_Port     GPIOC
#define LED_GREEN_Pin           GPIO_PIN_6
#define LED_RED_GPIO_Port       GPIOC
#define LED_RED_Pin             GPIO_PIN_7

/* Exported variables --------------------------------------------------------*/
extern volatile uint16_t BSP_RAM_Rece[BSP_RAM_RECE_SIZE];

/* Exported functions --------------------------------------------------------*/
void    BSP_ADC_StartDMA(ADC_HandleTypeDef *hadc);
void    BSP_ADC_Process(void);
int8_t  Get_Temperature(float current_voltage);
uint8_t Get_Battery_Capacity(int8_t current_temp, float current_vol);
void    Task_Battery_Monitor(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ADC_DMA_H */
