#ifndef AT_HEADER_H
#define AT_HEADER_H


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "stm32g0xx_hal.h" // replace this lib in need!
#include "usart.h"

#define BUFFER_SIZE_ALLOC (1024+1) // 1kb memory allocation application
#define BUFFER_SIZE (BUFFER_SIZE_ALLOC*3/4) // the boundary for buffer 

/**
 * Relay on this Macro time functions:
 * @ref Send_AT_Test
 * @ref Get_SLE_Mac_Address
 * @ref Get_SLE_Device_Mode
 * @ref Set_SLE_Device_Mode
 * 
 * */ 
#define WAIT_TIME_MS (3000)
/**
 * Relay on this Macro time functions:
 * 
 * @ref Soft_Reset_NearLink
 * -> @ref Send_AT_Command_With_Ready_Detect
 * @ref Restore_Default_Settings
 * -> @ref Send_AT_Command_With_Ready_Detect
 * */ 
#define RESTART_WAIT_TIME_MS (2*WAIT_TIME_MS) 

#endif // AT_HEADER_H
