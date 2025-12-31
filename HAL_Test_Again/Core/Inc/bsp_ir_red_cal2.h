/*
 * bsp_ir_red_cal2.h
 * MAX30102 数据处理重构版头文件
 */
#ifndef BSP_IR_RED_CAL2_H
#define BSP_IR_RED_CAL2_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Exported constants --------------------------------------------------------*/
#define SMOOTH_BUF_SIZE 5      /* 最近5次有效心率血氧缓冲区（环形） */

/* 核心配置参数（200个数据方案） */
#define SAMPLE_RATE 100        /* 采样率 (Hz) */
#define FILTER_WINDOW 3        /* 3点移动平均滤波窗口大小 */
#define BATCH_BUF_SIZE 200     /* 原始数据批量缓存大小 */
#define MIN_PEAK_INTERVAL 40   /* 最小波峰间隔 */
#define MAX_PEAK_INTERVAL 150  /* 最大波峰间隔 */
#define HR_AVG_WINDOW    2     /* 心率滑动平均窗口大小 */
#define SPO2_AVG_WINDOW  2     /* 血氧滑动平均窗口大小 */

/* Exported types ------------------------------------------------------------*/
/* 处理结果结构体 */
typedef struct {
    uint8_t heart_rate;   /* 心率 (次/分) */
    uint8_t spo2;         /* 血氧饱和度 (%) */
    uint16_t jiange;      /* 间隔 */
} MAX30102_Result_t;

/* 滤波后结果结构体 */
typedef struct {
    uint8_t smooth_hr;      /* 平滑心率 */
    uint8_t smooth_spo2;    /* 平滑血氧 */
} FilteredVitalSigns;

static struct {
    uint8_t index;         /* 当前写入位置 */
    uint8_t averaged;      /* 是否已进行过第一次5组平均（1=未，0=已） */
    uint8_t heart_rate[SMOOTH_BUF_SIZE];
    uint8_t spo2[SMOOTH_BUF_SIZE];
} smooth_buf = {0};

/* 全局变量声明 */
extern FilteredVitalSigns g_vital_signs;

/* Exported functions prototypes ---------------------------------------------*/
void MAX30102_ProcessData(uint8_t *fifo_buffuer);
void MAX30102_DetectPeaks(uint16_t *peaks);
void MAX30102_CalcVitalSigns(uint16_t *peaks);

#ifdef __cplusplus
}
#endif

#endif /* BSP_IR_RED_CAL2_H */
