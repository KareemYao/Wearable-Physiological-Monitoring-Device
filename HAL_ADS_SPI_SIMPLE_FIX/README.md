# ADS1292 ECG Data Acquisition (STM32G0 + HAL)

基于 STM32G0 系列 + HAL 库驱动 ADS1292 采集双通道 ECG 数据，通过 USART1 DMA 发送。

## 硬件连接

| ADS1292 | STM32G0 |
|---------|---------|
| SCLK    | PA5     |
| MISO    | PA6     |
| MOSI    | PA7     |
| CS      | PB0     |
| DRDY    | PB1     |
| START   | PB2     |
| RESET   | PB10    |

USART1 TX → PA9，波特率 115200。

## 功能说明

- ADS1292 配置：500 SPS，增益 6，开启内部参考电压及 RLD
- DRDY 下降沿触发外部中断，主循环读取 9 字节数据并解析双通道 24 位补码
- 数据格式：`CH1,CH2\r\n`，通过 USART1 DMA 发送

## 文件结构

```
Core/
├── Inc/
│   ├── bsp_ads1292.h       # ADS1292 驱动接口
│   └── bsp_usart_dma.h     # USART DMA 发送接口
└── Src/
    ├── bsp_ads1292.c       # ADS1292 SPI 驱动实现
    ├── bsp_usart_dma.c     # USART DMA 发送实现
    └── main.c              # 主程序
```

## 开发环境

- IDE: STM32CubeIDE
- HAL 库: STM32G0xx HAL
- 芯片: STM32G0 系列
