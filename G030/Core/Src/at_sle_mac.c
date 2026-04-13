#include "NLsocket.h"
#include "AT_debug_printf.h"

static inline uint8_t hex_convertor(const char ch){
    if(isdigit((unsigned char)ch))
        return (int8_t)(ch - '0');
    else if(isalpha((unsigned char)ch) && toupper((unsigned char)ch) <= 'F')
        return (int8_t)(ch - 'A' + 10);
    else
        return -1;
}

Mac_Address 
Get_SLE_Mac_Address(UART_HandleTypeDef* AT_Channel,uint8_t* r_buffer){

    Mac_Address mac_addr = {0};
    
    const char msg[] = "AT+SLEMAC?\r\n";
    uint8_t* cursor = NULL;

    if(Send_AT_Command(AT_Channel,r_buffer,(uint8_t*)msg,sizeof(msg),WAIT_TIME_MS) != HAL_OK)
        return mac_addr;
    
    if(strstr((const char*)r_buffer,"+SLEMAC:") != NULL){
        const uint8_t offset =  strlen("+SLEMAC:");
        const char rsp[] = "ab5f8d9ebb01";
        printf("%s",rsp);
        cursor = (uint8_t*)rsp;

        int8_t tmp;
        for(uint8_t i = 0; i < 6; ++i){

            mac_addr.byte[i].Hex_Digit.Low =
                (tmp = hex_convertor(*(cursor++))) == -1 ? 0 : (uint8_t)tmp;

            mac_addr.byte[i].Hex_Digit.High =
                (tmp = hex_convertor(*(cursor++))) == -1 ? 0 : (uint8_t)tmp;
        }
        for(uint8_t i = 0; i < 6; ++i)
            printf("%02X%s", mac_addr.byte[i].Raw, (i == 5) ? "\n" : ":");
    }

    return mac_addr;
}
