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
static void BSP_ADC_Process(void)
{
    uint32_t sum_ntc = 0; 
    uint32_t sum_bat = 0;
    uint16_t idx = 0;

    /* 计算通道平均值 (交错存储: IN0(NTC), IN1(BAT), IN0, IN1...) */
    for (uint8_t i = 0; i < (BSP_RAM_RECE_SIZE / 2); i++)
    {
        idx = (uint16_t)(i * 2 + 0);
        sum_ntc += BSP_RAM_Rece[idx];
        
        idx = (uint16_t)(i * 2 + 1);
        sum_bat += BSP_RAM_Rece[idx];
    }

    /* 1. 转换 NTC 引脚电压 (直接计算引脚真实电压) */
    Ther_V = (float)sum_ntc / (float)(BSP_RAM_RECE_SIZE / 2) * 3.3f / 4095.0f;
    
    /* 2. 转换电池真实电压 (ADC引脚电压 * 分压比 5.7) */
    /* 分压比 = (R69 + R68) / R68 = (47K + 10K) / 10K = 5.7f */
    Bat_V  = (float)sum_bat / (float)(BSP_RAM_RECE_SIZE / 2) * 3.3f / 4095.0f * 5.7f;
}

/* NTC 温度节点 (-10°C ~ 50°C, 每 10°C 一个) */
static const int8_t Temp_Nodes[] = {-10, 0, 10, 20, 30, 40, 50};
#define TEMP_NODE_NUM  (sizeof(Temp_Nodes) / sizeof(Temp_Nodes[0]))

/* NTC 分压电压查找表 (温度越高, 阻值越小, 电压越低) */
/* 硬件参数: VCC=3.3V, R_PullUp(R66)=10kΩ, NTC(R67)=10kΩ(B=3450K)对地 */
static const float NTC_Voltage_Table[TEMP_NODE_NUM] = {
    2.72f,  /* -10°C (计算 Rntc ≈ 46.6kΩ) */
    2.45f,  /* 0°C (计算 Rntc ≈ 28.8kΩ) */
    2.14f,  /* 10°C (计算 Rntc ≈ 18.5kΩ) */
    1.81f,  /* 20°C (计算 Rntc ≈ 12.2kΩ) */
    1.49f,  /* 30°C (计算 Rntc ≈  8.3kΩ) */
    1.21f,  /* 40°C (计算 Rntc ≈  5.7kΩ) */
    0.96f   /* 50°C (计算 Rntc ≈  4.1kΩ) */
};

/* 根据 NTC 电压查表获取温度 */
static int8_t Get_Temperature(float current_voltage)
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
/* 基于标准 3.7V/4.2V 3000mAh NCM/LCO 18650 容量型电芯放电特性优化 */
/* 硬件参考: 最大连续放电电流3A-9A, 交流内阻 30-50mΩ */
static const uint8_t Battery_Capacity_Table[TEMP_NODE_NUM][VOL_NODE_NUM] = {
    /* 4.2V   4.0V   3.8V   3.6V   3.5V   3.3V */
    /* -10°C */ { 70,    35,    10,     0,     0,     0},  /* 严寒：最大可用容量严重受限，电压急剧下降 */
    /* 0°C */ { 80,    50,    20,     5,     0,     0},  /* 偏冷：容量打八折，3.8V时电量明显虚低 */
    /* 10°C */ { 90,    65,    35,    10,     2,     0},  /* 微冷：性能开始恢复，但尾段仍容易掉电 */
    /* 20°C */ {100,    80,    50,    18,     5,     0},  /* 室温：标准参考基准线 */
    /* 30°C */ {100,    85,    55,    20,     8,     0},  /* 最适：最佳工作温度，中段放电平台最为坚挺 */
    /* 40°C */ {100,    86,    56,    22,     9,     0},  /* 偏热：内部活性极高，同电压下电量表现最好 */
    /* 50°C */ {100,    86,    56,    22,     9,     0}   /* 高热：与40度特性基本一致，不要超过60度 */
};

/* 根据温度和电压查表获取电池电量百分比 */
static uint8_t Get_Battery_Capacity(int8_t current_temp, float current_vol)
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

    if (battery_percent > 85)
    {
        /* >85%: 亮绿灯 */
        HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
    }
    else if (battery_percent >= 20)
    {
        /* 20%~85%: 全灭 (省电) */
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
