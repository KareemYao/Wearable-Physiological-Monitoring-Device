/**
 * @file  bsp_usart_dma.c
 * @brief USART DMA 发送封装实现
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_usart_dma.h"

/* Private variables ---------------------------------------------------------*/
static volatile uint8_t tx_complete = 1;    /* 发送完成标志 */

/* Exported functions --------------------------------------------------------*/

/* UART DMA 发送 */
void UART_Transmit_DMA(char *buf)
{
    if (!tx_complete)
        return;

    tx_complete = 0;
    HAL_UART_Transmit_DMA(&huart1, (uint8_t *)buf, strlen(buf));
}

/* UART DMA 发送完成回调 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart1)
    {
        tx_complete = 1;
    }
}

/* 构造并发送数据包: AA 55 HR SPO2 CHECKSUM BB */
void Send_Data(int32_t data1, int32_t data2)
{
    static char text_buf[32];
    sprintf(text_buf, "%d,%d\r\n", data1, data2);
    UART_Transmit_DMA(text_buf);
}
