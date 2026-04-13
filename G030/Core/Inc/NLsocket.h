#ifndef NLSOCKET_H
#define NLSOCKET_H

#ifdef DEBUG
    #define _DEBUG
#endif

#include "AT_Header.h"
#include "AT_Core.h"
#include "AT_Basic.h"
#include "AT_SLE_config.h"


/**
 * @brief 静态初始化 NearLink 通信套接字（用于全局或静态变量）
 *
 * @details 此宏生成一个符合 C99 指定初始化器语法的常量表达式，专为具有静态存储期的
 *          @ref NLsocket 实例设计（如全局变量、 @c static 变量）。它在编译期完成配置绑定，
 *          无任何运行时开销，并允许实例被声明为 @c const （若结构体支持）。
 *
 * @param uart_handle_ptr 指向已初始化的 @c UART_HandleTypeDef 的指针。
 *                        必须是编译期地址常量（如 @c &huart1 ），不可为 @c NULL。
 *
 * @note 此宏 **不显式初始化 @c .AT_buffer 成员**，原因如下：
 *       - 全局/静态变量默认零初始化（C 标准保证）， @c .AT_buffer 自动清零；
 *       - 显式写 @c .AT_buffer = {0} 虽合法，但冗余且增加宏体积；
 *       - 若用于局部变量（不推荐），需手动确保缓冲区清零（建议改用 @ref NLsocket_make）。
 *            完善的 @c NULL 检查（通过 @c assert 或返回错误码），确保运行时安全。
 *
 * @warning 此宏 **不包含对 @p uart_handle_ptr 的空指针检查**（如 @c assert ），
 *          原因如下：
 *          - 全局/静态初始化器必须是常量表达式，而 @c assert 在 Debug 模式下
 *            可能展开为含副作用的函数调用，违反 C 标准（§6.7.9）；
 *          - STM32 HAL 生成的 UART 句柄（如 @c huart1 ）是链接期符号，地址恒非 NULL；
 *          - 所有成员函数（如 @c Soft_Reset_NearLink ）已在入口处对参数进行空指针检查
 *
 * 
 * @par 设计理念
 *      此宏遵循“编译期配置 + 运行时防御”原则：
 *      - 配置绑定（函数指针、UART 句柄）在编译期完成，高效且不可变；
 *      - 安全性由成员函数的入口检查保障，避免重复校验。
 *
 * @par 示例
 * @code{.c}
 * // 全局 NearLink 实例（推荐用法）
 * NLsocket g_nlink = NL_SOCKET_INIT(&huart1);
 *
 * int main(void)
 * {
 *     // 首次调用即触发参数检查（若传入非法句柄）
 *     g_nlink.Basic.Soft_Reset(g_nlink.uart_handle, g_nlink.AT_buffer);
 * }
 * @endcode
 *
 * @see NLsocket_make
 * @see Soft_Reset_NearLink
 * @see Send_AT_Test
 */
#define NL_SOCKET_INIT(uart_handle_ptr)     \
    (NLsocket){                             \
        .uart_handle = (uart_handle_ptr),   \
        .Basic = {                          \
            .Send_AT_cmd= Send_AT_Command,  \
            .Test       = Send_AT_Test,     \
            .Soft_Reset = Soft_Reset_NearLink, \
            .Restore_Default = Restore_Default_Settings,\
        }                                   \
    }


typedef struct {

    UART_HandleTypeDef* uart_handle;
    
    uint8_t AT_buffer[BUFFER_SIZE_ALLOC];
    
    struct _Basic{
        HAL_StatusTypeDef (* const Restore_Default)(UART_HandleTypeDef*, uint8_t*);
        HAL_StatusTypeDef (* const Soft_Reset)(UART_HandleTypeDef*, uint8_t*);
        HAL_StatusTypeDef (* const Send_AT_cmd)(UART_HandleTypeDef*, uint8_t* ,const uint8_t* const ,const size_t, const uint32_t);
        HAL_StatusTypeDef (* const Test)(UART_HandleTypeDef*, uint8_t*);
    }Basic;
    
}NLsocket;

/**
 * @brief 创建一个动态的 NearLink 通信套接字实例（栈上/局部使用）
 *
 * @details 此函数在运行时构造一个完整的 @ref NLsocket 实例，适用于需要临时、
 *          可复制或局部使用的场景（如单元测试、多通道操作等）。
 *
 * @param[in] AT_Channel 指向 HAL UART 句柄的指针（必须非 NULL）
 *
 * @note 与静态初始化宏 @ref NL_SOCKET_INIT 不同：
 *       - 所有函数指针在运行时绑定，但行为与静态版本一致；
 *       - 内部缓冲区（ @c AT_buffer ）会被执行零初始化。
 *
 * @warning 此函数 **不应用于全局或静态变量初始化**，因为 C 标准禁止在
 *          具有静态存储期的对象初始化器中调用函数。请改用 @ref NL_SOCKET_INIT 宏。
 * 
 * @return 已初始化的 @ref NLsocket 实例
 *
 * @pre @p AT_Channel 必须指向一个已正确初始化的 @c UART_HandleTypeDef 对象
 * 
 * @post 返回的实例可立即用于调用其成员函数(如 @c .Basic.Soft_Reset )
 *
 * @par 示例
 * @code{.c}
 * void test_routine(void)
 * {
 *     NLsocket temp = NLsocket_make(&huart2);
 *     if (temp.Basic.Soft_Reset(temp.uart_handle, temp.AT_buffer) == HAL_OK) {
 *         // ... 处理复位成功
 *     }
 * }
 * @endcode
 *
 * @see NL_SOCKET_INIT
 * @see NLsocket
 */
static inline 
NLsocket NLsocket_make(UART_HandleTypeDef* AT_Channel){
    return (NLsocket){
        .uart_handle = AT_Channel,
        .AT_buffer = {0},
        .Basic = {                          
            .Send_AT_cmd= Send_AT_Command,
            .Test       = Send_AT_Test,
            .Soft_Reset = Soft_Reset_NearLink, 
            .Restore_Default = Restore_Default_Settings,
        }           
    };
}


#endif // NLSOCKET_H
