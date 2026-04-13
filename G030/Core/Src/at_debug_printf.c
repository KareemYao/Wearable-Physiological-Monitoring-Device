
/**
 * @file printf_port.c
 * @brief ARM 编译器 printf 兼容性移植实现
 */

#include <stdio.h>
#include "usart.h"  
#include "AT_debug_printf.h"

/* ============================================================================ */
/* 编译器特定的重定向实现 */
/* ============================================================================ */

#if COMPILER_ARMCC
    /* ========================================================================= */
    /* ArmCC (ARM Compiler 5) - Keil MDK 5.x */
    /* ========================================================================= */
    
    // 关闭半主机模式
    #pragma import(__use_no_semihosting)
    
    // 定义标准输出流
    struct __FILE {
        int handle;
    };
    
    FILE __stdout;
    FILE __stdin;
    
    /**
     * @brief ArmCC 的 fputc 实现
     * printf 最终会调用此函数
     */
    int fputc(int ch, FILE *f)
    {
        uint8_t c = (uint8_t)ch;
        
        // 安全检查：确保 UART 已初始化
        if (PRINTF_UART_HANDLE.Instance == NULL) {
            return EOF;
        }
        
        // 发送单个字符
        if (HAL_UART_Transmit(&PRINTF_UART_HANDLE, &c, 1, PRINTF_TIMEOUT_MS) != HAL_OK) {
            return EOF;
        }
        
        return ch;
    }
    
    /**
     * @brief 可选：支持 scanf
     */
    int fgetc(FILE *f)
    {
        uint8_t c;
        
        if (PRINTF_UART_HANDLE.Instance == NULL) {
            return EOF;
        }
        
        if (HAL_UART_Receive(&PRINTF_UART_HANDLE, &c, 1, PRINTF_TIMEOUT_MS) == HAL_OK) {
            return c;
        }
        
        return EOF;
    }
    
    // 必需的桩函数
    void _sys_exit(int x) {
        while(1);
    }
    
#elif COMPILER_ARMCLANG
    /* ========================================================================= */
    /* ArmClang (ARM Compiler 6) - Keil MDK 6.x+ */
    /* ========================================================================= */
    
    #include <rt_sys.h>
    
    /**
     * @brief ArmClang 的底层写入函数
     * 这是 ARM Compiler 6 推荐的方式
     */
    int __ARM_write(int fh, const unsigned char *buf, unsigned len, int mode)
    {
        (void)mode;  // 未使用
        
        // fh == 1: stdout, fh == 2: stderr
        if (fh != 1 && fh != 2) {
            return -1;  // 错误的文件句柄
        }
        
        // 安全检查
        if (PRINTF_UART_HANDLE.Instance == NULL || buf == NULL || len == 0) {
            return -1;
        }
        
        // 发送数据
        if (HAL_UART_Transmit(&PRINTF_UART_HANDLE, (uint8_t *)buf, len, PRINTF_TIMEOUT_MS) != HAL_OK) {
            return -1;
        }
        
        return len;  // 返回已写入的字节数
    }
    #ifdef WORKABLE
    /**
     * @brief 备用方案：fputc 也可以工作
     * 在某些配置下，ArmClang 也会调用 fputc
     */
    int fputc(int ch, FILE *f)
    {
        uint8_t c = (uint8_t)ch;
        
        if (PRINTF_UART_HANDLE.Instance == NULL) {
            return EOF;
        }
        
        if (HAL_UART_Transmit(&PRINTF_UART_HANDLE, &c, 1, PRINTF_TIMEOUT_MS) != HAL_OK) {
            return EOF;
        }
        
        return ch;
    }
    #endif
#elif COMPILER_ARMGCC
    /* ========================================================================= */
    /* Arm-GCC (GNU ARM Embedded Toolchain) */
    /* ========================================================================= */
    
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <errno.h>
    #include <unistd.h>
    
    /**
     * @brief GCC newlib 的 _write 系统调用
     * 这是 GCC 最标准的方式
     */
    __attribute__((weak))
    int _write(int file, char *ptr, int len)
    {
        // file == 1: stdout, file == 2: stderr
        if (file != STDOUT_FILENO && file != STDERR_FILENO) {
            errno = EBADF;
            return -1;
        }
        
        // 安全检查
        if (PRINTF_UART_HANDLE.Instance == NULL || ptr == NULL || len <= 0) {
            errno = EINVAL;
            return -1;
        }
        
        // 发送数据
        if (HAL_UART_Transmit(&PRINTF_UART_HANDLE, (uint8_t *)ptr, len, PRINTF_TIMEOUT_MS) != HAL_OK) {
            errno = EIO;
            return -1;
        }
        
        return len;  // 返回已写入的字节数
    }
    
    /**
     * @brief 备用方案：__io_putchar
     * 某些 newlib 配置使用此函数
     */
    __attribute__((weak))
    int __io_putchar(int ch)
    {
        uint8_t c = (uint8_t)ch;
        
        if (PRINTF_UART_HANDLE.Instance == NULL) {
            return EOF;
        }
        
        if (HAL_UART_Transmit(&PRINTF_UART_HANDLE, &c, 1, PRINTF_TIMEOUT_MS) != HAL_OK) {
            return EOF;
        }
        
        return ch;
    }
    
#endif

/* ============================================================================ */
/* 通用接口函数 */
/* ============================================================================ */

int printf_port_init(void)
{
    // 检查 UART 是否已初始化
    if (PRINTF_UART_HANDLE.Instance == NULL) {
        return -1;
    }
    
    return 0;
}

int printf_port_is_ready(void)
{
    return (PRINTF_UART_HANDLE.Instance != NULL) ? 1 : 0;
}

const char* printf_port_get_compiler(void)
{
    return COMPILER_STRING;
}

/* ============================================================================ */
/* 调试辅助函数 */
/* ============================================================================ */

#if defined(DEBUG) || defined(_DEBUG)
    void printf_port_test(void)
    {
        printf("\r\n=== printf_port Test ===\r\n");
        printf("Compiler: %s\r\n", printf_port_get_compiler());
        printf("Status: %s\r\n", printf_port_is_ready() ? "ready" : "unready");
        printf("Test Output: Hello World!\r\n");        
        printf("Test Output: pointer%X",0x7fcb42);
        printf("========================\r\n\r\n");
    }
#endif
