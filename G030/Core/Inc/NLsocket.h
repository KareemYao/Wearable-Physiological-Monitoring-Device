/**
 * @file  NLsocket.h
 * @brief NearLink 通信套接字接口
 */
#ifndef NLSOCKET_H
#define NLSOCKET_H

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "AT_Core.h"
#include "AT_Basic.h"

/* Defines -------------------------------------------------------------------*/
#define WAIT_TIME_MS        3000
#define BUFFER_SIZE_ALLOC   (1024 + 1)              /* 1KB 内存分配 */
#define BUFFER_SIZE         (BUFFER_SIZE_ALLOC * 3 / 4) /* 缓冲区安全边界 */

/* Types ---------------------------------------------------------------------*/
typedef struct {
    UART_HandleTypeDef *uart_handle;
    uint8_t AT_buffer[BUFFER_SIZE_ALLOC];

    struct _Basic {
        HAL_StatusTypeDef (* const Restore_Default)(UART_HandleTypeDef *, uint8_t *);
        HAL_StatusTypeDef (* const Soft_Reset)(UART_HandleTypeDef *, uint8_t *);
        HAL_StatusTypeDef (* const Send_AT)(UART_HandleTypeDef *, uint8_t *, const uint8_t *const, const size_t);
        HAL_StatusTypeDef (* const Test)(UART_HandleTypeDef *, uint8_t *);
    } Basic;
} NLsocket;

/* Macros --------------------------------------------------------------------*/

/**
 * @brief 静态初始化 NearLink 通信套接字 (用于全局或静态变量)
 *
 * @param uart_handle_ptr 指向已初始化的 UART_HandleTypeDef 的指针
 *
 * @note .AT_buffer 由全局/静态变量的零初始化保证清零,
 *       所有成员函数已在入口处对参数进行空指针检查。
 *
 * @par 示例
 * @code{.c}
 * NLsocket g_nlink = NL_SOCKET_INIT(&huart1);
 * @endcode
 *
 * @see NLsocket_make
 */
#define NL_SOCKET_INIT(uart_handle_ptr)                 \
    (NLsocket){                                         \
        .uart_handle = (uart_handle_ptr),               \
        .Basic = {                                      \
            .Send_AT         = Send_AT_Command,         \
            .Test            = Send_AT_Test,            \
            .Soft_Reset      = Soft_Reset_NearLink,     \
            .Restore_Default = Restore_Default_Settings \
        }                                               \
    }

/* Inline functions ----------------------------------------------------------*/

/**
 * @brief 创建一个动态的 NearLink 通信套接字实例 (栈上/局部使用)
 *
 * @param AT_Channel 指向 HAL UART 句柄的指针 (必须非 NULL)
 * @return 已初始化的 NLsocket 实例
 *
 * @see NL_SOCKET_INIT
 */
static inline NLsocket NLsocket_make(UART_HandleTypeDef *AT_Channel)
{
    return (NLsocket){
        .uart_handle = AT_Channel,
        .AT_buffer   = {0},
        .Basic = {
            .Send_AT         = Send_AT_Command,
            .Test            = Send_AT_Test,
            .Soft_Reset      = Soft_Reset_NearLink,
            .Restore_Default = Restore_Default_Settings
        }
    };
}

#endif /* NLSOCKET_H */
