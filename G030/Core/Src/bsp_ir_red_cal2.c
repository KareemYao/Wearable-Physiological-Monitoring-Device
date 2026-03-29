/**
 * @file  bsp_ir_red_cal2.c
 * @brief MAX30102 心率血氧数据处理实现
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_ir_red_cal2.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* Private types -------------------------------------------------------------*/
/* 心率校准平滑缓冲区 (环形) */
static struct {
    uint8_t index;                          /* 当前写入位置 */
    uint8_t averaged;                       /* 是否已完成首次平均 */
    uint8_t heart_rate[SMOOTH_BUF_SIZE];
} smooth_buf = {0};

/* AC 缓存区 */
static float red_ac[BATCH_BUF_SIZE];
static float ir_ac[BATCH_BUF_SIZE];

/* 共享内存区 (生命周期错开, 复用物理内存) */
typedef union {
    /* 阶段 A: 采集与滤波 */
    struct {
        uint32_t raw_red[BATCH_BUF_SIZE];
        uint32_t raw_ir[BATCH_BUF_SIZE];
        uint32_t red_filtered[BATCH_BUF_SIZE];
        uint32_t ir_filtered[BATCH_BUF_SIZE];
    } stage_filter;

    /* 阶段 B: 寻峰与寻谷 */
    struct {
        uint8_t pos_idx[BATCH_BUF_SIZE];
        float   pos_val[BATCH_BUF_SIZE];
        uint8_t neg_idx[BATCH_BUF_SIZE];
        float   neg_val[BATCH_BUF_SIZE];
    } stage_peaks;

} ReusableMem_t;

static ReusableMem_t reuse_mem;

/* 宏定义映射 */
#define raw_red_buf     (reuse_mem.stage_filter.raw_red)
#define raw_ir_buf      (reuse_mem.stage_filter.raw_ir)
#define red_filtered    (reuse_mem.stage_filter.red_filtered)
#define ir_filtered     (reuse_mem.stage_filter.ir_filtered)

#define pos_peak_idx    (reuse_mem.stage_peaks.pos_idx)
#define pos_peak_val    (reuse_mem.stage_peaks.pos_val)
#define neg_valley_idx  (reuse_mem.stage_peaks.neg_idx)
#define neg_valley_val  (reuse_mem.stage_peaks.neg_val)

/* Private variables ---------------------------------------------------------*/
static uint16_t buf_count = 0;                      /* 缓存计数 */

static uint8_t s_hr_buf[HR_AVG_WINDOW]     = {0};   /* 心率滤波缓存 */
static uint8_t s_hr_idx  = 0;                       /* 心率缓存索引 */
static uint8_t s_spo2_buf[SPO2_AVG_WINDOW] = {0};   /* 血氧滤波缓存 */
static uint8_t s_spo2_idx = 0;                      /* 血氧缓存索引 */

static float red_dc_global = 0.0f;                  /* 红光全局 DC 值 */
static float ir_dc_global  = 0.0f;                  /* 红外光全局 DC 值 */

/* Global variables ----------------------------------------------------------*/
FilteredVitalSigns g_vital_signs = {0};
static MAX30102_Result_t  result = {0};
static uint8_t new_hr_pos   = 0;
static uint8_t new_hr_neg   = 0;

/* Private functions ---------------------------------------------------------*/

/* 采集数据并处理 AC/DC, 满一批返回 1 */
static uint8_t MAX30102_CollectAndProcessAC(uint8_t *fifo_buffuer)
{
    /* 解析 ADC 值 */
    uint32_t red_adc = ((uint32_t)fifo_buffuer[0] << 16) |
                       ((uint32_t)fifo_buffuer[1] << 8)  |
                       (uint32_t)fifo_buffuer[2];

    uint32_t ir_adc  = ((uint32_t)fifo_buffuer[3] << 16) |
                       ((uint32_t)fifo_buffuer[4] << 8)  |
                       (uint32_t)fifo_buffuer[5];

    red_adc &= 0x3FFFF;    /* 取 18bit 有效数据 */
    ir_adc  &= 0x3FFFF;

    /* 缓存原始数据 */
    if (buf_count < BATCH_BUF_SIZE)
    {
        raw_red_buf[buf_count] = red_adc;
        raw_ir_buf[buf_count] = ir_adc;
        buf_count++;
    }

    /* 缓存满后执行处理 */
    if (buf_count >= BATCH_BUF_SIZE)
    {
        /* 三点移动平均滤波 */
        for (uint16_t i = 1; i < BATCH_BUF_SIZE - 1; i++)
        {
            red_filtered[i - 1] = (raw_red_buf[i - 1] + raw_red_buf[i] + raw_red_buf[i + 1]) / FILTER_WINDOW;
            ir_filtered[i - 1]  = (raw_ir_buf[i - 1] + raw_ir_buf[i] + raw_ir_buf[i + 1]) / FILTER_WINDOW;
        }

        memset(raw_red_buf, 0, sizeof(raw_red_buf));
        memset(raw_ir_buf, 0, sizeof(raw_ir_buf));
        buf_count = 0;

        /* 计算全局 DC */
        uint64_t red_dc_sum = 0, ir_dc_sum = 0;
        for (uint16_t i = 0; i < BATCH_BUF_SIZE - 2; i++)
        {
            red_dc_sum += red_filtered[i];
            ir_dc_sum  += ir_filtered[i];
        }
        red_dc_global = (float)red_dc_sum / (BATCH_BUF_SIZE - 2);
        ir_dc_global  = (float)ir_dc_sum / (BATCH_BUF_SIZE - 2);

        /* AC 分离 */
        for (uint16_t i = 0; i < BATCH_BUF_SIZE - 2; i++)
        {
            red_ac[i] = (float)red_filtered[i] - red_dc_global;
            ir_ac[i]  = (float)ir_filtered[i] - ir_dc_global;
        }
        return 1;
    }
    return 0;
}

/* 提取极值点 (正峰或负谷) */
static void ExtractTwoExtremes(uint8_t is_positive, uint8_t *ext_idx, float *ext_val, uint16_t *out_peaks)
{
    uint8_t count = 0;
    out_peaks[0] = 0;
    out_peaks[1] = 0;

    /* 找出所有候选点 */
    for (uint16_t i = 0; i < BATCH_BUF_SIZE - 2; i++)
    {
        if (is_positive ? (ir_ac[i] > 0) : (ir_ac[i] < 0))
        {
            if (count < BATCH_BUF_SIZE)
            {
                ext_idx[count] = i;
                ext_val[count] = ir_ac[i];
                count++;
            }
        }
    }

    if (count >= 2)
    {
        /* 寻找全局最大/极小值 */
        uint8_t ext1_idx = 0;
        float ext1_val = ext_val[0];
        for (uint8_t k = 1; k < count; k++)
        {
            if (is_positive ? (ext_val[k] > ext1_val) : (ext_val[k] < ext1_val))
            {
                ext1_val = ext_val[k];
                ext1_idx = k;
            }
        }

        /* 寻找第二个极值 (满足最小时间间隔) */
        uint8_t ext2_idx = 255;
        float ext2_val = 0.0f;

        for (uint8_t k = 0; k < count; k++)
        {
            if (k == ext1_idx) continue;

            uint16_t interval = (ext_idx[k] > ext_idx[ext1_idx]) ?
                                (ext_idx[k] - ext_idx[ext1_idx]) :
                                (ext_idx[ext1_idx] - ext_idx[k]);

            if (interval >= MIN_PEAK_INTERVAL)
            {
                if (ext2_idx == 255 ||
                  (is_positive ? (ext_val[k] > ext2_val) : (ext_val[k] < ext2_val)))
                {
                    ext2_val = ext_val[k];
                    ext2_idx = k;
                }
            }
        }

        /* 按时间先后顺序存入 out_peaks */
        if (ext2_idx != 255)
        {
            if (ext_idx[ext1_idx] < ext_idx[ext2_idx])
            {
                out_peaks[0] = ext_idx[ext1_idx];
                out_peaks[1] = ext_idx[ext2_idx];
            }
            else
            {
                out_peaks[0] = ext_idx[ext2_idx];
                out_peaks[1] = ext_idx[ext1_idx];
            }
        }
    }
}

/* Exported functions --------------------------------------------------------*/

/* 波峰检测: 输出 4 个极值索引 (0~1 正峰, 2~3 负谷) */
static void MAX30102_DetectPeaks(uint16_t *peaks)
{
    memset(&reuse_mem.stage_peaks, 0, sizeof(reuse_mem.stage_peaks));

    /* 提取正峰 */
    ExtractTwoExtremes(1, pos_peak_idx, pos_peak_val, &peaks[0]);

    /* 提取负谷 */
    ExtractTwoExtremes(0, neg_valley_idx, neg_valley_val, &peaks[2]);
}

/* 计算心率和血氧 */
static void MAX30102_CalcVitalSigns(uint16_t *peaks)
{
    result.heart_rate = 0;
    result.spo2 = 0;

    uint16_t pos_interval = peaks[1] - peaks[0];
    uint16_t neg_interval = peaks[3] - peaks[2];

    uint8_t hr_pos = 0;
    uint8_t hr_neg = 0;

    if (pos_interval > 0) hr_pos = (uint8_t)(60.0f / ((float)pos_interval / SAMPLE_RATE));
    if (neg_interval > 0) hr_neg = (uint8_t)(60.0f / ((float)neg_interval / SAMPLE_RATE));

    /* 寻找 AC 最大值 */
    float red_ac_max = 0.0f;
    float ir_ac_max  = 0.0f;

    for (uint16_t i = 0; i < BATCH_BUF_SIZE - 2; i++)
    {
        if (red_ac[i] > red_ac_max) red_ac_max = red_ac[i];
        if (ir_ac[i]  > ir_ac_max)  ir_ac_max  = ir_ac[i];
    }

    /* 计算 SpO2 (不参与校准, 由后级滑动平均滤波器降噪) */
    if (ir_ac_max > 0.0f && red_dc_global > 0.0f) {
        float R = (red_ac_max * ir_dc_global) / (ir_ac_max * red_dc_global);
        result.spo2 = (uint8_t)(104.0f - 17.0f * R);
    } else {
        result.spo2 = 0;
    }

    if (result.spo2 <= 90) result.spo2 = 90;
    if (result.spo2 > 100) result.spo2 = 99;

    new_hr_pos = hr_pos;
    new_hr_neg = hr_neg;
    uint8_t new_hr = 0;

    /* 心率校准机制 */
    uint8_t quiet_is_reasonable = (new_hr_pos > 60 && new_hr_pos < 90 &&
                                   new_hr_neg > 60 && new_hr_neg < 90);

    /* 判断心率是否在 40~150 之间 */
    uint8_t pos_valid = (new_hr_pos >= 40 && new_hr_pos <= 150);
    uint8_t neg_valid = (new_hr_neg >= 40 && new_hr_neg <= 150);

    if (smooth_buf.index == 0)
    {
        if (smooth_buf.averaged == 0)
        {
            /* 谁合理就用谁, 都合理则取平均 */
            if (pos_valid && !neg_valid) new_hr = new_hr_pos;
            else if (!pos_valid && neg_valid) new_hr = new_hr_neg;
            else if (pos_valid && neg_valid) new_hr = (new_hr_pos + new_hr_neg) / 2;
        }

        if (quiet_is_reasonable && smooth_buf.averaged == 0)
        {
            /* 首次抓到完美静息数据, 直接输出并打底 */
            new_hr = (new_hr_pos + new_hr_neg) / 2;
            smooth_buf.heart_rate[0] = new_hr;
            smooth_buf.index = 1;
            smooth_buf.averaged = 1;
        }
        else if (smooth_buf.averaged == 1)
        {
            /* 存满后用平均值覆盖第 0 位作为新基准 */
            uint16_t sum_hr = 0;
            for (uint8_t i = 0; i < SMOOTH_BUF_SIZE; i++)
            {
                sum_hr += smooth_buf.heart_rate[i];
            }
            smooth_buf.heart_rate[0] = (uint8_t)(sum_hr / SMOOTH_BUF_SIZE);
            new_hr = smooth_buf.heart_rate[0];

            smooth_buf.index = 1;
        }
    }
    else if (smooth_buf.averaged == 1)
    {
        uint8_t last_hr = smooth_buf.heart_rate[smooth_buf.index - 1];

        uint8_t pos_dev_ok = (abs((int)new_hr_pos - (int)last_hr) <= 10);
        uint8_t neg_dev_ok = (abs((int)new_hr_neg - (int)last_hr) <= 10);

        if (pos_dev_ok || neg_dev_ok)
        {
            if (pos_dev_ok && !neg_dev_ok) new_hr = new_hr_pos;
            else if (!pos_dev_ok && neg_dev_ok) new_hr = new_hr_neg;
            else if (pos_dev_ok && neg_dev_ok) new_hr = (new_hr_pos + new_hr_neg) / 2;

            smooth_buf.heart_rate[smooth_buf.index] = new_hr;
            smooth_buf.index++;
            if (smooth_buf.index >= SMOOTH_BUF_SIZE) smooth_buf.index = 0;
        }
        else
        {
            /* 偏差过大, 拒收脏数据, 维持上次稳态值 */
            new_hr = last_hr;
        }
    }

    result.heart_rate = new_hr;
}

/* 心率滑动平均滤波 */
static uint8_t HR_SlidingAvgFilter(uint8_t new_hr)
{
    static uint8_t valid_count = 0;
    uint32_t sum = 0;

    s_hr_buf[s_hr_idx] = new_hr;
    s_hr_idx = (s_hr_idx + 1) % HR_AVG_WINDOW;

    if (valid_count < HR_AVG_WINDOW) valid_count++;

    for (uint8_t i = 0; i < valid_count; i++)
        sum += s_hr_buf[i];

    g_vital_signs.smooth_hr = (uint8_t)(sum / valid_count);
    return g_vital_signs.smooth_hr;
}

/* 血氧滑动平均滤波 */
static uint8_t SPO2_SlidingAvgFilter(uint8_t new_spo2)
{
    static uint8_t valid_count = 0;
    uint32_t sum = 0;

    s_spo2_buf[s_spo2_idx] = new_spo2;
    s_spo2_idx = (s_spo2_idx + 1) % SPO2_AVG_WINDOW;

    if (valid_count < SPO2_AVG_WINDOW) valid_count++;

    for (uint8_t i = 0; i < valid_count; i++)
        sum += s_spo2_buf[i];

    g_vital_signs.smooth_spo2 = (uint8_t)(sum / valid_count);
    return g_vital_signs.smooth_spo2;
}

/* FIFO 数据处理入口 */
void MAX30102_ProcessData(uint8_t *fifo_buffuer)
{
    uint16_t peaks[4] = {0};

    if (MAX30102_CollectAndProcessAC(fifo_buffuer))
    {
        MAX30102_DetectPeaks(peaks);
        MAX30102_CalcVitalSigns(peaks);
        HR_SlidingAvgFilter(result.heart_rate);
        SPO2_SlidingAvgFilter(result.spo2);
    }
}
