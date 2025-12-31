/*
 * bsp_usart_dma.h
 * USART DMA 发送封装头文件
 */
#ifndef __BSP_USART_DMA_H__
#define __BSP_USART_DMA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "usart.h"
#include "bsp_ir_red_cal2.h"

/* 数据包格式: AA 55 HR SPO2 CHECKSUM BB */
#define PACKET_HEADER_1 0xAA
#define PACKET_HEADER_2 0x55
#define PACKET_TAIL     0xBB

void UART_Transmit_DMA(char* buf);
void Send_DataPacket(void);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_USART_DMA_H__ */
