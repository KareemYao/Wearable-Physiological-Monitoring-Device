/**
 * @file  at_core.c
 * @brief AT 命令核心收发实现
 */

/* Includes ------------------------------------------------------------------*/
#include "NLsocket.h"

/* Private functions ---------------------------------------------------------*/

/* 判断 AT 响应是否结束 (OK/ERROR/Unknown cmd) */
static int is_AT_response_end(uint8_t *buffer, uint16_t begin, uint16_t end)
{
    uint8_t *session = buffer + begin;
    uint8_t begin_with_CRLF = 1;

    if (end - begin < 3)
    {
        return 0;
    }

    if (buffer[end - 1] != '\r' || buffer[end] != '\n')
    {
        return 0;
    }

    if (buffer[begin] != '\r' || buffer[begin + 1] != '\n')
    {
        begin_with_CRLF = 0;
    }

    if (memcmp(session, "\r\nOK\r\n", 6))
    {
        return 1;
    }
    if (memcmp(session, "\r\nERROR\r\n", 9))
    {
        return 1;
    }
    if (!begin_with_CRLF)
    {
        if (memcmp(session, "Unknown cmd:", 12))
        {
            return 1;
        }
    }

    return 0;
}

/* 判断 NearLink 是否就绪 */
static int is_NearLink_ready(uint8_t *buffer, uint16_t begin, uint16_t end)
{
    uint8_t *session = buffer + begin;

    if (end - begin < 3)
    {
        return 0;
    }

    if (buffer[end - 1] != '\r' || buffer[end] != '\n')
    {
        return 0;
    }

    if (memcmp(session, "\r\nready\r\n", 9))
    {
        return 1;
    }

    return 0;
}

/* Exported functions --------------------------------------------------------*/

/* 读取 AT 响应 (等待 OK/ERROR 结尾) */
HAL_StatusTypeDef Read_AT_response(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer)
{
    if (AT_Channel == NULL || r_buffer == NULL)
    {
        return HAL_ERROR;
    }

    uint32_t start_time    = HAL_GetTick();
    uint16_t index_buffer  = 0;
    uint16_t session_head  = 0;
    uint32_t buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx);

#ifdef _DEBUG
    memset(r_buffer, 0, BUFFER_SIZE);
#endif

    while ((HAL_GetTick() - start_time) < 4 * WAIT_TIME_MS)
    {
        if (buffer_length >= BUFFER_SIZE)
        {
            goto panic;
        }

        for (; index_buffer < buffer_length; ++index_buffer)
        {
            static uint8_t prev_char = 0;
            const  uint8_t rx_cursor = r_buffer[index_buffer];

            if (prev_char == '\r' && rx_cursor == '\n')
            {
                if (is_NearLink_ready(r_buffer, session_head, index_buffer))
                {
                    HAL_UART_AbortReceive(AT_Channel);
                    r_buffer[++index_buffer] = '\0';
                    return HAL_OK;
                }
                else
                {
                    session_head = index_buffer - 1;
                }
            }
            else
            {
                prev_char = rx_cursor;
            }
        }
    }

    r_buffer[index_buffer] = '\0';
    return HAL_TIMEOUT;

panic:
    HAL_UART_AbortReceive(AT_Channel);
    return HAL_ERROR;
}

/* 检测 NearLink 就绪信号 */
HAL_StatusTypeDef Detect_NearLink_ready(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer)
{
    if (AT_Channel == NULL || r_buffer == NULL)
    {
        return HAL_ERROR;
    }

    uint32_t start_time    = HAL_GetTick();
    uint16_t index_buffer  = 0;
    uint16_t session_head  = 0;
    uint32_t buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx);

#ifdef _DEBUG
    memset(r_buffer, 0, BUFFER_SIZE);
#endif

    while ((HAL_GetTick() - start_time) < WAIT_TIME_MS)
    {
        if (buffer_length >= BUFFER_SIZE)
        {
            goto panic;
        }

        for (; index_buffer < buffer_length; ++index_buffer)
        {
            static uint8_t prev_char = 0;
            const  uint8_t rx_cursor = r_buffer[index_buffer];

            if (prev_char == '\r' && rx_cursor == '\n')
            {
                if (is_AT_response_end(r_buffer, session_head, index_buffer))
                {
                    HAL_UART_AbortReceive(AT_Channel);
                    r_buffer[++index_buffer] = '\0';
                    return HAL_OK;
                }
                else
                {
                    session_head = index_buffer - 1;
                }
            }
            else
            {
                prev_char = rx_cursor;
            }
        }
    }

    r_buffer[index_buffer] = '\0';
    return HAL_TIMEOUT;

panic:
    HAL_UART_AbortReceive(AT_Channel);
    return HAL_ERROR;
}

/* 发送 AT 命令并等待响应 */
HAL_StatusTypeDef Send_AT_Command(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer,
                                  const uint8_t *const cmd, const size_t cmd_len)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (AT_Channel == NULL || r_buffer == NULL)
    {
        return HAL_ERROR;
    }
    if (cmd == NULL && cmd_len != 0)
    {
        return HAL_ERROR;
    }

    if (HAL_UART_Receive_DMA(AT_Channel, r_buffer, BUFFER_SIZE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((status = HAL_UART_Transmit(AT_Channel, cmd, cmd_len, 100)) != HAL_OK)
    {
        return status;
    }

    if ((status = Read_AT_response(AT_Channel, r_buffer)) != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}

/* 发送 AT 命令并等待 ready 信号 */
HAL_StatusTypeDef Send_AT_Command_With_Ready_Detect(UART_HandleTypeDef *AT_Channel, uint8_t *r_buffer,
                                                     const uint8_t *const cmd, const size_t cmd_len)
{
    HAL_StatusTypeDef status = HAL_OK;

    if (AT_Channel == NULL || r_buffer == NULL)
    {
        return HAL_ERROR;
    }
    if (cmd == NULL && cmd_len != 0)
    {
        return HAL_ERROR;
    }

    if (HAL_UART_Receive_DMA(AT_Channel, r_buffer, BUFFER_SIZE) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((status = HAL_UART_Transmit(AT_Channel, cmd, cmd_len, 100)) != HAL_OK)
    {
        return status;
    }

    if ((status = Detect_NearLink_ready(AT_Channel, r_buffer)) != HAL_OK)
    {
        return status;
    }

    return HAL_OK;
}
