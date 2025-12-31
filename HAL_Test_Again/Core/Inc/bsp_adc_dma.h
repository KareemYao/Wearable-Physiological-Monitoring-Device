/*
 * bsp_adc_dma.h
 * ADC DMA 接口头文件
 */
#ifndef BSP_ADC_DMA_H
#define BSP_ADC_DMA_H

#include "stdint.h"
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BSP_RAM_RECE_SIZE 20

extern volatile uint16_t BSP_RAM_Rece[BSP_RAM_RECE_SIZE];

void BSP_ADC_StartDMA(ADC_HandleTypeDef *hadc);
void BSP_ADC_ProcessAndPrint(UART_HandleTypeDef *huart, uint32_t timeout_ms);
float BSP_ADC_GetTemp(void);
float BSP_ADC_GetVolat(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_ADC_DMA_H */
