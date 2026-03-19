/**
 * @file  bsp_ir_red_cal2.h
 * @brief MAX30102 心率血氧数据处理接口
 */
#ifndef BSP_IR_RED_CAL2_H
#define BSP_IR_RED_CAL2_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
#define SMOOTH_BUF_SIZE     3       /* 心率校准平滑缓冲区大小 */
#define SAMPLE_RATE         100     /* 采样率 (Hz) */
#define FILTER_WINDOW       3       /* 移动平均滤波窗口 */
#define BATCH_BUF_SIZE      200     /* 原始数据批量缓存大小 */
#define MIN_PEAK_INTERVAL   30      /* 最小波峰间隔 (采样点) */
#define MAX_PEAK_INTERVAL   150     /* 最大波峰间隔 (采样点) */
#define HR_AVG_WINDOW       3       /* 心率滑动平均窗口 */
#define SPO2_AVG_WINDOW     5       /* 血氧滑动平均窗口 */

/* Types ---------------------------------------------------------------------*/
/* 单次计算结果 */
typedef struct {
    uint8_t  heart_rate;    /* 心率 (bpm) */
    uint8_t  spo2;          /* 血氧饱和度 (%) */
} MAX30102_Result_t;

/* 平滑后生命体征 */
typedef struct {
    uint8_t smooth_hr;      /* 平滑心率 */
    uint8_t smooth_spo2;    /* 平滑血氧 */
} FilteredVitalSigns;

/* Exported variables --------------------------------------------------------*/
extern FilteredVitalSigns g_vital_signs;
extern MAX30102_Result_t  result;
extern uint8_t new_hr_pos;
extern uint8_t new_hr_neg;

/* Exported functions --------------------------------------------------------*/
void MAX30102_ProcessData(uint8_t *fifo_buffuer);
void MAX30102_DetectPeaks(uint16_t *peaks);
void MAX30102_CalcVitalSigns(uint16_t *peaks);

#ifdef __cplusplus
}
#endif

#endif /* BSP_IR_RED_CAL2_H */
