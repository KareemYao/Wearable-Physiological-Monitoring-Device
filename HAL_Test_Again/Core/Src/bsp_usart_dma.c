/*
 * bsp_usart_dma.c
 * USART DMA 发送封装
 */
#include "bsp_usart_dma.h"

static volatile uint8_t tx_complete = 1; /* 发送完成标志 */

/* UART DMA 发送*/
void UART_Transmit_DMA(char* buf)
{
    if (tx_complete == 0 && huart1.gState == HAL_UART_STATE_READY) {
        tx_complete = 1;
    }

    if (tx_complete) {
        tx_complete = 0;
        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)buf, 6);
    }
}

/* UART DMA 发送完成回调 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if(huart == &huart1) tx_complete = 1;
}

/* 构造并发送数据包 */
void Send_DataPacket(void)
{
    uint8_t packet[6] = {0};
    packet[0] = PACKET_HEADER_1;
    packet[1] = PACKET_HEADER_2;
    packet[2] = g_vital_signs.smooth_hr;
    packet[3] = g_vital_signs.smooth_spo2;  

    uint8_t checksum = 0;
    for(int i = 0; i <=3; i++) {
        checksum += packet[i];
    }
    packet[4] = checksum;
    packet[5] = PACKET_TAIL;

    UART_Transmit_DMA((char*)packet);
}
