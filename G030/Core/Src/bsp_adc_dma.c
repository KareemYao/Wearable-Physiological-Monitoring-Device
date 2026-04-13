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

    // /* 1. 转换 NTC 引脚电压 (直接计算引脚真实电压) */
    // Ther_V = (float)sum_ntc / (float)(BSP_RAM_RECE_SIZE / 2) * 3.3f / 4095.0f;
    
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

 /* 根据 NTC 电压查表获取温度 (引入一维线性插值) */
 static int8_t Get_Temperature(float current_voltage)
 {
     /* 电压高于最低温节点 (电压最大), 返回最低温 */
     if (current_voltage >= NTC_Voltage_Table[0])
         return Temp_Nodes[0];

     /* 电压低于最高温节点 (电压最小), 返回最高温 */
     if (current_voltage <= NTC_Voltage_Table[TEMP_NODE_NUM - 1])
         return Temp_Nodes[TEMP_NODE_NUM - 1];

     /* 遍历查找并插值 */
     for (uint8_t i = 0; i < TEMP_NODE_NUM - 1; i++)
     {
         /* 注意：NTC 电压是递减排列的 */
         if (current_voltage <= NTC_Voltage_Table[i] && current_voltage > NTC_Voltage_Table[i + 1])
         {
             float vol_diff = NTC_Voltage_Table[i] - NTC_Voltage_Table[i + 1];     /* 电压跨度 (正数) */
             float temp_diff = Temp_Nodes[i + 1] - Temp_Nodes[i];                  /* 温度跨度 (10度) */
             float current_offset = NTC_Voltage_Table[i] - current_voltage;        /* 当前电压偏离低温点(高电压)的差值 */
            
             /* 插值：低温基准 + (电压偏移比例 * 温度跨度) */
             return Temp_Nodes[i] + (int8_t)((current_offset / vol_diff) * temp_diff);
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

 /* 根据温度和电压查表获取电池电量百分比 (引入二维双线性插值) */
 static uint8_t Get_Battery_Capacity(int8_t current_temp, float current_vol)
 {
     uint8_t r = 0, c = 0;
     float v_ratio = 0.0f, t_ratio = 0.0f;
     float cap_t0 = 0.0f, cap_t1 = 0.0f;
     float final_cap = 0.0f;

     /* ---------------- 1. 处理温度边界与寻找行索引 ---------------- */
     if (current_temp <= Temp_Nodes[0]) {
         current_temp = Temp_Nodes[0];
         r = 0;
     } else if (current_temp >= Temp_Nodes[TEMP_NODE_NUM - 1]) {
         current_temp = Temp_Nodes[TEMP_NODE_NUM - 1];
         r = TEMP_NODE_NUM - 2; /* 钳位在最后一个有效区间内 */
     } else {
         /* 遍历寻找当前温度所在的区间 */
         for (uint8_t i = 0; i < TEMP_NODE_NUM - 1; i++) {
             if (current_temp >= Temp_Nodes[i] && current_temp <= Temp_Nodes[i + 1]) {
                 r = i;
                 break;
             }
         }
     }

     /* ---------------- 2. 处理电压边界与寻找列索引 ---------------- */
     /* 注意：电压节点数组是降序排列的 (4.2 -> 3.3) */
     if (current_vol >= Bat_Vol_Nodes[0]) {
         current_vol = Bat_Vol_Nodes[0];
         c = 0;
     } else if (current_vol <= Bat_Vol_Nodes[VOL_NODE_NUM - 1]) {
         current_vol = Bat_Vol_Nodes[VOL_NODE_NUM - 1];
         c = VOL_NODE_NUM - 2; /* 钳位在最后一个有效区间内 */
     } else {
         /* 遍历寻找当前电压所在的区间 */
         for (uint8_t j = 0; j < VOL_NODE_NUM - 1; j++) {
             if (current_vol <= Bat_Vol_Nodes[j] && current_vol >= Bat_Vol_Nodes[j + 1]) {
                 c = j;
                 break;
             }
         }
     }

     /* ---------------- 3. 计算插值比例 (位置百分比) ---------------- */
     /* 电压偏移比例：(上限电压 - 当前电压) / (上限电压 - 下限电压) */
     if (Bat_Vol_Nodes[c] != Bat_Vol_Nodes[c + 1]) {
         v_ratio = (Bat_Vol_Nodes[c] - current_vol) / (Bat_Vol_Nodes[c] - Bat_Vol_Nodes[c + 1]);
     }

     /* 温度偏移比例：(当前温度 - 下限温度) / (上限温度 - 下限温度) */
     if (Temp_Nodes[r] != Temp_Nodes[r + 1]) {
         t_ratio = (float)(current_temp - Temp_Nodes[r]) / (float)(Temp_Nodes[r + 1] - Temp_Nodes[r]);
     }

     /* ---------------- 4. 执行双线性插值运算 ---------------- */
     /* 步骤 A (横向): 在 T0 (较低温度) 行，计算基于当前电压的虚拟电量 */
     float q00 = Battery_Capacity_Table[r][c];       /* 左上角值 */
     float q01 = Battery_Capacity_Table[r][c + 1];   /* 右上角值 */
     cap_t0 = q00 + v_ratio * (q01 - q00);

     /* 步骤 B (横向): 在 T1 (较高温度) 行，计算基于当前电压的虚拟电量 */
     float q10 = Battery_Capacity_Table[r + 1][c];     /* 左下角值 */
     float q11 = Battery_Capacity_Table[r + 1][c + 1]; /* 右下角值 */
     cap_t1 = q10 + v_ratio * (q11 - q10);

     /* 步骤 C (纵向): 在上述两个虚拟电量之间，按温度比例做最后一次插值 */
     final_cap = cap_t0 + t_ratio * (cap_t1 - cap_t0);

     /* ---------------- 5. 安全限制与输出 ---------------- */
     if (final_cap > 100.0f) return 100;
     if (final_cap < 0.0f) return 0;
    
     return (uint8_t)final_cap;
 }

///* 无温度补偿电量估算 */
///* 单维电压节点 (4.2V 满电 ~ 3.3V 空电) */
///* 结合 3.7V 3000mAh 容量型 18650 常温(25°C)放电特性优化 */
//static const float Bat_Vol_1D_Nodes[] = {4.20f, 4.00f, 3.85f, 3.75f, 3.60f, 3.50f, 3.30f};
//#define VOL_1D_NODE_NUM  (sizeof(Bat_Vol_1D_Nodes) / sizeof(Bat_Vol_1D_Nodes[0]))

///* 单维电量百分比节点 (对应上述电压) */
//static const uint8_t Bat_Cap_1D_Table[VOL_1D_NODE_NUM] = {100, 80, 60, 40, 20, 5, 0};
//static uint8_t Get_Battery_Capacity_NoTemp(float current_vol)
//{
//    /* 1. 若电压高于最高节点，直接返回满电 */
//    if (current_vol >= Bat_Vol_1D_Nodes[0])
//    {
//        return Bat_Cap_1D_Table[0];
//    }

//    /* 2. 若电压低于最低节点，直接返回空电 */
//    if (current_vol <= Bat_Vol_1D_Nodes[VOL_1D_NODE_NUM - 1])
//    {
//        return Bat_Cap_1D_Table[VOL_1D_NODE_NUM - 1];
//    }

//    /* 3. 区间查找与线性插值计算，确保电量百分比平滑过渡 */
//    for (uint8_t i = 0; i < VOL_1D_NODE_NUM - 1; i++)
//    {
//        if (current_vol <= Bat_Vol_1D_Nodes[i] && current_vol > Bat_Vol_1D_Nodes[i + 1])
//        {
//            float vol_diff = Bat_Vol_1D_Nodes[i] - Bat_Vol_1D_Nodes[i + 1];    /* 当前区间的电压差 */
//            float cap_diff = Bat_Cap_1D_Table[i] - Bat_Cap_1D_Table[i + 1];    /* 当前区间的电量差 */
//            float current_offset = current_vol - Bat_Vol_1D_Nodes[i + 1];      /* 当前电压高出下限的差值 */
//            
//            /* 插值公式：下限电量 + (电压偏移量 / 电压总跨度) * 电量总跨度 */
//            return (uint8_t)(Bat_Cap_1D_Table[i + 1] + (current_offset / vol_diff) * cap_diff);
//        }
//    }

//    return 0; /* 默认安全返回值 */
//}

/* 电池监测任务: 读取电压, 计算电量, 控制 LED 指示 */
void Task_Battery_Monitor(void)
{
    BSP_ADC_Process();
    float ntc_vol = Ther_V;
    float bat_vol = Bat_V;

    int8_t  current_temp    = Get_Temperature(ntc_vol);
    uint8_t battery_percent = Get_Battery_Capacity(current_temp, bat_vol);
//    uint8_t battery_percent = Get_Battery_Capacity_NoTemp(bat_vol); /* 无温度补偿版本 */

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
