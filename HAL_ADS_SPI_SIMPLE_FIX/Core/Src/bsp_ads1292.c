/**
 * @file  bsp_ads1292.c
 * @brief ADS1292 ECG 传感器驱动实现
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_ads1292.h"
#include "spi.h"

/* Private defines -----------------------------------------------------------*/
#define ADS1292_CS_PORT     GPIOB
#define ADS1292_CS_PIN      GPIO_PIN_0
#define ADS1292_DRDY_PORT   GPIOB
#define ADS1292_DRDY_PIN    GPIO_PIN_1
#define ADS1292_START_PORT  GPIOB
#define ADS1292_START_PIN   GPIO_PIN_2
#define ADS1292_RESET_PORT  GPIOB
#define ADS1292_RESET_PIN   GPIO_PIN_10

/* 引脚控制宏 */
#define SPI_CS_LOW()            HAL_GPIO_WritePin(ADS1292_CS_PORT, ADS1292_CS_PIN, GPIO_PIN_RESET)
#define SPI_CS_HIGH()           HAL_GPIO_WritePin(ADS1292_CS_PORT, ADS1292_CS_PIN, GPIO_PIN_SET)
#define ADS1292_START_LOW()     HAL_GPIO_WritePin(ADS1292_START_PORT, ADS1292_START_PIN, GPIO_PIN_RESET)
#define ADS1292_START_HIGH()    HAL_GPIO_WritePin(ADS1292_START_PORT, ADS1292_START_PIN, GPIO_PIN_SET)
#define ADS1292_RESET_LOW()     HAL_GPIO_WritePin(ADS1292_RESET_PORT, ADS1292_RESET_PIN, GPIO_PIN_RESET)
#define ADS1292_RESET_HIGH()    HAL_GPIO_WritePin(ADS1292_RESET_PORT, ADS1292_RESET_PIN, GPIO_PIN_SET)
#define Read_DRDY_Pin()         HAL_GPIO_ReadPin(ADS1292_DRDY_PORT, ADS1292_DRDY_PIN)

/* Private variables ---------------------------------------------------------*/
extern SPI_HandleTypeDef hspi1;

/* Private functions ---------------------------------------------------------*/

/* 发送一个字节 */
static void SPI_Transmit_Byte(uint8_t data)
{
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

/* 接收一个字节 (发送 dummy 以产生时钟) */
static uint8_t SPI_Receive_Byte(uint8_t dummy_data)
{
    uint8_t rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &dummy_data, &rx_data, 1, HAL_MAX_DELAY);
    return rx_data;
}

/* 微秒延时 (基于 CPU 空循环, Cortex-M0+ 约 3 周期/循环) */
static void Delay_us(uint32_t us)
{
    uint32_t delay = (SystemCoreClock / 1000000 / 3) * us;
    while (delay--)
    {
        __NOP();
    }
}

 /*
   * ==================== 滤波器链设计 ====================
  *
  * 处理链: Raw → IIR Notch 50Hz → HPF 0.6Hz → FIR LPF 35Hz → 输出
  *
  * 各级职责:
  *   IIR Notch:  消除 50Hz 工频基波 (~-43dB at 50Hz)
  *   HPF:        去除基线漂移 (~-3dB at 0.6Hz, DC 完全去除)
  *   FIR LPF:    线性相位低通, 消除残余50Hz(-40.7dB) + 谐波(150Hz: -65.8dB)
  *
  * 50Hz 合计抑制: Notch(-43dB) + FIR(-40.7dB) ≈ -83.4 dB (实测)
  * IIR Notch 仅影响 37.5-62.5Hz, 不影响 ECG 频段 (0.5-35Hz) 的相位
  * FIR 为 ECG 频段提供严格线性相位, 保留波形形态
  */

 /*
  * --- 50Hz IIR 陷波器 (Q=2) ---
  * 仅影响 37.5-62.5Hz 附近, ECG 频段 (0.5-30Hz) 相位失真 <3°
  * Q8 系数, 输入预缩放 >>4, 所有运算 int32_t 安全
  */
 #define FILT_Q       8
 #define PRESCALE     4
 #define N_B0     223       /*  0.87109 */
 #define N_B1   (-361)      /* -1.41016 */
 #define N_B2     223       /*  0.87109 */
 #define N_A1   (-361)      /* -1.41016 */
 #define N_A2     190       /*  0.74219 */

 typedef struct {
     int32_t x1, x2, y1, y2;
 } FilterState;
 static FilterState notch_s = {0};

 /*
  * --- 0.6Hz 1阶高通 (DC blocker, 去基线漂移) ---
  * y[n] = x_s - x_prev + (alpha * y_prev) >> 8
  * alpha = 254/256 ≈ 0.992, 截止频率 ≈ 0.62Hz
  * 溢出: 254 × 524K = 133M (int32_t 安全)
  */
 #define HPF_ALPHA    254   /* Q8, cutoff ≈ 0.62Hz */
 static int32_t hpf_x_prev = 0;
 static int32_t hpf_y_prev = 0;

 /*
  * --- 51-tap FIR 线性相位低通 (35Hz cutoff, Hamming 窗) ---
  *   0-20Hz: ~0 dB, 50Hz: -40.7dB, 150Hz: -65.8dB
  *   溢出余量 30x, 群延迟 25 采样 = 50ms
  */
 #define FIR_N         51
 #define FIR_HALF      26
 #define FIR_Q         12
 #define FIR_IN_SHIFT  4

 /* 对称系数 h[0..25], h[i] = h[50-i], Q12 定标 */
 static const int16_t fir_coeff[FIR_HALF] = {
     -4,  -4,  -3,  -2,   2,   6,  12,  18,  21,  19,  11,  -5,
    -27, -52, -74, -85, -78, -47,  10,  92, 193, 302, 408, 496,
    554, 574
 };

 /* 环形缓冲区 */
 static int16_t fir_buf[FIR_N] = {0};
 static int fir_head = 0;

/* Exported functions --------------------------------------------------------*/

/* 写单个寄存器 */
void ADS1292_Write_Register(uint8_t reg_addr, uint8_t data)
{
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x40 | (reg_addr & 0x1F));   /* 写命令 + 地址 */
    Delay_us(8);
    SPI_Transmit_Byte(0x00);                        /* 写 1 个寄存器 */
    Delay_us(8);
    SPI_Transmit_Byte(data);
    SPI_CS_HIGH();
    Delay_us(5);
}

/* 读单个寄存器 */
uint8_t ADS1292_Read_Register(uint8_t reg_addr)
{
    uint8_t read_data = 0;

    SPI_CS_LOW();
    SPI_Transmit_Byte(0x20 | (reg_addr & 0x1F));   /* 读命令 + 地址 */
    Delay_us(8);
    SPI_Transmit_Byte(0x00);                        /* 读 1 个寄存器 */
    Delay_us(8);
    read_data = SPI_Receive_Byte(0x00);             /* 接收返回值 */
    SPI_CS_HIGH();
    Delay_us(5);

    return read_data;
}

/* 启动连续采集并开启RDATAC读取数据 */
void ADS1292_Start_Continuous_Conversion(void)
{
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x08);   /* START */
    SPI_CS_HIGH();
    Delay_us(10);

    SPI_CS_LOW();
    SPI_Transmit_Byte(0x10);   /* RDATAC */
    SPI_CS_HIGH();
}

/* 发送 SDATAC 并确认退出 RDATAC 模式 (最多重试 3 次) */
static uint8_t ADS1292_Exit_RDATAC(void)
{
    for (int retry = 0; retry < 3; retry++)
    {
        /* 拉高 CS 复位 SPI 状态机 */
        SPI_CS_HIGH();
        Delay_us(50);

        /* 发送 SDATAC (0x11) */
        SPI_CS_LOW();
        Delay_us(10);
        SPI_Transmit_Byte(0x11);
        SPI_CS_HIGH();
        HAL_Delay(10);          /* 等待 >>4 tCLK */

        /* 验证: 读 ID 寄存器, 正确值 bit4=1, bit[3:2]=00 */
        uint8_t id = ADS1292_Read_Register(0x00);
        if ((id & 0x1C) == 0x10)    /* bit[4:2] = 100 */
        {
            return 1;   /* 成功 */
        }
    }
    return 0;   /* 失败 */
}

/* 写寄存器并回读校验, 返回 1=成功, 0=失败 */
static uint8_t ADS1292_Write_Register_Verified(uint8_t reg_addr, uint8_t data)
{
    for (int retry = 0; retry < 3; retry++)
    {
        ADS1292_Write_Register(reg_addr, data);
        Delay_us(50);
        uint8_t readback = ADS1292_Read_Register(reg_addr);
        if (readback == data)
        {
            return 1;
        }
    }
    return 0;
}

/* 初始化配置 */
void ADS1292_Init_Config(void)
{
    /* ★ 关键: 初始化前必须拉低 START, 阻止 ADC 转换
     *   否则复位后 ADS1292 立即开始输出数据, DOUT 干扰 SPI 命令接收 */
    ADS1292_START_LOW();
    SPI_CS_HIGH();

    /* 硬件复位: 低电平脉冲至少需要 t_RST_PL (数据手册要求 >1 tMOD ≈ 7.8us) */
    ADS1292_RESET_HIGH();
    HAL_Delay(100);
    ADS1292_RESET_LOW();
    HAL_Delay(10);
    ADS1292_RESET_HIGH();
    HAL_Delay(150);             /* 复位后等待内部振荡器稳定 + POR */

    /* 退出 RDATAC 模式 (含重试和校验, 解决偶发 SDATAC 漏收问题) */
    ADS1292_Exit_RDATAC();

    /* CONFIG1 (0x01): 500 SPS
     * Bit7=0(连续转换), Bit[2:0]=010 -> 500 SPS
     * 500SPS 带宽~125Hz, 能完整采集 ECG 的 QRS 波(~40Hz) */
    ADS1292_Write_Register_Verified(0x01, 0x02);

    /* CONFIG2 (0x02): 内部参考电压开启, 导联脱落比较器关闭
     * Bit5=PDB_REFBUF=1(内部参考使能), Bit4=VREF_4V=0(2.42V 参考) */
    ADS1292_Write_Register_Verified(0x02, 0xA0);
    HAL_Delay(150);             /* 内部参考电压启动稳定 */

    /* CH1SET (0x04): 增益 12, 正常输入
     * GAIN[2:0]=110 -> PGA=12, MUX[3:0]=0000 -> 正常电极输入
     * 满量程 +-VREF/12 = +-202mV, ECG ~1mV 不会饱和
     * 相比 PGA=6, 信号幅度翻倍, 输入参考噪声从 0.9uVrms 降至 0.8uVrms */
    ADS1292_Write_Register_Verified(0x04, 0x60);

    /* CH2SET (0x05): 增益 6, MUX=RLD_MEASURE */
    ADS1292_Write_Register_Verified(0x05, 0x02);

    /* RLD_SENS (0x06): PDB_RLD=1, CH1 正负端路由到 RLD */
    ADS1292_Write_Register_Verified(0x06, 0x23);

    /* RESP1 (0x09): 关闭呼吸阻抗检测 */
    ADS1292_Write_Register_Verified(0x09, 0x02);

    /* RESP2 (0x0A): 内部参考驱动 RLD + 校准使能 */
    ADS1292_Write_Register_Verified(0x0A, 0x83);

    /* 偏移校准 */
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x1A);   /* OFFSETCAL */
    SPI_CS_HIGH();
    HAL_Delay(100);
}

/* 读取 ECG 数据 (在 DRDY 下降沿中断中调用) */
void ADS1292_Read_ECG_Data(int32_t *ch1_ecg_data, int32_t *ch2_data)
{
    uint8_t spi_rx_buf[9];

    SPI_CS_LOW();
    for (int i = 0; i < 9; i++)
    {
        spi_rx_buf[i] = SPI_Receive_Byte(0x00);
    }
    SPI_CS_HIGH();

    /* 跳过 3 字节状态字, 拼接通道数据 (24 位补码 -> 32 位有符号) */
    *ch1_ecg_data = (spi_rx_buf[3] << 16) | (spi_rx_buf[4] << 8) | spi_rx_buf[5];
    if (*ch1_ecg_data & 0x800000)
    {
        *ch1_ecg_data |= 0xFF000000;
    }

    *ch2_data = (spi_rx_buf[6] << 16) | (spi_rx_buf[7] << 8) | spi_rx_buf[8];
    if (*ch2_data & 0x800000)
    {
        *ch2_data |= 0xFF000000;
    }
}

 /* 50Hz 陷波滤波器 (Q=2, 仅影响 37.5-62.5Hz, ECG 频段安全) */
 int32_t Notch_Filter(int32_t x)
 {
     FilterState *s = &notch_s;
     int32_t x0 = x >> PRESCALE;

     int32_t y0 = N_B0 * x0
                + N_B1 * s->x1
                + N_B2 * s->x2
                - N_A1 * s->y1
                - N_A2 * s->y2;
     y0 >>= FILT_Q;

     s->x2 = s->x1;
     s->x1 = x0;
     s->y2 = s->y1;
     s->y1 = y0;

     return y0 << PRESCALE;
 }

 /* 0.6Hz 高通去基线漂移 (1阶 DC blocker) */
 int32_t HPF_DCBlock(int32_t x)
 {
     int32_t x_s = x >> PRESCALE;
     int32_t y = x_s - hpf_x_prev + ((HPF_ALPHA * hpf_y_prev) >> FILT_Q);
     hpf_x_prev = x_s;
     hpf_y_prev = y;
     return y << PRESCALE;
 }

 /* FIR 线性相位低通滤波器 (51-tap, 35Hz cutoff) */
 int32_t FIR_LPF_Filter(int32_t x)
 {
     /* 输入预缩放, 存入环形缓冲区 */
     fir_buf[fir_head] = (int16_t)(x >> FIR_IN_SHIFT);

     int32_t acc = 0;
     int p, q;

     /* 利用对称性: h[i]*(buf[n-i] + buf[n-(N-1-i)]), 乘法次数减半 */
     for (int i = 0; i < FIR_HALF - 1; i++) {
         p = fir_head - i;
         if (p < 0) p += FIR_N;
         q = fir_head - (FIR_N - 1 - i);
         if (q < 0) q += FIR_N;

         acc += (int32_t)fir_coeff[i] * ((int32_t)fir_buf[p] + (int32_t)fir_buf[q]);
     }

     /* 中间点 (无对称配对) */
     p = fir_head - (FIR_HALF - 1);
     if (p < 0) p += FIR_N;
     acc += (int32_t)fir_coeff[FIR_HALF - 1] * (int32_t)fir_buf[p];

     /* 更新头指针 */
     if (++fir_head >= FIR_N) fir_head = 0;

     /* 反缩放: Q12 定标抵消 + 补回输入预缩放 */
     return acc >> (FIR_Q - FIR_IN_SHIFT);
 }
