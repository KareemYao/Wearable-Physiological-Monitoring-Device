/**
 * @file  bsp_usart_dma.c
 * @brief USART DMA 发送封装实现
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_usart_dma.h"

/* Private variables ---------------------------------------------------------*/
static volatile uint8_t tx_complete = 1;    /* 发送完成标志 */
static char text_buf[2][48];                /* 双缓冲 */
static uint8_t buf_idx = 0;                 /* 当前写入缓冲索引 */

/* Exported functions --------------------------------------------------------*/

/* UART DMA 发送 */
void UART_Transmit_DMA(char *buf)
{
    if (!tx_complete)
        return;

    tx_complete = 0;
    if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)buf, strlen(buf)) != HAL_OK)
    {
        tx_complete = 1; // 发送失败，恢复标志位
    }
}

/* UART DMA 发送完成回调 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        tx_complete = 1;
    }
}

/* 发数据 */
void Send_Data(int32_t data1, int32_t data2)
{
    if (!tx_complete)
        return;

    char *buf = text_buf[buf_idx];
    sprintf(buf, "%d,%d\r\n", (int)data1, (int)data2);
    buf_idx ^= 1;
    tx_complete = 0;
    if (HAL_UART_Transmit_DMA(&huart1, (uint8_t *)buf, strlen(buf)) != HAL_OK)
    {
        tx_complete = 1;
    }
}
