/*
 * bsp_tim.h
 * 定时器封装头文件
 */
#ifndef BSP_TIM_H
#define BSP_TIM_H

#include "stm32f4xx_hal.h"
#include "tim.h"

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif /* BSP_TIM_H */
