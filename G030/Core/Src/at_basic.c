/**
 * @file  at_basic.c
 * @brief AT 基础命令实现 (测试/复位/恢复出厂)
 */

/* Includes ------------------------------------------------------------------*/
#include "NLsocket.h"

/* Exported functions --------------------------------------------------------*/

/* 发送 AT 测试命令 */
HAL_StatusTypeDef Send_AT_Test(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer)
{
    HAL_StatusTypeDef status = HAL_OK;
    const char msg[] = "AT\r\n";

    if ((status = Send_AT_Command(AT_Channel, r_buffer, (uint8_t *)msg, sizeof(msg))) != HAL_OK)
    {
        return status;
    }

    if (strstr((const char *)r_buffer, "\r\nOK\r\n") != NULL)
    {
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/* 软复位 NearLink 模组 */
HAL_StatusTypeDef Soft_Reset_NearLink(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer)
{
    HAL_StatusTypeDef status = HAL_OK;
    const char msg[] = "AT+RST\r\n";

    if ((status = Send_AT_Command_With_Ready_Detect(AT_Channel, r_buffer, (uint8_t *)msg, sizeof(msg))) != HAL_OK)
    {
        return status;
    }

    /*
     * 参考文档: bs21_模组combo_AT通用指令_v4.18p_0.0.3.pdf
     * 1.2 启动信息: 建议检测 ready 来检测启动信息
     */
    if (strstr((const char *)r_buffer, "\r\nready\r\n") != NULL)
    {
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/* 恢复 NearLink 模组出厂设置 */
HAL_StatusTypeDef Restore_Default_Settings(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer)
{
    HAL_StatusTypeDef status = HAL_OK;
    const char msg[] = "AT+RESTORE\r\n";

    if ((status = Send_AT_Command_With_Ready_Detect(AT_Channel, r_buffer, (uint8_t *)msg, sizeof(msg))) != HAL_OK)
    {
        return status;
    }

    if (strstr((const char *)r_buffer, "\r\nOK\r\n") != NULL)
    {
        if (strstr((const char *)r_buffer, "\r\nready\r\n") != NULL)
        {
            return HAL_OK;
        }
    }

    return HAL_ERROR;
}
