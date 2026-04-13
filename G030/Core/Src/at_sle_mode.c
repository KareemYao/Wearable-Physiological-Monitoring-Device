#include "NLsocket.h"

SLE_Device_Mode 
Get_SLE_Device_Mode(UART_HandleTypeDef* AT_Channel, uint8_t* r_buffer){
    
    SLE_Device_Mode mode = Readin_Error;

    const char msg[] = "AT+SLEMODE?\r\n";
    uint8_t* cursor = NULL;

    if(Send_AT_Command(AT_Channel,r_buffer,(uint8_t*)msg,sizeof(msg),WAIT_TIME_MS) != HAL_OK)
        return Readin_Error;
        

    if((cursor = (uint8_t*)strstr((const char*)r_buffer,"+SLEMODE :")) != NULL){
        const uint8_t offset = strlen("+SLEMODE :");
        switch (cursor[offset]){
            case '0':
                mode = Slave_Mode;
                break;
            case '1':
                mode = Master_Mode;
                break;
            case '2':
                mode = iBeacon_Mode;
                break;
            case '9':
                mode = SLE_Closed;
                break;
            default:
                mode = Prase_Error;
        }
    }
    
    return mode;
}


HAL_StatusTypeDef 
Set_SLE_Device_Mode(SLE_Device_Mode mode,UART_HandleTypeDef* AT_Channel,uint8_t* r_buffer){
    
    HAL_StatusTypeDef status = HAL_OK;
    char AT_instr[] = "AT+SLEMODE=X\r\n";


    switch (mode){
        case Slave_Mode:
            AT_instr[12] = '0';
            break;

        case Master_Mode:
            AT_instr[12] = '1';
            break;

        case iBeacon_Mode:
            AT_instr[12] = '2';
            break;

        case SLE_Closed:
            AT_instr[12] = '9';
            break; 

        default:
            return HAL_ERROR; // ERROR SYNTAX!
    }

    status = Send_AT_Command(AT_Channel,r_buffer,(uint8_t*)AT_instr,sizeof(AT_instr),WAIT_TIME_MS);
        
    return status;
}
