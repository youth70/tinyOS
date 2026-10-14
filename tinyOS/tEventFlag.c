#include "tinyOS.h"

void tEventFlagInit(tEventFlag *eventFlag)
{
    tEventInit(&(eventFlag->ECB), tEventTypeEventFlag);   // 初始化内嵌 ECB
    eventFlag->flags = 0;                                  // 标志位清零
}

uint32_t tEventFlagWait(tEventFlag *eventFlag, uint32_t mask,
                        uint32_t waitType, uint32_t clearType,
                        uint32_t timeout, uint32_t *result)
{
    uint32_t status = tTaskEnterCritical();               // 进入临界区

    uint32_t matchedFlags = eventFlag->flags & mask;
    uint32_t matched = 0;

    if (waitType == TINYOS_EVENTFLAG_WAIT_ALL)            // 等待所有位
        matched = (matchedFlags == mask);
    else                                                   // 等待任意位
        matched = (matchedFlags != 0);

    if (matched)                                           // 条件已满足，立即返回
    {
        *result = matchedFlags;
        if (clearType == TINYOS_EVENTFLAG_CLEAR_SET)       // 需要清除标志
            eventFlag->flags &= ~matchedFlags;
        tTaskExitCritical(status);
        return tErrorNoError;
    }

    currentTask->waitFlagInfo.mask      = mask;            // 关心哪些位
    currentTask->waitFlagInfo.waitType  = waitType;        // AND / OR
    currentTask->waitFlagInfo.clearType = clearType;       // 是否清除
    currentTask->waitFlagInfo.result    = 0;               // 结果先清零

    tEventWait(&(eventFlag->ECB), currentTask, (void *)0,
               tEventTypeEventFlag, timeout);              // 挂起等待
    tTaskExitCritical(status);

    tTaskSched();                                          // 让出 CPU

    // ★ 从 TCB 内嵌结构体读结果
    *result = currentTask->waitFlagInfo.result;            // 取回满足的标志位
    return currentTask->waitEventResult;                   // 返回等待结果
}

uint32_t tEventFlagSet(tEventFlag *eventFlag, uint32_t flags)
{
    uint32_t status = tTaskEnterCritical();                // 进入临界区
    uint32_t wakeCount = 0;                                // 统计唤醒任务数
    tNode *node;
    tNode *nextNode;

    eventFlag->flags |= flags;                             // 设置标志位

    node = eventFlag->ECB.waitList.firstnode;              // 从等待链表第一个节点开始
    while (node != &(eventFlag->ECB.waitList.headNode))    // 遍历整个等待链表
    {
        nextNode = node->nextNode;                         // 先保存下一个节点
        tTask *task = tNodeParent(node, tTask, linkNode);  // 反推 TCB

        tEventFlagWaitInfo *info = &task->waitFlagInfo;

        uint32_t matchedFlags = eventFlag->flags & info->mask;  // 过滤
        uint32_t matched = 0;

        if (info->waitType == TINYOS_EVENTFLAG_WAIT_ALL)        // AND 模式
            matched = (matchedFlags == info->mask);
        else                                                     // OR 模式
            matched = (matchedFlags != 0);

        if (matched)                                             // 条件满足
        {
            info->result = matchedFlags;                         // 记录结果

            if (info->clearType == TINYOS_EVENTFLAG_CLEAR_SET)   // 需要清除？
                eventFlag->flags &= ~matchedFlags;               // 清除被满足的位

            tEventRemoveTask(task, (void *)0, tErrorNoError);    // 从等待链表移除
            tTaskSchedRdy(task);                                 // 加回就绪表
            wakeCount++;                                         // 唤醒计数 +1
        }

        node = nextNode;                                         // 下一个节点
    }

    tTaskExitCritical(status);                                   // 退出临界区

    if (wakeCount > 0)                                           // 有任务被唤醒
        tTaskSched();                                            // 请求调度

    return wakeCount;                                            // 返回唤醒数量
}

uint32_t tEventFlagClear(tEventFlag *eventFlag, uint32_t flags)
{
    uint32_t status = tTaskEnterCritical();       // 进入临界区
    eventFlag->flags &= ~flags;                    // 清除指定标志位
    tTaskExitCritical(status);                    // 退出临界区
    return tErrorNoError;                          // 返回成功
}

uint32_t tEventFlagDestroy(tEventFlag *eventFlag)
{
    uint32_t status = tTaskEnterCritical();                         // 进入临界区
    uint32_t count = tEventRemoveAll(&(eventFlag->ECB), (void *)0, tErrorDel);  // 唤醒所有等待者
    eventFlag->flags = 0;                                           // 标志清零
    tTaskExitCritical(status);                                      // 退出临界区

    if (count > 0)                                                  // 有任务被唤醒？
        tTaskSched();                                               // 请求调度

    return count;                                                   // 返回唤醒任务数
}

void tEventFlagGetInfo(tEventFlag *eventFlag, tEventFlagInfo *info)
{
    uint32_t status = tTaskEnterCritical();              // 进入临界区

    info->flags = eventFlag->flags;                       // 当前标志位
    info->taskCount = tEventWaitCount(&(eventFlag->ECB)); // 等待任务数

    tTaskExitCritical(status);                            // 退出临界区
}
