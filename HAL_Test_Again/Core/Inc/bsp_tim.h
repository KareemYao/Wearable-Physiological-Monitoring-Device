/*
 * bsp_tim.h
 * 定时器封装头文件
 */
#ifndef BSP_TIM_H
#define BSP_TIM_H

#include "stdint.h"
#include "stm32f4xx_hal.h"
#include "bsp_soft_i2c.h"
#include "bsp_ir_red_cal2.h"
#include "bsp_usart_dma.h"
#include "tim.h"

#ifdef __cplusplus
extern "C" {
#endif

void BSP_TIM_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef BSP_TIM_Start_IT(void);
//uint32_t BSP_TIM_GetTick(void);
//void BSP_TIM_Delay(uint32_t DelayMs);

#ifdef __cplusplus
}
#endif

#endif /* BSP_TIM_H */
