# STM32F4 + MAX30102（心率/血氧）采集与串口 DMA 发送（Core）

本仓库包含一个 STM32F4 HAL 工程的 `Core/` 目录源码：
- 使用**软件 I2C**读取 MAX30102 FIFO 数据
- 在**定时器中断**中周期性处理红光/红外数据，计算心率（HR）与血氧（SpO2）
- 通过 **USART1 + DMA** 发送简易数据包到上位机

> 说明：当前工作区仅包含 `Core/Inc` 与 `Core/Src`。若你要直接在 STM32CubeIDE/Keil 编译，请确保仓库中同时包含 CubeMX 生成的完整工程（如 `Drivers/`、`.ioc`、启动文件等）。

## 功能概览

- MAX30102
  - FIFO 读取：从器件地址 `0x57` 读取寄存器 `0x07`，每次 6 字节（red/ir 各 3 字节）
  - 数据处理：缓存一批数据后执行滤波、AC/DC 分离、波峰检测、心率/血氧计算、滑动平均
- 定时器
  - 在 `HAL_TIM_PeriodElapsedCallback` 中完成 FIFO 读取与数据处理
  - 每累计 200 次中断发送一次数据包（代码注释为 `200 * 10ms = 2s`，实际周期取决于 CubeMX 中 TIM 配置）
- 串口 DMA
  - USART1 使用 `HAL_UART_Transmit_DMA` 发送固定 6 字节数据包

## 目录结构（Core）

- `Inc/`：头文件
- `Src/`：源文件

与业务相关的 BSP 文件：
- `Inc/bsp_soft_i2c.h` + `Src/bsp_soft_i2c.c`：软件 I2C（GPIO bit-bang）
- `Inc/bsp_ir_red_cal2.h` + `Src/bsp_ir_red_cal2.c`：MAX30102 数据处理
- `Inc/bsp_tim.h` + `Src/bsp_tim.c`：定时器封装与周期任务
- `Inc/bsp_usart_dma.h` + `Src/bsp_usart_dma.c`：USART1 DMA 发送与打包
- `Inc/bsp_adc_dma.h` + `Src/bsp_adc_dma.c`：ADC DMA（如工程中有使用）

## 硬件连接（默认）

### MAX30102（软件 I2C）
软件 I2C 引脚在 `Inc/bsp_soft_i2c.h` 中定义：
- SCL：`GPIOB Pin 3`
- SDA：`GPIOB Pin 6`

MAX30102 常见连接：
- VCC → 3.3V
- GND → GND
- SDA/SCL → 对应 GPIO（外部上拉电阻视模块情况而定）

### 串口（USART1）
- USART1 TX/RX 连接到 USB-TTL 或上位机串口
- 使用 DMA 发送（固定 6 字节帧）

## 串口数据协议

数据包格式在 `Inc/bsp_usart_dma.h` 中定义：

| Byte0 | Byte1 | Byte2 | Byte3 | Byte4 | Byte5 |
|------:|------:|------:|------:|------:|------:|
| 0xAA  | 0x55  | HR    | SpO2  | CHK   | 0xBB  |

- `HR`：平滑后的心率（`g_vital_signs.smooth_hr`）
- `SpO2`：平滑后的血氧（`g_vital_signs.smooth_spo2`）
- `CHK`：简单累加校验（`Byte0..Byte3` 累加，8-bit 溢出丢弃高位）

## 运行流程（代码层面）

入口在 `Src/main.c`：
1. HAL 初始化与外设初始化（CubeMX 生成）
2. 调用 `HR_Device_Init(...)` 初始化 MAX30102（寄存器与参数由 `main.c` 中数组给出）
3. `BSP_TIM_Init(&htim1)` 绑定定时器句柄并 `BSP_TIM_Start_IT()` 启动中断
4. 定时器中断回调中：
   - `My_SI2C_ReadDatas(0x57, 0x07, fifo_buffer, 6)` 读取 FIFO
   - `MAX30102_ProcessData(fifo_buffer)` 处理数据
   - 每累计 200 次中断调用 `Send_DataPacket()` 发送串口包

## 编译与烧录

推荐 STM32CubeIDE：
1. 使用 STM32CubeMX 生成/打开完整工程
2. 将本仓库的 `Core/` 覆盖或合并到工程对应的 `Core/`
3. 确认外设配置：
   - USART1 已启用且配置 DMA TX
   - TIM1 已启用并按需要设置中断周期（代码注释假设约 10ms）
   - GPIO 已启用（软件 I2C 使用的 PB3/PB6）
4. 编译并下载到板子

## 上位机解析建议

- 以帧头 `AA 55` 定位
- 固定长度 6 字节
- 验证尾字节 `BB` 与校验 `CHK`

## 注意事项

- 软件 I2C 需要较稳定的时序与上拉条件；如通信不稳定，优先检查 SDA/SCL 上拉、GPIO 配置与 `I2C_Delay_us()` 延时。
- 采样与发送周期由 TIM 配置决定；若要更快/更慢发送，修改定时器周期或 `bsp_tim.c` 中计数阈值。
- 本工程的 MAX30102 算法参数（采样率、滤波窗口、波峰间隔等）在 `Inc/bsp_ir_red_cal2.h` 中定义，可按需要调整。

