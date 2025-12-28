/*
 * bsp_tim.c
 * 定时器封装实现
 */

#include "bsp_tim.h"
#include "bsp_usart.h"
#include "bsp_soft_i2c.h"
#include <string.h>

/* 定时器中断回调函数 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim1)
    {
        uint8_t buffer[60] = {0};

        /* 读取 10 个样本，每个 6 字节 */
        for (uint8_t i = 0; i < 10; i++)
        {
            int ret = My_SI2C_ReadDatas(0x57, 0x07, fifo_buffer, 6);
            if (ret == 0)
            {
                memcpy(&buffer[6 * i], fifo_buffer, 6);
            }
        }

        /* 分包发送数据 */
        for (uint8_t k = 0; k < 10; k++)
        {
            Send_DataPacket(buffer, k);
        }
    }
}
