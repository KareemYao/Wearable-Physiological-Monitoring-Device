/**
 * @file AT_debug_printf.h
 * @brief ARM 编译器 printf 兼容性移植
 * @author Kareem Yao
 * @date 4.01.2026
 * 
 * 支持的编译器：
 *   - ArmCC (ARM Compiler 5)
 *   - ArmClang (ARM Compiler 6)
 *   - Arm-GCC (GNU ARM Embedded)
 */

#ifndef AT_DEBUG_PRINTF_H
#define AT_DEBUG_PRINTF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/* ============================================================================ */
/* 配置宏（可选覆盖） */
/* ============================================================================ */

// 默认使用 UART1
#ifndef PRINTF_UART_HANDLE
    #define PRINTF_UART_HANDLE      huart1
#endif

// 发送超时（毫秒）
#ifndef PRINTF_TIMEOUT_MS
    #define PRINTF_TIMEOUT_MS       100
#endif

/* ============================================================================ */
/* 编译器自动检测 */
/* ============================================================================ */

// ArmCC (ARM Compiler 5) - Keil MDK 5.x
#if defined(__CC_ARM) || defined(__ARMCC_VERSION) && (__ARMCC_VERSION < 6000000)
    #define COMPILER_ARMCC      1
    #define COMPILER_ARMCLANG   0
    #define COMPILER_ARMGCC     0
    #define COMPILER_STRING     "ArmCC (ARM Compiler 5)"

// ArmClang (ARM Compiler 6) - Keil MDK 6.x+
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
    #define COMPILER_ARMCC      0
    #define COMPILER_ARMCLANG   1
    #define COMPILER_ARMGCC     0
    #define COMPILER_STRING     "ArmClang (ARM Compiler 6)"

// Arm-GCC (GNU ARM Embedded Toolchain)
#elif defined(__GNUC__) && defined(__ARMEL__)
    #define COMPILER_ARMCC      0
    #define COMPILER_ARMCLANG   0
    #define COMPILER_ARMGCC     1
    #define COMPILER_STRING     "Arm-GCC (GNU ARM Embedded)"

#else
    #error "Unsupported compiler. Only ArmCC, ArmClang, and Arm-GCC are supported."
#endif


/* ============================================================================ */
/* 外部接口 */
/* ============================================================================ */

/**
 * @brief 初始化 printf 重定向
 * @return 0=成功, -1=失败
 */
int printf_port_init(void);

/**
 * @brief 检查 printf 是否可用
 * @return 1=可用, 0=不可用
 */
int printf_port_is_ready(void);

/**
 * @brief 获取当前编译器信息
 * @return 编译器名称字符串
 */
const char* printf_port_get_compiler(void);

#if defined(DEBUG) || defined(_DEBUG)
    void printf_port_test(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* AT_DEBUG_PRINTF_H */
