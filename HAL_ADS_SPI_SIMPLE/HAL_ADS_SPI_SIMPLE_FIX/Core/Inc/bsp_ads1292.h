/**
 * @file  bsp_ads1292.h
 * @brief ADS1292 ECG 传感器驱动接口
 */
#ifndef __BSP_ADS1292_H
#define __BSP_ADS1292_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Defines -------------------------------------------------------------------*/
#define ADS1292_DRDY_PIN      GPIO_PIN_1  /* DRDY 引脚 (供 main.c 中断回调使用) */

/* Exported functions --------------------------------------------------------*/
void ADS1292_Init_Config(void);
void ADS1292_Start_Continuous_Conversion(void);
void ADS1292_Stop_Continuous_Conversion(void);
void ADS1292_Read_ECG_Data(int32_t *ch1_ecg_data, int32_t *ch2_data);
void ADS1292_Write_Register(uint8_t reg_addr, uint8_t data);
uint8_t ADS1292_Read_Register(uint8_t reg_addr);

/* DSP Filter functions */
int32_t Notch_Filter(int32_t x);
int32_t HPF_DCBlock(int32_t x);
int32_t FIR_LPF_Filter(int32_t x);
int32_t RLD_Noise_Cancel(int32_t ch1, int32_t ch2);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_ADS1292_H */
