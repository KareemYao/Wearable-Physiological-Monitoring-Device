/**
 * @file  AT_Core.h
 * @brief AT 命令核心收发接口
 */
#ifndef AT_CORE_H
#define AT_CORE_H

/* Includes ------------------------------------------------------------------*/
#include "AT_Header.h"

/* Exported functions --------------------------------------------------------*/

/**
 * @brief  从 UART 接收完整的 AT 响应 (跳过前导垃圾, 按行读取直到 OK/ERROR)
 * @param  AT_Channel  指向 HAL UART 句柄的指针
 * @param  r_buffer    用户提供的接收缓冲区
 * @retval HAL_OK      成功接收到完整的 AT 响应
 * @retval HAL_TIMEOUT 接收超时
 * @retval HAL_ERROR   参数为 NULL 或缓冲区溢出
 */
HAL_StatusTypeDef Read_AT_response(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer);

/**
 * @brief  检测 NearLink 就绪信号
 * @param  AT_Channel  指向 HAL UART 句柄的指针
 * @param  r_buffer    用户提供的接收缓冲区
 * @retval HAL_OK      检测到 ready 信号
 * @retval HAL_TIMEOUT 超时
 * @retval HAL_ERROR   参数为 NULL 或缓冲区溢出
 */
HAL_StatusTypeDef Detect_NearLink_ready(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer);

/**
 * @brief  发送任意 AT 命令并等待完整响应
 * @param  AT_Channel  HAL UART 句柄的指针
 * @param  r_buffer    用户提供的接收缓冲区
 * @param  cmd         要发送的命令字节流
 * @param  cmd_len     命令长度 (字节数)
 * @retval HAL_OK      成功收到以 OK/ERROR 结尾的完整响应
 * @retval HAL_TIMEOUT 等待响应超时
 * @retval HAL_ERROR   参数错误或底层操作失败
 * @see    Read_AT_response
 */
HAL_StatusTypeDef Send_AT_Command(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer,
                                  const uint8_t *const cmd, const size_t cmd_len);

/**
 * @brief  发送 AT 命令并等待 ready 信号
 * @param  AT_Channel  HAL UART 句柄的指针
 * @param  r_buffer    用户提供的接收缓冲区
 * @param  cmd         要发送的命令字节流
 * @param  cmd_len     命令长度 (字节数)
 * @retval HAL_OK      成功收到 ready 信号
 * @retval HAL_TIMEOUT 等待响应超时
 * @retval HAL_ERROR   参数错误或底层操作失败
 */
HAL_StatusTypeDef Send_AT_Command_With_Ready_Detect(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer,
                                                     const uint8_t *const cmd, const size_t cmd_len);

#endif /* AT_CORE_H */
