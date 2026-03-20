#include "NLsocket.h"

HAL_StatusTypeDef 
Send_AT_Test(UART_HandleTypeDef* AT_Channel,uint8_t* r_buffer){
    
    HAL_StatusTypeDef status = HAL_OK;
    const char msg[] = "AT\r\n";

    if((status = Send_AT_Command(AT_Channel,r_buffer,(uint8_t*)msg,sizeof(msg))) != HAL_OK)
        return status;

    if(strstr((const uint8_t*)r_buffer,"\r\nOK\r\n") != NULL)
        return HAL_OK;
    else 
        return HAL_ERROR;
}


HAL_StatusTypeDef 
Soft_Reset_NearLink(UART_HandleTypeDef* AT_Channel,uint8_t* r_buffer){

    HAL_StatusTypeDef status = HAL_OK;
    const char msg[] = "AT+RST\r\n";

    if((status = Send_AT_Command_With_Ready_Detect(AT_Channel,r_buffer,(uint8_t*)msg,sizeof(msg))) != HAL_OK)
        return status;

    /**
     * bs21_模组combo_AT通用指令_v4.18p_0.0.3.pdf:
     * 1. 指令格式和默认配置说明 
     *  1.2 启动信息
     *      (2) 客户建议检测 ready 来检测启动信息，不建议检测固件版本号
     *      和编译时间(后续版本可能 进行版本更新)
     * 
     */ 
    if(strstr((const uint8_t*)r_buffer,"\r\nready\r\n") != NULL)
        return HAL_OK;
    else 
        return HAL_ERROR;
}


HAL_StatusTypeDef 
Restore_Default_Settings(UART_HandleTypeDef* AT_Channel,uint8_t* r_buffer){
    
    HAL_StatusTypeDef status = HAL_OK;
    const char msg[] = "AT+RESTORE\r\n";

    if((status = Send_AT_Command_With_Ready_Detect(AT_Channel,r_buffer,(uint8_t*)msg,sizeof(msg))) != HAL_OK)
        return status;
    
    if(strstr((const uint8_t*)r_buffer,"\r\nOK\r\n") != NULL){
        if(strstr((const uint8_t*)r_buffer,"\r\nready\r\n") != NULL)
            return HAL_OK;
    }
    else 
        return HAL_ERROR;
}