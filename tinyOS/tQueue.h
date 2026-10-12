#ifndef TQUEUE_H
#define TQUEUE_H

#include "tEvent.h"

typedef struct _tQueue
{
    tEventControlBlock ECB;      // 内嵌事件控制块
    uint32_t Count;              // 当前消息数
    uint32_t maxCount;           // 最大消息数
    uint32_t read;               // 读索引
    uint32_t write;              // 写索引
    uint32_t itemSize;           // 每个消息占多少字节
    uint8_t *queueBuffer;        // 环形缓冲区
} tQueue;

typedef struct _tQueueInfo
{
    uint32_t Count;
    uint32_t maxCount;
    uint32_t taskCount;
} tQueueInfo;

void tQueueInit(tQueue *queue, void *buffer, uint32_t maxCount, uint32_t itemSize);
uint32_t tQueueWait(tQueue *queue, void *msg, uint32_t timeout);
uint32_t tQueueNoWaitGet(tQueue *queue, void *msg);
uint32_t tQueueNotify(tQueue *queue, void *msg);
void tQueueFlush(tQueue *queue);
uint32_t tQueueDestroy(tQueue *queue);
void tQueueGetInfo(tQueue *queue, tQueueInfo *info);

#endif
