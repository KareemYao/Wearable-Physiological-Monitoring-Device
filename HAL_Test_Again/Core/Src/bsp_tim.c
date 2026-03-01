/*
 * bsp_tim.c
 * 定时器封装实现
 */
#include "bsp_tim.h"

static TIM_HandleTypeDef *s_htim = NULL;
//static volatile uint32_t s_millis = 0;
static uint16_t tim_counter = 0;

/* 初始化定时器模块 */
void BSP_TIM_Init(TIM_HandleTypeDef *htim)
{
    s_htim = htim;
//    s_millis = 0;
}

/* 启动定时器中断 */
HAL_StatusTypeDef BSP_TIM_Start_IT(void)
{
    if (s_htim == NULL)
    {
        return HAL_ERROR;
    }
    return HAL_TIM_Base_Start_IT(s_htim);
}

// /* 获取当前毫秒数 */
// uint32_t BSP_TIM_GetTick(void)
// {
//     return s_millis;
// }

// /* 毫秒级延时 */
// void BSP_TIM_Delay(uint32_t DelayMs)
// {
//     uint32_t start = BSP_TIM_GetTick();
//     while ((uint32_t)(BSP_TIM_GetTick() - start) < DelayMs)
//     {
//     }
// }

/* 定时器中断回调 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == s_htim)
    {
        // s_millis++;
        int ret = My_SI2C_ReadDatas(0x57, 0x07, fifo_buffer, 6);
        if (ret == 0) {
            MAX30102_ProcessData(fifo_buffer); 
        }

        /* 定时发送数据包 (200 * 10ms = 2s) */
        tim_counter++;
        if(tim_counter >= 200)
        {
            tim_counter = 0;
            Send_DataPacket();
        }
    }
}
