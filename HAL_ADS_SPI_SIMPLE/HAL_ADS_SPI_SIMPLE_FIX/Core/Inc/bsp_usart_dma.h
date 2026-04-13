/**
 * @file  bsp_usart_dma.h
 * @brief USART DMA 发送封装接口
 */
#ifndef BSP_USART_DMA_H
#define BSP_USART_DMA_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "usart.h"
#include "bsp_ads1292.h"

/* Exported functions --------------------------------------------------------*/
void UART_Transmit_DMA(char *buf);
void Send_Data(int32_t data1, int32_t data2);

#ifdef __cplusplus
}
#endif

#endif /* BSP_USART_DMA_H */
