/*
 * bsp_usart.h
 * 串口封装头文件
 */
#ifndef BSP_USART_H
#define BSP_USART_H

#include "stm32f4xx_hal.h"
#include "usart.h"

void UART_Transmit_DMA(char* buf);
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void Send_DataPacket(uint8_t *buffer, uint8_t k);

#endif /* BSP_USART_H */
