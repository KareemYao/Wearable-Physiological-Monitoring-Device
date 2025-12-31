/*
 * bsp_adc_dma.c
 * ADC DMA 缓冲、滤波与打印实现
 */

#include "bsp_adc_dma.h"
#include "stdio.h"
#include "string.h"
#include "math.h"

volatile uint16_t BSP_RAM_Rece[BSP_RAM_RECE_SIZE] = {0};

static float Ther_V = 0.0f;
static float Bat_V = 0.0f;

/* 启动 ADC DMA 传输 */
void BSP_ADC_StartDMA(ADC_HandleTypeDef *hadc)
{
    HAL_ADC_Start_DMA(hadc, (uint32_t *)BSP_RAM_Rece, BSP_RAM_RECE_SIZE);
}

/* 处理 ADC 数据并通过串口打印 */
void BSP_ADC_ProcessAndPrint(UART_HandleTypeDef *huart, uint32_t timeout_ms)
{
    uint32_t sum_ch1 = 0;
    uint32_t sum_ch2 = 0;
    uint16_t idx = 0;

    /* 计算通道平均值 (交错存储: ch1, ch2, ch1, ch2...) */
    for (uint8_t i = 0; i < (BSP_RAM_RECE_SIZE / 2); i++)
    {
        idx = (uint16_t)(i * 2 + 0);
        sum_ch1 += BSP_RAM_Rece[idx];
        idx = (uint16_t)(i * 2 + 1);
        sum_ch2 += BSP_RAM_Rece[idx];
    }

    /* 转换为电压值 */
    Ther_V = (float)sum_ch1 / (float)(BSP_RAM_RECE_SIZE / 2) * 3.3f / 4095.0f;
    Bat_V = (float)sum_ch2 / (float)(BSP_RAM_RECE_SIZE / 2) * 3.3f / 4095.0f;

    /* 打印温度电压 */
    char Pbuffer_ch1[64] = {0};
    char Pbuffer_ch2[64] = {0};
    char errbuf[64];
    HAL_StatusTypeDef uart_ret;

    snprintf(Pbuffer_ch1, sizeof(Pbuffer_ch1), "Temp=%.3f\r\n", Ther_V);
    uart_ret = HAL_UART_Transmit(huart, (uint8_t *)Pbuffer_ch1, strlen(Pbuffer_ch1), timeout_ms);

    if (uart_ret != HAL_OK)
    {
        snprintf(errbuf, sizeof(errbuf), "UART ret=%d\r\n", (int)uart_ret);
        HAL_UART_Transmit(huart, (uint8_t *)errbuf, strlen(errbuf), timeout_ms);
        return;
    }

    snprintf(Pbuffer_ch2, sizeof(Pbuffer_ch2), "Volat=%.3f\r\n", Bat_V);
    uart_ret = HAL_UART_Transmit(huart, (uint8_t *)Pbuffer_ch2, strlen(Pbuffer_ch2), timeout_ms);
    
    if (uart_ret != HAL_OK)
    {
        snprintf(errbuf, sizeof(errbuf), "UART ret=%d\r\n", (int)uart_ret);
        HAL_UART_Transmit(huart, (uint8_t *)errbuf, strlen(errbuf), timeout_ms);
    }
}

float BSP_ADC_GetTemp(void)
{
    return Ther_V;
}

float BSP_ADC_GetVolat(void)
{
    return Bat_V;
}
