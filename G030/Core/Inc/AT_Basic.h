#ifndef AT_BASIC_H
#define AT_BASIC_H

#include "AT_Header.h"


/**
 * @brief 发送标准 "AT" 命令并等待 NearLink 模组 OK 响应
 *
 * @details 本函数调用命令代理发送函数 @ref Send_AT_Command 发送 AT 测试指令，并在
 *          接收到的完整响应中搜索 "\r\nOK\r\n" 子串以确认通信链路正常且模块处
 *          于 AT 指令模式。
 *
 * @param[in]  AT_Channel   指向 HAL UART 句柄的指针
 * @param[out] r_buffer     接收响应的缓冲区
 *
 * @note 在透传模式下 NearLink 模块不会响应测试指令，建议先判断 NearLink 模块的状态再使用
 *
 * @retval HAL_OK           成功发送测试指令，并在响应中检测到 "\r\nOK\r\n"
 * @retval HAL_ERROR        发生以下任一错误：
 *                          - 接受响应过程中 @ref Send_AT_Command 抛出相关 @c HAL_ERROR
 *                            (具体情况参考 @ref Send_AT_Command 的返回值说明)
 *                          - 收到完整响应但未找到 "\r\nOK\r\n" 子串
 * @retval HAL_TIMEOUT      Send_AT_Command() 超时，未在规定时间内收到 OK/ERROR 结尾的完整响应
 *
 * @pre 调用者必须确保满足通用 AT 命令发送的前置条件（参见 @ref Send_AT_Command）
 *
 * @warning 此函数为阻塞式调用，直到收到响应或超时。
 *
 * @see Send_AT_Command
 */
HAL_StatusTypeDef 
Send_AT_Test(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);


/**
 * @brief 软复位 NearLink 模组，并等待其完成启动
 *
 * @details 本函数调用命令代理发送函数 @ref Send_AT_Command 发送 AT+RST 指令触发软复位，
 *          并在接收到的完整响应中搜索 "\r\nready\r\n" 的启动成功消息
 *              参考文档：bs21_模组combo_AT通用指令_v4.18p_0.0.3.pdf,§1.2(2)。
 *
 * @param[in]  AT_Channel   指向 HAL UART 句柄的指针
 * @param[out] r_buffer     接收响应的缓冲区
 *
 * @note 本函数内部会调用 HAL_UART_Receive_DMA() 启动 DMA 接收，
 *       因此调用者无需预先启动 DMA。
 *
 * @retval HAL_OK           成功发送复位指令，并在响应中检测到 "\r\nready\r\n" 启动信息
 * @retval HAL_ERROR        发生以下任一错误：
 *                          - 接受响应过程中 @ref Send_AT_Command 抛出相关 @c HAL_ERROR
 *                            (具体情况参考 @ref Send_AT_Command 的返回值说明)
 *                          - 收到完整响应但未找到 "\r\nready\r\n" 子串
 * @retval HAL_TIMEOUT      Send_AT_Command() 超时，未在规定时间内收到 OK/ERROR 结尾的完整响应
 *
 * @pre 调用者必须确保满足通用 AT 命令发送的前置条件（参见 @ref Send_AT_Command）
 * 
 * @see Send_AT_Command
 */
HAL_StatusTypeDef 
Soft_Reset_NearLink(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);


/**
 * @brief 恢复 NearLink 模组至出厂默认设置
 *
 * @details 本函数发送 AT+RESTORE 指令，恢复出厂模式，擦除配置信息(三元组、IO 映射除外)，
 *          模块在恢复后会自动重启并输出 "\r\nready\r\n" 启动信息。
 *              参考文档：bs21_模组combo_AT通用指令_v4.18p_0.0.3.pdf,§2.4。
 *
 * @param[in]  AT_Channel   指向 HAL UART 句柄的指针
 * @param[out] r_buffer     接收响应的缓冲区
 *
 * @retval HAL_OK           成功发送重置指令，并在响应中检测到 "\r\nready\r\n" 启动信息
 * @retval HAL_ERROR        发生以下任一错误：
 *                          - 接受响应过程中 @ref Send_AT_Command 抛出相关 @c HAL_ERROR
 *                            (具体情况参考 @ref Send_AT_Command 的返回值说明)
 *                          - 收到完整响应但未找到 "\r\nready\r\n" 子串
 * @retval HAL_TIMEOUT      Send_AT_Command() 超时，未在规定时间内收到 OK/ERROR 结尾的完整响应
 *
 * @pre 调用者必须确保满足通用 AT 命令发送的前置条件（参见 @ref Send_AT_Command）
 *
 * @warning 此操作不可逆，请确保已备份必要配置。
 * 
 * @see Send_AT_Command
 */
HAL_StatusTypeDef Restore_Default_Settings(UART_HandleTypeDef* AT_Channel, uint8_t* r_buffer);
HAL_StatusTypeDef 
Restore_Default_Settings(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);

#endif // AT_BASIC_H
