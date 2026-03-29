/**
 * @file  AT_Basic.h
 * @brief AT 基础命令接口 (测试/复位/恢复出厂)
 */
#ifndef AT_BASIC_H
#define AT_BASIC_H

/* Includes ------------------------------------------------------------------*/
#include "AT_Header.h"

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  发送 "AT" 测试命令并等待 OK 响应
 * @param  AT_Channel  指向 HAL UART 句柄的指针
 * @param  r_buffer    接收响应的缓冲区
 * @retval HAL_OK      成功检测到 "\r\nOK\r\n"
 * @retval HAL_ERROR   未找到 OK 或底层错误
 * @retval HAL_TIMEOUT 超时
 * @see    Send_AT_Command
 */
HAL_StatusTypeDef Send_AT_Test(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer);

/**
 * @brief  软复位 NearLink 模组, 并等待 ready 信号
 * @param  AT_Channel  指向 HAL UART 句柄的指针
 * @param  r_buffer    接收响应的缓冲区
 * @retval HAL_OK      成功检测到 "\r\nready\r\n"
 * @retval HAL_ERROR   未找到 ready 或底层错误
 * @retval HAL_TIMEOUT 超时
 * @see    Send_AT_Command
 */
HAL_StatusTypeDef Soft_Reset_NearLink(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer);

/**
 * @brief  恢复 NearLink 模组至出厂默认设置
 * @param  AT_Channel  指向 HAL UART 句柄的指针
 * @param  r_buffer    接收响应的缓冲区
 * @retval HAL_OK      成功检测到 OK 和 ready
 * @retval HAL_ERROR   未找到预期响应或底层错误
 * @retval HAL_TIMEOUT 超时
 * @warning 此操作不可逆, 请确保已备份必要配置
 * @see    Send_AT_Command
 */
HAL_StatusTypeDef Restore_Default_Settings(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer);

#endif /* AT_BASIC_H */
