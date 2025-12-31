/*
 * bsp_soft_i2c.c
 * 软件 I2C 驱动实现
 */

#include "bsp_soft_i2c.h"

/* 微秒级延时 */
void I2C_Delay_us(void)
{
    volatile uint32_t delay = 15;
    while(delay--);
}

/* 产生 I2C 起始信号 */
void I2C_Start(void)
{
    I2C_SDA_HIGH();
    I2C_SCL_HIGH();
    I2C_Delay_us();
    I2C_SDA_LOW();
    I2C_Delay_us();
    I2C_SCL_LOW();
}

/* 产生 I2C 停止信号 */
void I2C_Stop(void)
{
    I2C_SDA_LOW();
    I2C_SCL_HIGH();
    I2C_Delay_us();
    I2C_SDA_HIGH();
    I2C_Delay_us();
}

/* 发送 I2C 应答信号 */
void I2C_Send_Ack(uint8_t ack)
{
    I2C_SCL_LOW();
    if(ack)
    {
        I2C_SDA_HIGH();
    }
    else
    {
        I2C_SDA_LOW();
    }
    I2C_Delay_us();
    I2C_SCL_HIGH();
    I2C_Delay_us();
    I2C_SCL_LOW();
    I2C_SDA_HIGH();
}

/* 接收 I2C 应答信号 */
uint8_t I2C_Receive_Ack(void)
{
    uint8_t ack;
    I2C_SCL_LOW();
    I2C_SDA_HIGH();
    I2C_Delay_us();
    I2C_SCL_HIGH();
    I2C_Delay_us();
    ack = I2C_SDA_READ();
    I2C_Delay_us();
    I2C_SCL_LOW();
    return ack;
}

/* 发送 1 字节 I2C 数据 */
void I2C_Send_Byte(uint8_t data)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        I2C_SCL_LOW();
        if(data & 0x80)
            I2C_SDA_HIGH();
        else
            I2C_SDA_LOW();
        data <<= 1;
        I2C_Delay_us();
        I2C_SCL_HIGH();
        I2C_Delay_us();
    }
    I2C_SCL_LOW();
}

/* 接收 1 字节 I2C 数据 */
uint8_t I2C_Receive_Byte(uint8_t ack)
{
    uint8_t data = 0;
    I2C_SDA_HIGH();
    for(uint8_t i = 0; i < 8; i++)
    {
        I2C_SCL_LOW();
        I2C_Delay_us();
        I2C_SCL_HIGH();
        data <<= 1;
        if(I2C_SDA_READ())
            data |= 0x01;
        I2C_Delay_us();
    }
    I2C_Send_Ack(ack);
    return data;
}

/* I2C 总线恢复 */
void I2C_Bus_Recovery(void)
{
    I2C_SDA_HIGH();
    for(uint8_t i = 0; i < 9; i++)
    {
        I2C_SCL_HIGH();
        I2C_Delay_us();
        I2C_SCL_LOW();
        I2C_Delay_us();
    }
    I2C_Start();
}

/* 写数据到 I2C 设备 */
int My_SI2C_WriteData(uint8_t Addr, const uint8_t reg_addr, uint8_t data)
{
    I2C_Start();
    I2C_Send_Byte(Addr << 1);

    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return -1;
    }
    
    I2C_Send_Byte(reg_addr);
    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return -2;
    }
    
    I2C_Send_Byte(data);
    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return -3;
    }
    I2C_Stop();
    return 0;
}

/* 从 I2C 设备读数据 */
int My_SI2C_ReadData(uint8_t Addr, const uint8_t reg_addr, uint8_t ack)
{
    uint8_t data = 0;

    I2C_Start();
    I2C_Send_Byte(Addr << 1);
    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return 1;
    }
    
    I2C_Send_Byte(reg_addr);
    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return 2;
    }

    I2C_Start();
    I2C_Send_Byte((Addr << 1) | 0x01);
    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return 3;
    }
    
    data = I2C_Receive_Byte(ack);
    I2C_Stop();
    return data;
}

/* 初始化心率设备 */
void HR_Device_Init(uint8_t Addr, const uint8_t *reg_addrs, uint8_t *datas, uint8_t count)
{
    for(uint8_t i = 0; i < count; i++)
    {
        My_SI2C_WriteData(Addr, reg_addrs[i], datas[i]);
    }
}

/* 读取多个数据 */
uint8_t My_SI2C_ReadDatas(uint8_t Addr, const uint8_t reg_addr, uint8_t *fifo_buffer, uint8_t FIFO_len)
{
    if(fifo_buffer == NULL || FIFO_len == 0)
    {
        return 4;
    }
    
    I2C_Start();
    I2C_Send_Byte(Addr << 1);
    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return 3;
    }
    
    I2C_Send_Byte(reg_addr);
    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return 2;
    }

    I2C_Start();
    I2C_Send_Byte((Addr << 1) | 0x01);
    if(I2C_Receive_Ack() == GPIO_PIN_SET)
    {
        I2C_Bus_Recovery();
        I2C_Stop();
        return 1;
    }
    
    for(uint8_t i = 0; i < FIFO_len - 1; i++)
    {
        fifo_buffer[i] = I2C_Receive_Byte(0);
    }
  
    fifo_buffer[FIFO_len - 1] = I2C_Receive_Byte(1);

    I2C_Stop();
    return 0;
}

 
 
 
