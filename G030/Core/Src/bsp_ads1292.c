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
 * 50Hz IIR Notch Filter — 级联两个二阶节 (4 阶)
 * fs = 500Hz, f0 = 50Hz, Q = 5
 *
 * 设计参数:
 *   w0 = 2*pi*50/500 = 0.62831853072
 *   cos(w0) = 0.80901699437
 *   alpha = sin(w0) / (2*Q) = 0.58778525229 / 10 = 0.05877852523
 *
 * 归一化系数 (除以 a0 = 1+alpha = 1.05877852523):
 *   b0/a0 =  0.94447910542
 *   b1/a0 = -1.52862148028
 *   b2/a0 =  0.94447910542
 *   a1/a0 = -1.52862148028
 *   a2/a0 =  0.88895821084
 *
 * Q24 定点 (乘以 16777216):
 */
#define NOTCH_B0    15845665L       /*  0.94447910542 */
#define NOTCH_B1  (-25647906L)      /* -1.52862148028 */
#define NOTCH_B2    15845665L       /*  0.94447910542 */
#define NOTCH_A1  (-25647906L)      /* -1.52862148028 */
#define NOTCH_A2    14914113L       /*  0.88895821084 */
#define QN_SHIFT    24

typedef struct {
    int32_t x1, x2, y1, y2;
} NotchState;

static NotchState ns1 = {0}, ns2 = {0};

static int32_t Notch_Stage(NotchState *s, int32_t x0)
{
    int64_t y0 = (int64_t)NOTCH_B0 * x0
               + (int64_t)NOTCH_B1 * s->x1
               + (int64_t)NOTCH_B2 * s->x2
               - (int64_t)NOTCH_A1 * s->y1
               - (int64_t)NOTCH_A2 * s->y2;
    y0 >>= QN_SHIFT;

    s->x2 = s->x1;
    s->x1 = x0;
    s->y2 = s->y1;
    s->y1 = (int32_t)y0;

    return (int32_t)y0;
}

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

/* 启动连续采集 */
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

/* 停止连续采集 */
void ADS1292_Stop_Continuous_Conversion(void)
{
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x11);   /* SDATAC */
    SPI_CS_HIGH();
    Delay_us(10);

    SPI_CS_LOW();
    SPI_Transmit_Byte(0x0A);   /* STOP */
    SPI_CS_HIGH();
}

/* 初始化配置 */
void ADS1292_Init_Config(void)
{
    /* 硬件复位: 低电平脉冲至少需要 t_RST_PL (数据手册要求 >2 tCLK) */
    ADS1292_RESET_HIGH();
    HAL_Delay(100);
    ADS1292_RESET_LOW();
    HAL_Delay(10);
    ADS1292_RESET_HIGH();
    HAL_Delay(150);             /* 复位后等待内部振荡器稳定 */

    /* 停止默认连续读取模式 (必须, 否则写寄存器无效) */
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x11);   /* SDATAC */
    SPI_CS_HIGH();
    HAL_Delay(10);

    /* CONFIG1 (0x01): 500 SPS
     * Bit7=0(连续转换), Bit[2:0]=010 -> 500 SPS
     * 500SPS 带宽~125Hz, 能完整采集 ECG 的 QRS 波(~40Hz) */
    ADS1292_Write_Register(0x01, 0x02);

    /* CONFIG2 (0x02): 内部参考电压开启, 导联脱落比较器关闭
     * Bit5=PDB_REFBUF=1(内部参考使能), Bit4=VREF_4V=0(2.42V 参考) */
    ADS1292_Write_Register(0x02, 0xA0);
    HAL_Delay(150);             /* 内部参考电压启动稳定 */

    /* CH1SET (0x04): 增益 6, 正常输入
     * 增益 6 是默认值, ECG 信号~1mV, 满量程 +-VREF/6 约 +-0.67V */
    ADS1292_Write_Register(0x04, 0x00);

    /* CH2SET (0x05): 增益 6, MUX=RLD_MEASURE */
    ADS1292_Write_Register(0x05, 0x02);

    /* RLD_SENS (0x06): PDB_RLD=1, CH1 正负端路由到 RLD */
    ADS1292_Write_Register(0x06, 0x23);

    /* RESP1 (0x09): 关闭呼吸阻抗检测 */
    ADS1292_Write_Register(0x09, 0x02);

    /* RESP2 (0x0A): 内部参考驱动 RLD + 校准使能 */
    ADS1292_Write_Register(0x0A, 0x83);

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

/* 50Hz 陷波滤波器 (级联两个二阶节) */
int32_t Notch_Filter(int32_t x)
{
    x = Notch_Stage(&ns1, x);
    x = Notch_Stage(&ns2, x);
    return x;
}
