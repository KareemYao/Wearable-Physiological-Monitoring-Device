#ifndef AT_CORE_H
#define AT_CORE_H

#include "AT_Header.h"


/**
 * @brief 从 UART 接收完整的 AT 响应（跳过前导垃圾，按行读取直到 OK/ERROR）
 * 
 * @param[in]  AT_Channel   指向 HAL UART 句柄的指针
 * @param[out] r_buffer     用户提供的接收缓冲区（必须足够大以容纳预期响应，内部按固定最大长度处理并预留 25% 余量）
 * 
 * @pre 必须已成功调用 HAL_UART_Receive_DMA(AT_Channel, r_buffer, ...) 启动 DMA 接收，
 *      且 DMA 仍在运行中。本函数依赖 DMA 向 r_buffer 写入数据并通过
 *      __HAL_DMA_GET_COUNTER 监控接收进度。
 * 
 * @retval HAL_OK           成功接收到完整的 AT 响应（以 OK 或 ERROR 结尾）
 * @retval HAL_TIMEOUT      接收超时，未能在规定时间 @ref WAIT_TIME_MS 内收到完整响应字符串
 * @retval HAL_ERROR        发生严重错误，包括：
 *                          - 传入的 @p AT_Channel 或 @p r_buffer 为 @c NULL ；
 *                          - 接收数据量超过内部缓冲区安全阈值（达到其容量的 75%），
 *                            此时函数将触发 panic 并复位模块。
 */
HAL_StatusTypeDef 
Read_AT_response(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);

HAL_StatusTypeDef 
Detect_NearLink_ready(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);



/**
 * @brief 发送任意 AT 命令并等待完整响应（以 OK 或 ERROR 结尾）
 *
 * @details 执行标准 AT 命令交互流程：
 *          1. 启动 DMA 接收准备捕获响应；
 *          2. 发送用户提供的命令字节流；
 *          3. 调用 Read_AT_response 等待模组返回以 \r\nOK\r\n 或 \r\nERROR\r\n 结尾的完整响应。
 *          
 * @note 注意：本函数不解析响应内容，仅确保收到完整帧。
 * 
 * @warning 
 *
 * @param[in]  AT_Channel   HAL UART 句柄的指针
 * @param[out] r_buffer     用户提供的接收缓冲区
 * @param[in]  cmd          要发送的命令字节流（无需以 \0 结尾）
 * @param[in]  cmd_len      命令长度（字节数）
 *
 * @retval HAL_OK           成功收到以 OK/ERROR 结尾的完整响应
 * @retval HAL_ERROR        发生以下任一错误：
 *                          - 传入的 @p AT_Channel , @p r_buffer 或 @p cmd 为 @c NULL ，
 *                            以及 @p cmd_len 等于 0；
 *                          - 底层 UART 发送或 DMA 接收操作失败；
 *                          - Read_AT_response() 返回错误（例如接收缓冲区溢出或内部状态异常）。
 * 
 * @retval HAL_TIMEOUT      等待响应超时，未能在规定时间 @ref WAIT_TIME_MS 内收到完整响应字符串
 *
 * 
 * @pre 
 *   - @c BUFFER_SIZE 必须 ≥ 预期最大 AT 响应长度
 *   - UART 外设已通过 @c MX_USARTx_UART_Init() 正确初始化
 *   - 若使用 DMA，相关 DMA 通道已配置并启用
 * 
 * @see Read_AT_response
 */ 
HAL_StatusTypeDef 
Send_AT_Command(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer,
    const uint8_t* const cmd,
    const size_t cmd_len
);

HAL_StatusTypeDef 
Send_AT_Command_With_Ready_Detect(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer,
    const uint8_t* const cmd,
    const size_t cmd_len
);

#endif // AT_CORE_H
