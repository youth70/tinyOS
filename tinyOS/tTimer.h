#ifndef TTIMER_H
#define TTIMER_H

#include "tConfig.h"
#include "tEvent.h"

// 定时器状态
#define TINYOS_TIMER_STATE_STOPPED      0    // 已停止
#define TINYOS_TIMER_STATE_RUNNING      1    // 运行中

// 定时器模式
#define TINYOS_TIMER_MODE_ONESHOT       0    // 单次
#define TINYOS_TIMER_MODE_PERIODIC      1    // 周期

typedef struct _tTimer
{
    tNode linkNode;                      // 挂到定时器链表的节点
    uint32_t startDelayTicks;            // 初始延时 tick 数
    uint32_t durationTicks;              // 周期 tick 数（周期模式用）
    uint32_t delayTicks;                 // 当前剩余 tick 数
    void (*timerFunc)(void *arg);        // 回调函数
    void *arg;                           // 回调参数
    uint32_t state;                      // 状态：停止 / 运行
    uint32_t mode;                       // 模式：单次 / 周期
} tTimer;

typedef struct _tTimerInfo
{
    uint32_t startDelayTicks;
    uint32_t durationTicks;
    uint32_t delayTicks;
    void (*timerFunc)(void *arg);
    void *arg;
    uint32_t state;
    uint32_t mode;
} tTimerInfo;

void tTimerInit(tTimer *timer, uint32_t startDelayTicks, uint32_t durationTicks,
                void (*timerFunc)(void *arg), void *arg, uint32_t mode);
void tTimerStart(tTimer *timer);
void tTimerStop(tTimer *timer);
void tTimerDestroy(tTimer *timer);
void tTimerGetInfo(tTimer *timer, tTimerInfo *info);
void tTimerModuleTickNotify(void);
void tTimerModuleInit(void);

#endif
