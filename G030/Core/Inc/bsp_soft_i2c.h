/**
 * @file  bsp_soft_i2c.h
 * @brief 软件 I2C 驱动接口
 */
#ifndef BSP_SOFT_I2C_H
#define BSP_SOFT_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32g0xx_hal.h"

/* Defines -------------------------------------------------------------------*/
#define I2C_SCL_PIN         GPIO_PIN_3
#define I2C_SCL_PORT        GPIOB
#define I2C_SDA_PIN         GPIO_PIN_6
#define I2C_SDA_PORT        GPIOB

#define I2C_SCL_HIGH()      HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET)
#define I2C_SCL_LOW()       HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET)
#define I2C_SDA_HIGH()      HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET)
#define I2C_SDA_LOW()       HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET)
#define I2C_SDA_READ()      HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN)

/* Exported variables --------------------------------------------------------*/
extern uint8_t fifo_buffer[6];

/* Exported functions --------------------------------------------------------*/
void    I2C_Delay_us(void);
void    I2C_Start(void);
void    I2C_Stop(void);
void    I2C_Send_Ack(uint8_t ack);
uint8_t I2C_Receive_Ack(void);
void    I2C_Send_Byte(uint8_t data);
uint8_t I2C_Receive_Byte(uint8_t ack);

int     My_SI2C_WriteData(uint8_t Addr, const uint8_t reg_addr, uint8_t data);
int     My_SI2C_ReadData(uint8_t Addr, const uint8_t reg_addr, uint8_t ack);
void    HR_Device_Init(uint8_t Addr, const uint8_t *reg_addrs, uint8_t *datas, uint8_t count);
uint8_t My_SI2C_ReadDatas(uint8_t Addr, const uint8_t reg_addr, uint8_t *fifo_buffer, uint8_t FIFO_len);

#ifdef __cplusplus
}
#endif

#endif /* BSP_SOFT_I2C_H */

