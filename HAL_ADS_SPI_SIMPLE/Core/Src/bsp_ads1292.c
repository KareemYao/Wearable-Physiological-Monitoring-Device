#include "bsp_ads1292.h"
#include "spi.h"

/* 引脚定义 */
#define ADS1292_CS_PORT    GPIOB
#define ADS1292_CS_PIN     GPIO_PIN_0
#define ADS1292_DRDY_PORT  GPIOB
#define ADS1292_DRDY_PIN   GPIO_PIN_1
#define ADS1292_START_PORT GPIOB
#define ADS1292_START_PIN  GPIO_PIN_2
#define ADS1292_RESET_PORT GPIOB
#define ADS1292_RESET_PIN  GPIO_PIN_10

/* 引脚控制宏 */
#define SPI_CS_LOW()         HAL_GPIO_WritePin(ADS1292_CS_PORT, ADS1292_CS_PIN, GPIO_PIN_RESET)
#define SPI_CS_HIGH()        HAL_GPIO_WritePin(ADS1292_CS_PORT, ADS1292_CS_PIN, GPIO_PIN_SET)
#define ADS1292_START_LOW()  HAL_GPIO_WritePin(ADS1292_START_PORT, ADS1292_START_PIN, GPIO_PIN_RESET)
#define ADS1292_START_HIGH() HAL_GPIO_WritePin(ADS1292_START_PORT, ADS1292_START_PIN, GPIO_PIN_SET)
#define ADS1292_RESET_LOW()  HAL_GPIO_WritePin(ADS1292_RESET_PORT, ADS1292_RESET_PIN, GPIO_PIN_RESET)
#define ADS1292_RESET_HIGH() HAL_GPIO_WritePin(ADS1292_RESET_PORT, ADS1292_RESET_PIN, GPIO_PIN_SET)
#define Read_DRDY_Pin()      HAL_GPIO_ReadPin(ADS1292_DRDY_PORT, ADS1292_DRDY_PIN)

extern SPI_HandleTypeDef hspi1;

/* 发送一个字节 */
void SPI_Transmit_Byte(uint8_t data)
{
    HAL_SPI_Transmit(&hspi1, &data, 1, HAL_MAX_DELAY);
}

/* 接收一个字节（发送 dummy 以产生时钟） */
uint8_t SPI_Receive_Byte(uint8_t dummy_data)
{
    uint8_t rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &dummy_data, &rx_data, 1, HAL_MAX_DELAY);
    return rx_data;
}

/* 微秒延时（基于 CPU 空循环，Cortex-M0+ 约 3 周期/循环） */
void Delay_us(uint32_t us)
{
    uint32_t delay = (SystemCoreClock / 1000000 / 3) * us;
    while (delay--) {
        __NOP();
    }
}

/* 写单个寄存器 */
void ADS1292_Write_Register(uint8_t reg_addr, uint8_t data)
{
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x40 | (reg_addr & 0x1F)); // 写命令 + 地址
    Delay_us(2);
    SPI_Transmit_Byte(0x00); // 写 1 个寄存器
    Delay_us(2);
    SPI_Transmit_Byte(data);
    SPI_CS_HIGH();
    Delay_us(5);
}

/* 启动连续采集 */
void ADS1292_Start_Continuous_Conversion(void)
{
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x08); // START
    SPI_CS_HIGH();
    Delay_us(10);

    SPI_CS_LOW();
    SPI_Transmit_Byte(0x10); // RDATAC
    SPI_CS_HIGH();
}

/* 停止连续采集 */
void ADS1292_Stop_Continuous_Conversion(void)
{
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x11); // SDATAC
    SPI_CS_HIGH();
    Delay_us(10);

    SPI_CS_LOW();
    SPI_Transmit_Byte(0x0A); // STOP
    SPI_CS_HIGH();
}

/* 初始化配置 */
void ADS1292_Init_Config(void)
{
    // 硬件复位
    ADS1292_RESET_HIGH();
    HAL_Delay(100);
    ADS1292_RESET_LOW();
    HAL_Delay(1);
    ADS1292_RESET_HIGH();
    HAL_Delay(100);

    // 停止默认连续读取模式（必须，否则写寄存器无效）
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x11); // SDATAC
    SPI_CS_HIGH();
    HAL_Delay(5);

    ADS1292_Write_Register(0x01, 0x02);  // CONFIG1: 500 SPS
    ADS1292_Write_Register(0x02, 0xE0);  // CONFIG2: 开启内部参考电压缓冲
    HAL_Delay(150);                       // 等待参考电压稳定

    ADS1292_Write_Register(0x04, 0x00);  // CH1SET: 增益6，正常输入
    ADS1292_Write_Register(0x05, 0x00);  // CH2SET: 增益6，正常输入
    ADS1292_Write_Register(0x06, 0x2F);  // RLD_SENS: 开启 RLD 及共模提取
    ADS1292_Write_Register(0x09, 0x02);  // RESP1: ADS1292 非 R 版必须写 0x02
    ADS1292_Write_Register(0x0A, 0x83);  // RESP2: CALIB_ON=1, RLDREF_INT=1, Bit0=1

    // 偏移校准（CALIB_ON 已使能）
    SPI_CS_LOW();
    SPI_Transmit_Byte(0x1A); // OFFSETCAL
    SPI_CS_HIGH();
    HAL_Delay(100);
}

/* 读取 ECG 数据（在 DRDY 下降沿中断中调用） */
void ADS1292_Read_ECG_Data(int32_t *ch1_ecg_data, int32_t *ch2_data)
{
    uint8_t spi_rx_buf[9];

    SPI_CS_LOW();
    for (int i = 0; i < 9; i++) {
        spi_rx_buf[i] = SPI_Receive_Byte(0x00);
    }
    SPI_CS_HIGH();

    // 跳过 3 字节状态字，拼接通道数据（24 位补码 → 32 位有符号）
    *ch1_ecg_data = (spi_rx_buf[3] << 16) | (spi_rx_buf[4] << 8) | spi_rx_buf[5];
    if (*ch1_ecg_data & 0x800000) {
        *ch1_ecg_data |= 0xFF000000;
    }

    *ch2_data = (spi_rx_buf[6] << 16) | (spi_rx_buf[7] << 8) | spi_rx_buf[8];
    if (*ch2_data & 0x800000) {
        *ch2_data |= 0xFF000000;
    }
}
