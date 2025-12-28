/*
 * bsp_usart.c
 * 串口封装实现
 */

#include "bsp_usart.h"
#include "bsp_tim.h"

/* 发送完成标志位 */
volatile uint8_t tx_complete = 1;

/* 数据包缓存 */
static uint8_t packet[10];

/* 串口 DMA 发送 */
void UART_Transmit_DMA(char* buf)
{
    if (tx_complete && huart1.gState == HAL_UART_STATE_READY)
    {
        tx_complete = 0;
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)buf, 10);
    }
}

/* 串口发送完成回调函数 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1)
    {
        tx_complete = 1;
    }
}

/* 发送数据包 */
void Send_DataPacket(uint8_t *buffer, uint8_t k)
{
    packet[0] = 0xAA;
    packet[1] = 0x55;

    /* 根据样本索引 k，取 buffer 中第 k 个 6 字节样本 */
	  //红光
    packet[2] = buffer[6 * k + 0];
    packet[3] = buffer[6 * k + 1];
    packet[4] = buffer[6 * k + 2];
	  //红外光
    packet[5] = buffer[6 * k + 3];
    packet[6] = buffer[6 * k + 4];
    packet[7] = buffer[6 * k + 5];

    /* 计算校验和 */
    uint8_t checksum = 0;
    for (int i = 0; i <= 7; i++)
    {
        checksum += packet[i];
    }
    packet[8] = checksum;
    packet[9] = 0xBB;

    /* DMA 发送数据包 */
    UART_Transmit_DMA((char*)packet);
}
