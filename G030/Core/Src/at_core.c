#include "NLsocket.h"

#ifdef _DEBUG
#define lowbit(x) ((x)&(-x))
char tmp_log[10];
uint8_t log_index = 0;

void logger(uint32_t pri){
    if(log_index < 600)
        tmp_log[log_index++] = pri;
    else log_index = 0;
}
void var_logger(uint32_t pri){
    uint8_t k;
    for(k=0;pri>0;pri-=lowbit(pri))
        k += (uint8_t)lowbit(pri);
    if(log_index < 600)
        tmp_log[log_index++] = pri;
    else log_index = 0;
}

#else
static void logger(uint32_t pri){ (void)pri; }
#endif

int is_AT_response_end(uint8_t* buffer, uint16_t begin, uint16_t end){
    
    uint8_t* session = buffer + begin;
    uint16_t length  = end - begin + 1;
    uint8_t begin_with_CRLF = 1; // true

    if (length < 3)  // 至少 "\r\nX\r\n" => 5 字节，索引差 >=4 
        return 0;

    if (buffer[end - 1] != '\r' || buffer[end] != '\n') // 没有 "\r\n" 结尾
        return 0;

    if (buffer[begin] != '\r' || buffer[begin + 1] != '\n') 
        begin_with_CRLF = 0; // false

        
    // "\r\nOK\r\n"           <- "\r\nOK\r\n" 
    // "\r\nERROR\r\n"        <- "\r\n+<CMD>:<error_code>\r\nERROR\r\n" 
    // "Unknown cmd:...\r\n"  <- "Unknown cmd:ok\r\n"
    if ( length >= 6 &&
        !memcmp(session,"\r\nOK\r\n",6))
        return 1;
    if ( length >= 9 &&
        !memcmp(session,"\r\nERROR\r\n",9))
        return 1;
    if (!begin_with_CRLF){
        if ( length >= 12 &&
            !memcmp(session,"Unknown cmd:",12))
            return 1;
    }
    
    return 0;
}

int is_NearLink_ready(uint8_t* buffer, uint16_t begin, uint16_t end){
    
    uint8_t* session = buffer + begin;
    uint16_t length  = end - begin + 1; 
    
    logger(begin);
    logger(end);

    if (length <= 4)  // 至少 "\r\nX\r\n" => 5 字节，索引差 >=4 
        return 0;

    if (buffer[end - 1] != '\r' || buffer[end] != '\n') // 没有 "\r\n" 结尾
        return 0;

    if ( length >= 9 &&
        !memcmp(session,"\r\nready\r\n",9))
        return 1;
    
    return 0;
}

HAL_StatusTypeDef 
Read_AT_response(UART_HandleTypeDef* AT_Channel, uint8_t* r_buffer, uint32_t Timeout){
    
    if(AT_Channel == NULL || r_buffer == NULL)
        return HAL_ERROR;
    
    
    uint32_t start_time    = HAL_GetTick();
    uint16_t index_buffer  = 0;
    uint16_t session_head  = 0;
    uint32_t buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx);


    for(buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx);
        (HAL_GetTick() - start_time) <  Timeout;
        buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx)){ 
            
        
        if (buffer_length >= BUFFER_SIZE) 
            goto panic;
        
        if(index_buffer == buffer_length)
            continue;
        logger('t');
        logger((uint8_t)(HAL_GetTick() - start_time));
        logger('i');
        logger(index_buffer);
        logger('l');
        logger((uint8_t)buffer_length);
        for(; index_buffer < buffer_length; ++index_buffer){
            
            static uint8_t prev_char = 0;
            const  uint8_t rx_cursor = r_buffer[index_buffer];
            
            logger('?');
            
            if(rx_cursor == '\0')
                continue;
            logger(rx_cursor);
            
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
Detect_NearLink_ready(UART_HandleTypeDef* AT_Channel,uint8_t* r_buffer, const uint32_t Timeout){
    if(AT_Channel == NULL || r_buffer == NULL)
        return HAL_ERROR;
    
    
    uint32_t start_time    = HAL_GetTick();
    uint16_t index_buffer  = 0;
    uint16_t session_head  = 0;
    uint32_t buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx);
    
    logger(HAL_GetTick() - start_time);

    for(buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx);
        (HAL_GetTick() - start_time) < Timeout;
        buffer_length = BUFFER_SIZE_ALLOC - __HAL_DMA_GET_COUNTER((*AT_Channel).hdmarx)){ 
        
        if (buffer_length >= BUFFER_SIZE) 
            goto panic;

        logger(HAL_GetTick() - start_time);
        logger(buffer_length);
        logger(r_buffer[buffer_length]);
        for(; index_buffer < buffer_length; ++index_buffer){
            
            static uint8_t prev_char = 0;
            const  uint8_t rx_cursor = r_buffer[index_buffer];

            if(prev_char == '\r' && rx_cursor == '\n'){
                if(is_NearLink_ready(r_buffer,session_head,index_buffer)){
                    // 成功！停止 DMA 读取
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
    logger(HAL_GetTick() - start_time);
    logger(WAIT_TIME_MS);
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
    UART_HandleTypeDef* AT_Channel, uint8_t* r_buffer, const uint8_t* const cmd, const size_t cmd_len, const uint32_t Timeout){
    
    HAL_StatusTypeDef status = HAL_OK;

    if(AT_Channel == NULL || r_buffer == NULL)
        return HAL_ERROR;

    if(cmd == NULL && cmd_len != 0)
        return HAL_ERROR;
    
    if ((status = HAL_UART_Receive_DMA(AT_Channel, r_buffer, BUFFER_SIZE)) != HAL_OK)
        goto end;
    

    if((status = HAL_UART_Transmit(AT_Channel, cmd, cmd_len, 100)) != HAL_OK)
        goto end;

    if((status = Read_AT_response(AT_Channel,r_buffer,Timeout)) != HAL_OK)
        goto end;
    
end:
    HAL_UART_AbortReceive(AT_Channel);
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
    
    if ((status = HAL_UART_Receive_DMA(AT_Channel, r_buffer, BUFFER_SIZE)) != HAL_OK)
        goto end;
    

    if((status = HAL_UART_Transmit(AT_Channel, cmd, cmd_len, 100)) != HAL_OK)
        goto end;

    if((status = Detect_NearLink_ready(AT_Channel,r_buffer, RESTART_WAIT_TIME_MS)) != HAL_OK)
        goto end;
end:
    HAL_UART_AbortReceive(AT_Channel);
    return status;
}
