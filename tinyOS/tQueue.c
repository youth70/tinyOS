#include "tinyOS.h"
#include <string.h>

// 初始化队列
void tQueueInit(tQueue *queue, void *buffer, uint32_t maxCount, uint32_t itemSize)
{
    tEventInit(&(queue->ECB), tEventTypeQueue);

    queue->queueBuffer = (uint8_t *)buffer;
    queue->maxCount = maxCount;
    queue->itemSize = itemSize;
    queue->Count = 0;
    queue->read = 0;
    queue->write = 0;
}

// 阻塞接收
uint32_t tQueueWait(tQueue *queue, void *msg, uint32_t timeout)
{
    uint32_t status = tTaskEnterCritical();

    if (queue->Count > 0)
    {
        // 有消息，从缓冲区拷贝出来
        uint8_t *src = queue->queueBuffer + queue->read * queue->itemSize;
        memcpy(msg, src, queue->itemSize);

        queue->read++;
        if (queue->read >= queue->maxCount)
            queue->read = 0;

        queue->Count--;

        tTaskExitCritical(status);
        return tErrorNoError;
    }
    else
    {
        // 没消息，等
        // 把接收缓冲区地址存到 TCB，发送者会直接拷贝到这里
        currentTask->eventMsg = msg;

        tEventWait(&(queue->ECB), currentTask, (void *)0,
                   tEventTypeQueue, timeout);
        tTaskExitCritical(status);

        tTaskSched();

        currentTask->eventMsg = (void *)0;
        return currentTask->waitEventResult;
    }
}

// 非阻塞接收
uint32_t tQueueNoWaitGet(tQueue *queue, void *msg)
{
    uint32_t status = tTaskEnterCritical();

    if (queue->Count > 0)
    {
        uint8_t *src = queue->queueBuffer + queue->read * queue->itemSize;
        memcpy(msg, src, queue->itemSize);

        queue->read++;
        if (queue->read >= queue->maxCount)
            queue->read = 0;

        queue->Count--;

        tTaskExitCritical(status);
        return tErrorNoError;
    }

    tTaskExitCritical(status);
    return tErrorResourceUnavailable;
}

// 发送消息
uint32_t tQueueNotify(tQueue *queue, void *msg)
{
    uint32_t status = tTaskEnterCritical();

    if (tEventWaitCount(&(queue->ECB)) > 0)
    {
        // 有任务在等，直接拷贝到它的缓冲区
        tTask *task = tEventWakeUp(&(queue->ECB), (void *)0, tErrorNoError);

        // task->eventMsg 是接收者提供的缓冲区地址
        memcpy(task->eventMsg, msg, queue->itemSize);

        // 唤醒后如果优先级更高，触发调度
        if (task->prio < currentTask->prio)
        {
            tTaskSched();
        }

        tTaskExitCritical(status);
        return tErrorNoError;
    }
    else
    {
        // 没有等待者，检查缓冲区是否满
        if (queue->Count >= queue->maxCount)
        {
            tTaskExitCritical(status);
            return tErrorResourceFull;
        }

        // 拷贝到环形缓冲区
        uint8_t *dst = queue->queueBuffer + queue->write * queue->itemSize;
        memcpy(dst, msg, queue->itemSize);

        queue->write++;
        if (queue->write >= queue->maxCount)
            queue->write = 0;

        queue->Count++;

        tTaskExitCritical(status);
        return tErrorNoError;
    }
}

// 清空队列
void tQueueFlush(tQueue *queue)
{
    uint32_t status = tTaskEnterCritical();

    queue->Count = 0;
    queue->read = 0;
    queue->write = 0;

    tTaskExitCritical(status);
}

// 销毁队列
uint32_t tQueueDestroy(tQueue *queue)
{
    uint32_t status = tTaskEnterCritical();

    uint32_t count = tEventRemoveAll(&(queue->ECB), (void *)0, tErrorDel);

    queue->Count = 0;
    queue->read = 0;
    queue->write = 0;

    tTaskExitCritical(status);

    if (count > 0)
    {
        tTaskSched();
    }

    return count;
}

// 查询信息
void tQueueGetInfo(tQueue *queue, tQueueInfo *info)
{
    uint32_t status = tTaskEnterCritical();

    info->Count = queue->Count;
    info->maxCount = queue->maxCount;
    info->taskCount = tEventWaitCount(&(queue->ECB));

    tTaskExitCritical(status);
}
