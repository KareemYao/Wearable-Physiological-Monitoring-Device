/*
 * bsp_ir_red_cal2.c
 * MAX30102 数据处理实现
 */
#include "bsp_ir_red_cal2.h"
#include "math.h"
#include <string.h>
#include <stdlib.h>

/* Private variables ---------------------------------------------------------*/
static uint32_t raw_red_buf[BATCH_BUF_SIZE] = {0}; /* 红光原始数据缓存 */
static uint32_t raw_ir_buf[BATCH_BUF_SIZE] = {0};  /* 红外光原始数据缓存 */
static uint16_t buf_count = 0;                     /* 缓存计数 */

static uint32_t red_filtered[BATCH_BUF_SIZE] = {0};   /* 红光滤波后数据 */
static uint32_t ir_filtered[BATCH_BUF_SIZE] = {0};    /* 红外光滤波后数据 */
static float red_ac[BATCH_BUF_SIZE] = {0};            /* 红光AC分量 */
static float ir_ac[BATCH_BUF_SIZE] = {0};             /* 红外光AC分量 */

static float red_diff[BATCH_BUF_SIZE];      
static uint16_t neg_valley_idx[BATCH_BUF_SIZE];        
static float neg_valley_val[BATCH_BUF_SIZE];       
static uint8_t neg_count = 0;

static uint8_t s_hr_buf[HR_AVG_WINDOW] = {0};      /* 心率滤波缓存 */
static uint8_t s_hr_idx = 0;                       /* 心率缓存索引 */
static uint8_t s_spo2_buf[SPO2_AVG_WINDOW] = {0};  /* 血氧滤波缓存 */
static uint8_t s_spo2_idx = 0;                     /* 血氧缓存索引 */

static float red_dc_global = 0.0f;                 /* 红光全局DC值 */
static float ir_dc_global = 0.0f;                  /* 红外光全局DC值 */

/* Global variables ----------------------------------------------------------*/
FilteredVitalSigns g_vital_signs = {0};
MAX30102_Result_t result = {0};

/* 采集数据并处理AC/DC：满一批数据返回 1 */
static uint8_t MAX30102_CollectAndProcessAC(uint8_t *fifo_buffuer) {

    /* 1. 解析ADC值 */
    uint32_t red_adc = ((uint32_t)fifo_buffuer[0] << 16) |
                       ((uint32_t)fifo_buffuer[1] << 8)  |
                       (uint32_t)fifo_buffuer[2];

    uint32_t ir_adc  = ((uint32_t)fifo_buffuer[3] << 16) |
                       ((uint32_t)fifo_buffuer[4] << 8)  |
                       (uint32_t)fifo_buffuer[5];

    red_adc >>= 6; /* 取18bit有效数据 */
    ir_adc >>= 6;

    /* 2. 缓存原始数据 */
    if(buf_count < BATCH_BUF_SIZE)
    {
        raw_red_buf[buf_count] = red_adc;
        raw_ir_buf[buf_count] = ir_adc;
        buf_count++;
    }

    /* 3. 缓存满后执行处理 */
    if(buf_count >= BATCH_BUF_SIZE)
    {
        /* 3.1 3点批量滤波 */
        for(uint16_t i = 1; i < BATCH_BUF_SIZE - 1; i++)
        {
            red_filtered[i-1] = (raw_red_buf[i-1] + raw_red_buf[i] + raw_red_buf[i+1]) / FILTER_WINDOW;
            ir_filtered[i-1] = (raw_ir_buf[i-1] + raw_ir_buf[i] + raw_ir_buf[i+1]) / FILTER_WINDOW;
        }

        /* 3.2 计算全局DC */
        uint64_t red_dc_sum = 0, ir_dc_sum = 0;
        for(uint16_t i = 0; i < BATCH_BUF_SIZE - 2; i++)
        {
            red_dc_sum += red_filtered[i];
            ir_dc_sum += ir_filtered[i];
        }
        red_dc_global = (float)red_dc_sum / (BATCH_BUF_SIZE - 2);
        ir_dc_global = (float)ir_dc_sum / (BATCH_BUF_SIZE - 2);

        /* 3.3 AC分离 */ 
        for(uint16_t i = 0; i < BATCH_BUF_SIZE - 2; i++)
        {
            red_ac[i] = (float)red_filtered[i] - red_dc_global;
            ir_ac[i] = (float)ir_filtered[i] - ir_dc_global;
        }

        /* 4. 重置缓存 */
        buf_count = 0;
        memset(raw_red_buf, 0, sizeof(raw_red_buf));
        memset(raw_ir_buf, 0, sizeof(raw_ir_buf));
        return 1;
    }
    return 0;
}

/* 波峰检测：输出两个波峰索引 */
void MAX30102_DetectPeaks(uint16_t *peaks)
{
    memset(red_diff, 0, sizeof(red_diff));
    uint8_t peak_count = 0;

    /* 1. 计算AC信号一阶导 */ 
    for(uint16_t i = 1; i < BATCH_BUF_SIZE - 2; i++)
    {
        red_diff[i] = red_ac[i] - red_ac[i-1];
    }
     
    /* 2. 判定波峰：一阶导由正变负 + 最小间隔 */
    for(uint16_t i = 1; i < BATCH_BUF_SIZE - 2; i++)
    {
        if(red_diff[i] > 5 && red_diff[i+1] < 5)
        {
            if(peak_count == 0 || (i - peaks[peak_count-1]) >= MIN_PEAK_INTERVAL)
            {
                peaks[peak_count++] = i;
                if(peak_count >= 2) break;
            }
        }
    }

    /* 3. 兜底机制：基于负谷寻找 */
    if(peak_count < 2)
    {  
        neg_count = 0;
        memset(neg_valley_idx, 0, sizeof(neg_valley_idx));
        memset(neg_valley_val, 0, sizeof(neg_valley_val)); 
        
        for(uint16_t i = 0; i < BATCH_BUF_SIZE - 2; i++)
        {
            if(red_ac[i] < 0)
            {
                if(neg_count < BATCH_BUF_SIZE)
                {
                    neg_valley_idx[neg_count] = i;
                    neg_valley_val[neg_count] = red_ac[i];
                    neg_count++;
                }
            }
        }

        if(neg_count >= 2)
        {
            /* 寻找最小负谷 */
            uint8_t min1_idx = 0;
            float min1_val = neg_valley_val[0];
            for(uint8_t k = 1; k < neg_count; k++)
            {
                if(neg_valley_val[k] < min1_val)
                {
                    min1_val = neg_valley_val[k];
                    min1_idx = k;
                }
            }

            /* 寻找第二个负谷 */
            uint8_t min2_idx = 255;
            float min2_val = 0.0f;

            for(uint8_t k = 0; k < neg_count; k++)
            {
                if(k == min1_idx) continue;

                uint16_t interval = (neg_valley_idx[k] > neg_valley_idx[min1_idx]) ?
                                    (neg_valley_idx[k] - neg_valley_idx[min1_idx]) :
                                    (neg_valley_idx[min1_idx] - neg_valley_idx[k]);

                if(interval >= MIN_PEAK_INTERVAL)
                {
                    if(min2_idx == 255 || neg_valley_val[k] < min2_val)
                    {
                        min2_val = neg_valley_val[k];
                        min2_idx = k;
                    }
                }
            }

            if(min2_idx != 255)
            {
                if(neg_valley_idx[min1_idx] < neg_valley_idx[min2_idx])
                {
                    peaks[0] = neg_valley_idx[min1_idx];
                    peaks[1] = neg_valley_idx[min2_idx];
                }
                else
                {
                    peaks[0] = neg_valley_idx[min2_idx];
                    peaks[1] = neg_valley_idx[min1_idx];
                }
            }
        }
    }
}

/* 计算心率和血氧 */
void MAX30102_CalcVitalSigns(uint16_t *peaks)
{
    result.heart_rate = 0;
    result.spo2 = 0;
    result.jiange = 0; 
	
    /* 1. 计算心率 */
    uint32_t peak_interval = peaks[1] - peaks[0];
    if(peak_interval >= MIN_PEAK_INTERVAL && peak_interval <= MAX_PEAK_INTERVAL)
    {
        float interval_sec = (float)peak_interval / SAMPLE_RATE;
        result.heart_rate = (uint8_t)(60.0f / interval_sec);
        result.jiange = (uint8_t)peak_interval;
    }

    /* 2. 寻找AC最大值 */
    float red_ac_max = 0.0f;
    float ir_ac_max  = 0.0f;

    for(uint16_t i = 0; i < BATCH_BUF_SIZE - 2; i++)
    {
        if(red_ac[i] > red_ac_max) red_ac_max = red_ac[i];
        if(ir_ac[i]  > ir_ac_max)  ir_ac_max  = ir_ac[i];
    }

    /* 3. 计算SpO2 */
    float R = (red_ac_max * ir_dc_global) / (ir_ac_max * red_dc_global);
    result.spo2 = (uint8_t)(104.0f - 17.0f * R);
		
		if(result.spo2 <=90)result.spo2=90;
    if(result.spo2 >100)result.spo2=99;
		
    uint8_t new_hr   = result.heart_rate;
    uint8_t new_spo2 = result.spo2;

		
		//血氧心率校准机制
    uint8_t quiet_is_reasonable = (new_hr > 60 && new_hr < 100 && new_spo2 >= 90); //初始状态为静息状态校准指标
		//    uint8_t sport_is_reasonable = (new_hr > 40 && new_hr < 150 && new_spo2 >= 90);//初始状态为运动状态校准指标
		
if(smooth_buf.index == 0)  // 第一次
{
    if(quiet_is_reasonable && smooth_buf.averaged==0)  //averaged用于判断是否已进行过一次5组平均（0=未，1=已）
    {
        smooth_buf.heart_rate[0] = new_hr;
        smooth_buf.spo2[0] = new_spo2;
        smooth_buf.index = 1;
			  smooth_buf.averaged =1;
        // 第一次合理，直接输出本次值
    }
    else if(smooth_buf.averaged==1)
    {
       // 【关键：每次存满都用5组平均覆盖第0位置作为新基准】
            uint32_t sum_hr = 0, sum_spo2 = 0;
            for(uint8_t i = 0; i < SMOOTH_BUF_SIZE; i++)
            {
                sum_hr   += smooth_buf.heart_rate[i];
                sum_spo2 += smooth_buf.spo2[i];
            }
            smooth_buf.heart_rate[0] = (uint8_t)(sum_hr   / SMOOTH_BUF_SIZE);
            smooth_buf.spo2[0]       = (uint8_t)(sum_spo2 / SMOOTH_BUF_SIZE);
            smooth_buf.index = 1;
    }
}
else
{
    uint8_t last_hr   = smooth_buf.heart_rate[smooth_buf.index - 1];
    uint8_t last_spo2 = smooth_buf.spo2[smooth_buf.index - 1];

        if(abs((int)new_hr - (int)last_hr) <= 20 && abs((int)new_spo2 - (int)last_spo2) <= 10)
        {
            smooth_buf.heart_rate[smooth_buf.index] = new_hr;
            smooth_buf.spo2[smooth_buf.index]       = new_spo2;
            smooth_buf.index++;
            if(smooth_buf.index >= SMOOTH_BUF_SIZE) smooth_buf.index = 0;
        }
        else
        {
            new_hr   = last_hr;
            new_spo2 = last_spo2;
        }
    }

    result.heart_rate = new_hr;
    result.spo2       = new_spo2;
}

/* 心率滑动平均 */
uint8_t HR_SlidingAvgFilter(uint8_t new_hr)
{
    static uint8_t valid_count = 0;
    uint32_t sum = 0;

    s_hr_buf[s_hr_idx] = new_hr;
    s_hr_idx = (s_hr_idx + 1) % HR_AVG_WINDOW;

    if(valid_count < HR_AVG_WINDOW) valid_count++;

    for(uint8_t i = 0; i < valid_count; i++) sum += s_hr_buf[i];

    g_vital_signs.smooth_hr = (uint8_t)(sum / valid_count);
    return g_vital_signs.smooth_hr;
}

/* 血氧滑动平均 */
uint8_t SPO2_SlidingAvgFilter(uint8_t new_spo2)
{
    static uint8_t valid_count = 0;
    uint32_t sum = 0;
	
    s_spo2_buf[s_spo2_idx] = new_spo2;
    s_spo2_idx = (s_spo2_idx + 1) % SPO2_AVG_WINDOW;

    if(valid_count < SPO2_AVG_WINDOW) valid_count++;
	
    for(uint8_t i = 0; i < valid_count; i++) sum += s_spo2_buf[i];

    g_vital_signs.smooth_spo2 = (uint8_t)(sum / valid_count);
    return g_vital_signs.smooth_spo2;
}

/* FIFO 数据处理 */
void MAX30102_ProcessData(uint8_t *fifo_buffuer)
{
    uint16_t peaks[2] = {0};
    uint8_t ac_ready = 0;

    ac_ready = MAX30102_CollectAndProcessAC(fifo_buffuer);
    if(ac_ready)
    {   
        MAX30102_DetectPeaks(peaks);
        MAX30102_CalcVitalSigns(peaks);
        
        HR_SlidingAvgFilter(result.heart_rate);	
        SPO2_SlidingAvgFilter(result.spo2);
    }
}


