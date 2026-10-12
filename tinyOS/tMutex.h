#ifndef TMUTEX_H
#define TMUTEX_H

#include "tEvent.h"

typedef struct _tMutex
{
    tEventControlBlock ECB;      // 内嵌事件控制块
    tTask *owner;                 // 当前持有锁的任务
    uint32_t lockCount;           // 嵌套加锁次数
    uint32_t originalPrio;        // 持有者的原始优先级
} tMutex;

typedef struct _tMutexInfo
{
    tTask *owner;                 // 当前持有者
    uint32_t lockCount;           // 嵌套次数
    uint32_t taskCount;           // 等待任务数
} tMutexInfo;

void tMutexInit(tMutex *mutex);
uint32_t tMutexWait(tMutex *mutex, uint32_t timeout);
uint32_t tMutexNoWaitGet(tMutex *mutex);
uint32_t tMutexNotify(tMutex *mutex);
uint32_t tMutexDestroy(tMutex *mutex);
void tMutexGetInfo(tMutex *mutex, tMutexInfo *info);

#endif
