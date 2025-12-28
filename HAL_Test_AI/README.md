# MAX30102 原始数据采集与高速传输模块

## 1. 项目简介
本代码模块实现了 **MAX30102 传感器** 的原始数据采集，并通过 **STM32F4 UART DMA** 方式将数据实时传输至上位机。该模块专门为大模型训练 (LLM Training) 提供高频、原始的红光 (Red) 与红外光 (IR) 采样数据。

## 2. 硬件连接 (Hardware Connection)

| 信号名称 | STM32 引脚 | GPIO 模式 | 备注 |
| :--- | :--- | :--- | :--- |
| I2C_SCL | PB6 | Output Open-Drain | 软件模拟 I2C 时钟 |
| I2C_SDA | PB3 | Output Open-Drain | 软件模拟 I2C 数据 |
| UART_TX | PA9 | Alternate Function | 串口 1 发送 (使用 DMA2_Stream7) |
| UART_RX | PA10 | Alternate Function | 串口 1 接收 |

> **注意**: 软件 I2C 引脚已配置为开漏模式并启用内部上拉，若信号不稳定请外接 4.7kΩ 上拉电阻。

## 3. 软件架构说明
*   **采集触发**: 使用 TIM1 定时器中断。每进入一次中断，读取 10 个样本点。
*   **底层驱动**: `bsp_soft_i2c.c` 实现了对 MAX30102 (地址 0x57) 的寄存器读写。
*   **非阻塞传输**: 串口发送采用 DMA 模式。`bsp_usart.c` 中维护了 `tx_complete` 标志位，确保在高速传输时数据包不会发生重叠或丢失。

## 4. 通信协议 (Communication Protocol)
*   **波特率**: 460800 (请确保上位机支持此高波特率)
*   **单条数据包长度**: 10 字节

### 包结构:

| 偏移 | 长度 | 内容 | 说明 |
| :--- | :--- | :--- | :--- |
| 0 | 1 Byte | 0xAA | 帧头 1 |
| 1 | 1 Byte | 0x55 | 帧头 2 |
| 2-4 | 3 Bytes | Red_Raw | 红光原始数据 (大端序: MSB -> Mid -> LSB) |
| 5-7 | 3 Bytes | IR_Raw | 红外原始数据 (大端序: MSB -> Mid -> LSB) |
| 8 | 1 Byte | CheckSum | 校验和 (Byte 0-7 累加) |
| 9 | 1 Byte | 0xBB | 帧尾 |

## 5. 🔴 重要：数据预处理说明 (必读)
**供算法组同学参考**：本代码为了保证模型训练的原始性，发送的是从传感器寄存器直接读取的 **24-bit 原始映射数据**。但 MAX30102 的 ADC 有效分辨率最大为 18-bit。

### 关键处理步骤：
传感器内部数据是左对齐的。当你合成 24-bit 整数后，必须将其向右移 6 位，才能得到真实的采样数值。

**计算公式**: 
$$Value_{real} = (Byte_{High} \ll 16 | Byte_{Mid} \ll 8 | Byte_{Low}) \gg 6$$

### Python 解析示例：
```python
def process_max30102_raw(payload_3bytes):
    # 合并为 24位 整数
    raw_24bit = (payload_3bytes[0] << 16) | (payload_3bytes[1] << 8) | payload_3bytes[2]
    # 右移 6 位得到 18位 有效数据
    real_val = raw_24bit >> 6
    return real_val

# 假设收到红光部分字节为: [0x03, 0xF1, 0x40]
# 真实值 = 0x03F140 >> 6 = 4037
```

## 6. 文件清单
*   `bsp_soft_i2c.c/h`: 软件 I2C 底层实现。
*   `bsp_usart.c/h`: DMA 串口封包发送逻辑。
*   `bsp_tim.c/h`: 定时器中断业务逻辑（数据采集调度）。
*   `stm32f4xx_it.c`: 中断向量入口（已添加 TIM1 和 USART1 的处理）。
*   `main.c`: 初始化外设及 MAX30102 配置。

## 7. 集成注意事项
*   **DMA 中断**: 必须保持 `DMA2_Stream7_IRQHandler` 的开启，否则发送标志位无法复位。
*   **配置修改**: 如需更改采样频率，请调整 `tim.c` 中的 `Prescaler` 或 `Period`。
