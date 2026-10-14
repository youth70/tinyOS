#ifndef TEVENTFLAG_H
#define TEVENTFLAG_H

#include "tEvent.h"

// 等待类型
#define TINYOS_EVENTFLAG_WAIT_ALL      0    // 等待所有位（AND）
#define TINYOS_EVENTFLAG_WAIT_ANY      1    // 等待任意位（OR）

// 清除类型
#define TINYOS_EVENTFLAG_CLEAR_NONE    0    // 不清除
#define TINYOS_EVENTFLAG_CLEAR_SET     1    // 清除被满足的标志位

typedef struct _tEventFlag
{
    tEventControlBlock ECB;          // 内嵌事件控制块
    uint32_t flags;                  // 32 位事件标志
} tEventFlag;


typedef struct _tEventFlagInfo
{
    uint32_t flags;                  // 当前标志
    uint32_t taskCount;              // 等待任务数
} tEventFlagInfo;

void tEventFlagInit(tEventFlag *eventFlag);
uint32_t tEventFlagWait(tEventFlag *eventFlag, uint32_t mask,
                        uint32_t waitType, uint32_t clearType,
                        uint32_t timeout, uint32_t *result);
uint32_t tEventFlagSet(tEventFlag *eventFlag, uint32_t flags);
uint32_t tEventFlagClear(tEventFlag *eventFlag, uint32_t flags);
uint32_t tEventFlagDestroy(tEventFlag *eventFlag);
void tEventFlagGetInfo(tEventFlag *eventFlag, tEventFlagInfo *info);

#endif
