#ifndef __BSP_ADS1292_H
#define __BSP_ADS1292_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* ADS1292 DRDY 引脚定义 (供 main.c 中断回调使用) */
#define ADS1292_DRDY_PIN      GPIO_PIN_1

/* 函数声明 */
void ADS1292_Init_Config(void);
void ADS1292_Start_Continuous_Conversion(void);
void ADS1292_Stop_Continuous_Conversion(void);
void ADS1292_Read_ECG_Data(int32_t *ch1_ecg_data, int32_t *ch2_data);
void ADS1292_Write_Register(uint8_t reg_addr, uint8_t data);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADS1292_H */
