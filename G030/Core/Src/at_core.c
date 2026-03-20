#include "NLsocket.h"

int is_AT_response_end(uint8_t* buffer, uint16_t begin, uint16_t end){
    
    uint8_t* session = buffer + begin;
    uint8_t begin_with_CRLF = 1; // true

    if (end - begin < 3)  // 至少 "\r\nX\r\n" => 5 字节，索引差 >=4 
        return 0;

    if (buffer[end - 1] != '\r' || buffer[end] != '\n') // 没有 "\r\n" 结尾
        return 0;

    if (buffer[begin] != '\r' || buffer[begin + 1] != '\n') 
        begin_with_CRLF = 0; // false

        
    // "\r\nOK\r\n"           <- "\r\nOK\r\n" 
    // "\r\nERROR\r\n"        <- "\r\n+<CMD>:<error_code>\r\nERROR\r\n" 
    // "Unknown cmd:...\r\n"  <- "Unknown cmd:ok\r\n"
    if (memcmp(session,"\r\nOK\r\n",6))
        return 1;
    if (memcmp(session,"\r\nERROR\r\n",9))
        return 1;
    if (!begin_with_CRLF){
        if (memcmp(session,"Unknown cmd:",12))
            return 1;
    }
    
    return 0;
}

int is_NearLink_ready(uint8_t* buffer, uint16_t begin, uint16_t end){
    
    uint8_t* session = buffer + begin;

    if (end - begin < 3)  // 至少 "\r\nX\r\n" => 5 字节，索引差 >=4 
        return 0;

    if (buffer[end - 1] != '\r' || buffer[end] != '\n') // 没有 "\r\n" 结尾
        return 0;

    if (memcmp(session,"\r\nready\r\n",9))
        return 1;
    
    return 0;
}
HAL_StatusTypeDef 
Read_AT_response(UART_HandleTypeDef* AT_Channel, uint8_t* r_buffer){
    
    if(AT_Channel == NULL || r_buffer == NULL)
        return HAL_ERROR;
    
    
    uint32_t start_time    = HAL_GetTick();
    uint16_t index_buffer  = 0;
    uint16_t session_head  = 0;
    uint32_t buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx);

#ifdef _DEBUG
    // 清空输出缓冲区
    memset(r_buffer, 0, BUFFER_SIZE);
#endif

    // 4 times WAIT_TIME_MS
    while ((HAL_GetTick() - start_time) < 4 * WAIT_TIME_MS){
        
        if (buffer_length >= BUFFER_SIZE) 
            goto panic;


        for(index_buffer; index_buffer < buffer_length; ++index_buffer){
            
            static uint8_t prev_char = 0;
            const  uint8_t rx_cursor = r_buffer[index_buffer];

            if(prev_char == '\r' && rx_cursor == '\n'){
                if(is_NearLink_ready(r_buffer,session_head,index_buffer)){
                    // 成功！停止 DMA 读取
                    HAL_UART_AbortReceive(AT_Channel);
                    r_buffer[++index_buffer] = '\0';
                    return HAL_OK;
                }
                else 
                    session_head = index_buffer - 1;
            }
            else 
                prev_char = rx_cursor;
        }     
    }

    // 超时
    r_buffer[index_buffer] = '\0';
    return HAL_TIMEOUT;

panic:
    HAL_UART_AbortReceive(AT_Channel);
    // 触发 panic：重启模块 or 系统复位
    //  module_panic_and_reset(); // 通过软重启实现
    return HAL_ERROR;
}

HAL_StatusTypeDef 
Detect_NearLink_ready(UART_HandleTypeDef* AT_Channel,uint8_t* r_buffer){
    if(AT_Channel == NULL || r_buffer == NULL)
        return HAL_ERROR;
    
    
    uint32_t start_time    = HAL_GetTick();
    uint16_t index_buffer  = 0;
    uint16_t session_head  = 0;
    uint32_t buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx);

#ifdef _DEBUG
    // 清空输出缓冲区
    memset(r_buffer, 0, BUFFER_SIZE);
#endif

    while ((HAL_GetTick() - start_time) < WAIT_TIME_MS){
        
        if (buffer_length >= BUFFER_SIZE) 
            goto panic;


        for(index_buffer; index_buffer < buffer_length; ++index_buffer){
            
            static uint8_t prev_char = 0;
            const  uint8_t rx_cursor = r_buffer[index_buffer];

            if(prev_char == '\r' && rx_cursor == '\n'){
                if(is_AT_response_end(r_buffer,session_head,index_buffer)){
                    // 成功！停止 DMA 读取
                    HAL_UART_AbortReceive(AT_Channel);
                    r_buffer[++index_buffer] = '\0';
                    return HAL_OK;
                }
                else 
                    session_head = index_buffer - 1;
            }
            else 
                prev_char = rx_cursor;
        }     
    }

    // 超时
    r_buffer[index_buffer] = '\0';
    return HAL_TIMEOUT;

panic:
    HAL_UART_AbortReceive(AT_Channel);
    // 触发 panic：重启模块 or 系统复位
    //  module_panic_and_reset(); // 通过软重启实现
    return HAL_ERROR;
}

HAL_StatusTypeDef
Send_AT_Command(
    UART_HandleTypeDef* AT_Channel, uint8_t* r_buffer, const uint8_t* const cmd, const size_t cmd_len){
    
    HAL_StatusTypeDef status = HAL_OK;

    if(AT_Channel == NULL || r_buffer == NULL)
        return HAL_ERROR;

    if(cmd == NULL && cmd_len != 0)
        return HAL_ERROR;
    
    if (HAL_UART_Receive_DMA(AT_Channel, r_buffer, BUFFER_SIZE) != HAL_OK)
        return HAL_ERROR; 
    

    if((status = HAL_UART_Transmit(AT_Channel, cmd, cmd_len, 100)) != HAL_OK)
        return status;


    if((status = Read_AT_response(AT_Channel,r_buffer)) != HAL_OK)
        return status;
}

HAL_StatusTypeDef 
Send_AT_Command_With_Ready_Detect(
    UART_HandleTypeDef* AT_Channel,uint8_t* r_buffer,const uint8_t* const cmd,const size_t cmd_len){
    
    HAL_StatusTypeDef status = HAL_OK;

    if(AT_Channel == NULL || r_buffer == NULL)
        return HAL_ERROR;

    if(cmd == NULL && cmd_len != 0)
        return HAL_ERROR;
    
    if (HAL_UART_Receive_DMA(AT_Channel, r_buffer, BUFFER_SIZE) != HAL_OK)
        return HAL_ERROR; 
    

    if((status = HAL_UART_Transmit(AT_Channel, cmd, cmd_len, 100)) != HAL_OK)
        return status;


    if((status = Detect_NearLink_ready(AT_Channel,r_buffer)) != HAL_OK)
        return status;
}
