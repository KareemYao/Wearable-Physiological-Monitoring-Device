#ifndef AT_SLE_MODE_H
#define AT_SLE_MODE_H

#include "AT_Header.h"

typedef enum {
    Slave_Mode    = 0,
    Master_Mode   = 1,
    iBeacon_Mode  = 2,
    SLE_Closed    = 9,
    Prase_Error   = 10,
    Readin_Error  = 11,
}SLE_Device_Mode; 

SLE_Device_Mode 
Get_SLE_Device_Mode(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);


HAL_StatusTypeDef 
Set_SLE_Device_Mode(
    SLE_Device_Mode mode,
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);



#endif // AT_SLE_MODE_H
