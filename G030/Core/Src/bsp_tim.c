/**
 * @file  bsp_tim.c
 * @brief 定时器封装实现
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_tim.h"

/* Private variables ---------------------------------------------------------*/
static TIM_HandleTypeDef *s_htim = NULL;
static uint8_t data_time_counter = 0;

/* Exported functions --------------------------------------------------------*/

/* 初始化定时器模块 */
void BSP_TIM_Init(TIM_HandleTypeDef *htim)
{
    s_htim = htim;
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

/* 定时器中断回调: 读取 FIFO 并处理数据 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == s_htim)
    {
        /* 读取 MAX30102 FIFO 数据并处理 */
        int ret = My_SI2C_ReadDatas(0x57, 0x07, fifo_buffer, 6);
        if (ret == 0)
        {
            MAX30102_ProcessData(fifo_buffer);
        }

        /* 定时发送数据包并启动 ADC (200 * 10ms = 2s) */
        data_time_counter++;
        if (data_time_counter >= 200)
        {
            data_time_counter = 0;
            Send_DataPacket();
            BSP_ADC_StartDMA(&hadc1);
        }
    }
}
