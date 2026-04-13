#ifndef AT_SLE_CONFIG_H
#define AT_SLE_CONFIG_H

#include "AT_Header.h"

#include "AT_SLE_mode.h"
#include "AT_SLE_Mac.h"

HAL_StatusTypeDef 
Get_SLE_Status(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);


HAL_StatusTypeDef 
Get_SLE_Device_Name(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);


HAL_StatusTypeDef 
Set_SLE_Device_Name(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);


#endif // AT_SLE_CONFIG_H
