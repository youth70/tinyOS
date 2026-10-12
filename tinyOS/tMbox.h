#ifndef TMBOX_H
#define TMBOX_H

#include "tEvent.h"

#define TINYOS_MBOX_NOTIFY_NORMAL      0    // 普通模式
#define TINYOS_MBOX_NOTIFY_OVERWRITE   1    // 覆盖模式

typedef struct _tMbox
{
    tEventControlBlock ECB;         // 内嵌事件控制块
    uint32_t Count;                 // 当前消息数
    uint32_t maxCount;              // 最大消息数
    uint32_t read;                  // 读索引
    uint32_t write;                 // 写索引
    void **msgBuffer;               // 消息缓冲区
} tMbox;

typedef struct _tMboxInfo
{
    uint32_t Count;                 // 当前消息数
    uint32_t maxCount;              // 最大消息数
    uint32_t taskCount;             // 等待任务数
} tMboxInfo;

void tMboxInit(tMbox *mbox, void **msgBuffer, uint32_t maxCount);
uint32_t tMboxWait(tMbox *mbox, void **msg, uint32_t timeout);
uint32_t tMboxNoWaitGet(tMbox *mbox, void **msg);
uint32_t tMboxNotify(tMbox *mbox, void *msg, uint32_t notifyOption);
void tMboxFlush(tMbox *mbox);
uint32_t tMboxDestroy(tMbox *mbox);
void tMboxGetInfo(tMbox *mbox, tMboxInfo *info);

#endif
