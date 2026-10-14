#include "tinyOS.h"
#include <string.h>

void tQueueInit(tQueue *queue, void *buffer, uint32_t maxCount, uint32_t itemSize) // 初始化队列函数
{
    // 参数合法性校验：任一非法直接返回
    if (queue == (tQueue *)0 ||                                               // queue 为空指针
        buffer == (void *)0 ||                                                // buffer 为空指针
        maxCount == 0 ||                                                      // 最大消息数为 0
        itemSize == 0)                                                        // 单条消息大小为 0
    {
        return;
    }

    tEventInit(&(queue->ECB), tEventTypeQueue);                               // 初始化事件控制块，类型为队列
    queue->queueBuffer = (uint8_t *)buffer;                                   // 保存缓冲区首地址
    queue->maxCount = maxCount;                                               // 记录队列最大消息条数
    queue->itemSize = itemSize;                                               // 记录单条消息占用的字节数
    queue->Count = 0;                                                         // 当前消息数清零
    queue->read = 0;                                                          // 读索引清零
    queue->write = 0;                                                         // 写索引清零
}

uint32_t tQueueWait(tQueue *queue, void *msg, uint32_t timeout)               // 阻塞接收队列消息
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    if (queue->Count > 0)                                                     // 队列中有消息？
    {
        uint8_t *src = queue->queueBuffer + queue->read * queue->itemSize;    // 计算读位置地址
        memcpy(msg, src, queue->itemSize);                                    // 拷贝到用户缓冲区

        queue->read++;                                                        // 读索引后移
        if (queue->read >= queue->maxCount)                                   // 越界？
            queue->read = 0;                                                  // 回绕
        queue->Count--;                                                       // 消息数减 1

        tTaskExitCritical(status);
        return tErrorNoError;                                                 // 成功
    }
    else                                                                      // 队列为空，等待
    {
        currentTask->eventMsg = msg;                                          // 保存接收缓冲区地址
        tEventWait(&(queue->ECB), currentTask, (void *)0,
                   tEventTypeQueue, timeout);                                 // 挂到队列等待链表
        tTaskExitCritical(status);

        tTaskSched();                                                         // 让出 CPU

        currentTask->eventMsg = (void *)0;                                    // 清空 eventMsg
        return currentTask->waitEventResult;                                  // 返回等待结果
    }
}

uint32_t tQueueNoWaitGet(tQueue *queue, void *msg)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    if (queue->Count > 0)                                                     // 有消息？
    {
        uint8_t *src = queue->queueBuffer + queue->read * queue->itemSize;    // 计算读位置
        memcpy(msg, src, queue->itemSize);                                    // 拷贝出来

        queue->read++;
        if (queue->read >= queue->maxCount)
            queue->read = 0;
        queue->Count--;

        tTaskExitCritical(status);
        return tErrorNoError;                                                 // 成功
    }

    tTaskExitCritical(status);
    return tErrorResourceUnavailable;                                         // 没消息
}

uint32_t tQueueNotify(tQueue *queue, void *msg)                               // 发送消息
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    if (tEventWaitCount(&(queue->ECB)) > 0)                                   // 有任务在等？
    {
        tTask *task = tEventWakeUp(&(queue->ECB), (void *)0, tErrorNoError);  // 唤醒一个等待者（msg 传 NULL）
        memcpy(task->eventMsg, msg, queue->itemSize);                         // 拷贝到等待者的接收缓冲区

        tTaskExitCritical(status);

        if (task->prio < currentTask->prio)                                   // 被唤醒任务优先级更高？
            tTaskSched();                                                     // 触发调度

        return tErrorNoError;
    }
    else                                                                      // 无等待者
    {
        if (queue->Count >= queue->maxCount)                                  // 队列满？
        {
            tTaskExitCritical(status);
            return tErrorResourceFull;
        }

        uint8_t *dst = queue->queueBuffer + queue->write * queue->itemSize;   // 计算写位置
        memcpy(dst, msg, queue->itemSize);                                    // 拷贝到环形缓冲区

        queue->write++;                                                       // 写索引后移
        if (queue->write >= queue->maxCount)
            queue->write = 0;                                                 // 回绕
        queue->Count++;

        tTaskExitCritical(status);
        return tErrorNoError;
    }
}

void tQueueFlush(tQueue *queue)                                               // 清空队列中的所有消息
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区
	
    queue->Count = 0;                                                         // 消息数清零
    queue->read = 0;                                                          // 读索引归零
    queue->write = 0;                                                         // 写索引归零

    tTaskExitCritical(status);
}

uint32_t tQueueDestroy(tQueue *queue)                                         // 销毁队列
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    uint32_t count = tEventRemoveAll(&(queue->ECB), (void *)0, tErrorDel);    // 唤醒所有等待者

    queue->Count = 0;                                                         // 消息数清零
    queue->read = 0;                                                          // 读索引归零
    queue->write = 0;                                                         // 写索引归零

    tTaskExitCritical(status);

    if (count > 0)                                                            // 有任务被唤醒？
        tTaskSched();                                                         // 触发调度

    return count;
}

void tQueueGetInfo(tQueue *queue, tQueueInfo *info)                           // 查询队列信息
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    info->Count = queue->Count;                                               // 当前消息数
    info->maxCount = queue->maxCount;                                         // 最大容量
    info->taskCount = tEventWaitCount(&(queue->ECB));                         // 等待任务数

    tTaskExitCritical(status);
}
