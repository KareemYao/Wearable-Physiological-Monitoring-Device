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
#include "main.h"
#include "usart.h"
#include "bsp_ir_red_cal2.h"

/* Defines -------------------------------------------------------------------*/
#define PACKET_HEADER_1     0xAA    /* 包头字节1 */
#define PACKET_HEADER_2     0x55    /* 包头字节2 */
#define PACKET_TAIL         0xBB    /* 包尾字节 */

/* Exported functions --------------------------------------------------------*/
void UART_Transmit_DMA(char *buf);
void Send_DataPacket(void);

#ifdef __cplusplus
}
#endif

#endif /* BSP_USART_DMA_H */
