/**
 * @file  bsp_adc_dma.c
 * @brief ADC DMA 采集及电池监测实现
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_adc_dma.h"

/* Private variables ---------------------------------------------------------*/
volatile uint16_t BSP_RAM_Rece[BSP_RAM_RECE_SIZE] = {0};

static float Ther_V = 0.0f;             /* NTC 热敏电阻电压 */
static float Bat_V  = 0.0f;             /* 电池电压 */

/* Exported functions --------------------------------------------------------*/

/* 启动 ADC DMA 传输 */
void BSP_ADC_StartDMA(ADC_HandleTypeDef *hadc)
{
    HAL_ADC_Start_DMA(hadc, (uint32_t *)BSP_RAM_Rece, BSP_RAM_RECE_SIZE);
}

/* 处理 ADC 数据 */
void BSP_ADC_Process(void)
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
    Bat_V  = (float)sum_ch2 / (float)(BSP_RAM_RECE_SIZE / 2) * 3.3f * 2.0f / 4095.0f;
}

/* NTC 温度节点 (-10°C ~ 50°C, 每 10°C 一个) */
static const int8_t Temp_Nodes[] = {-10, 0, 10, 20, 30, 40, 50};
#define TEMP_NODE_NUM  (sizeof(Temp_Nodes) / sizeof(Temp_Nodes[0]))

/* NTC 分压电压查找表 (温度越高, 阻值越小, 电压越低) */
static const float NTC_Voltage_Table[TEMP_NODE_NUM] = {
    3.24f,  /* -10°C */
    3.20f,  /*   0°C */
    3.14f,  /*  10°C */
    3.06f,  /*  20°C */
    2.94f,  /*  30°C */
    2.78f,  /*  40°C */
    2.58f   /*  50°C */
};

/* 根据 NTC 电压查表获取温度 */
int8_t Get_Temperature(float current_voltage)
{
    /* 电压高于最低温节点, 返回最低温 */
    if (current_voltage >= NTC_Voltage_Table[0])
        return Temp_Nodes[0];

    /* 电压低于最高温节点, 返回最高温 */
    if (current_voltage <= NTC_Voltage_Table[TEMP_NODE_NUM - 1])
        return Temp_Nodes[TEMP_NODE_NUM - 1];

    /* 遍历查找最接近的温度节点 */
    for (uint8_t i = 0; i < TEMP_NODE_NUM - 1; i++)
    {
        if (current_voltage <= NTC_Voltage_Table[i] && current_voltage > NTC_Voltage_Table[i + 1])
        {
            float diff1 = NTC_Voltage_Table[i] - current_voltage;
            float diff2 = current_voltage - NTC_Voltage_Table[i + 1];
            return (diff1 < diff2) ? Temp_Nodes[i] : Temp_Nodes[i + 1];
        }
    }

    return 25; /* 默认常温 */
}

/* 电池电压节点 (4.2V 满电 ~ 3.3V 空电) */
static const float Bat_Vol_Nodes[] = {4.2f, 4.0f, 3.8f, 3.6f, 3.5f, 3.3f};
#define VOL_NODE_NUM  (sizeof(Bat_Vol_Nodes) / sizeof(Bat_Vol_Nodes[0]))

/* 电量百分比二维查找表 [温度][电压] */
static const uint8_t Battery_Capacity_Table[TEMP_NODE_NUM][VOL_NODE_NUM] = {
    /*           4.2V  4.0V  3.8V  3.6V  3.5V  3.3V */
    /* -10°C */ { 90,   50,   15,    0,    0,    0},
    /*   0°C */ { 95,   70,   40,   10,    0,    0},
    /*  10°C */ {100,   80,   55,   20,    5,    0},
    /*  20°C */ {100,   85,   60,   25,   10,    0},
    /*  30°C */ {100,   90,   65,   30,   10,    0},
    /*  40°C */ {100,   90,   65,   30,   10,    0},
    /*  50°C */ {100,   90,   65,   30,   10,    0}
};

/* 根据温度和电压查表获取电池电量百分比 */
uint8_t Get_Battery_Capacity(int8_t current_temp, float current_vol)
{
    uint8_t temp_index = 0;
    uint8_t vol_index  = 0;

    /* 锁定温度行索引 */
    for (uint8_t i = 0; i < TEMP_NODE_NUM; i++)
    {
        if (current_temp == Temp_Nodes[i])
        {
            temp_index = i;
            break;
        }
    }

    /* 锁定电压列索引 */
    if (current_vol >= Bat_Vol_Nodes[0])
    {
        vol_index = 0;
    }
    else if (current_vol <= Bat_Vol_Nodes[VOL_NODE_NUM - 1])
    {
        vol_index = VOL_NODE_NUM - 1;
    }
    else
    {
        for (uint8_t j = 0; j < VOL_NODE_NUM - 1; j++)
        {
            if (current_vol <= Bat_Vol_Nodes[j] && current_vol > Bat_Vol_Nodes[j + 1])
            {
                float diff1 = Bat_Vol_Nodes[j] - current_vol;
                float diff2 = current_vol - Bat_Vol_Nodes[j + 1];
                vol_index = (diff1 < diff2) ? j : (j + 1);
                break;
            }
        }
    }

    return Battery_Capacity_Table[temp_index][vol_index];
}

/* 电池监测任务: 读取电压, 计算电量, 控制 LED 指示 */
void Task_Battery_Monitor(void)
{
    BSP_ADC_Process();
    float ntc_vol = Ther_V;
    float bat_vol = Bat_V;

    int8_t  current_temp    = Get_Temperature(ntc_vol);
    uint8_t battery_percent = Get_Battery_Capacity(current_temp, bat_vol);

    if (battery_percent > 50)
    {
        /* >50%: 亮绿灯 */
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
    }
    else if (battery_percent >= 20)
    {
        /* 20%~50%: 全灭 (省电) */
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
    }
    else
    {
        /* <20%: 亮红灯 */
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
    }
}
