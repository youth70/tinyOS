#include "tinyOS.h"

void tEventFlagInit(tEventFlag *eventFlag)
{
    tEventInit(&(eventFlag->ECB), tEventTypeEventFlag);   // 1. 初始化内嵌 ECB
    eventFlag->flags = 0;                                  // 2. 标志位清零
}

uint32_t tEventFlagWait(tEventFlag *eventFlag, uint32_t mask,
                        uint32_t waitType, uint32_t clearType,
                        uint32_t timeout, uint32_t *result)
{
    uint32_t status = tTaskEnterCritical();//进入临界区

    uint32_t matchedFlags = eventFlag->flags & mask;
    uint32_t matched = 0;

    if (waitType == TINYOS_EVENTFLAG_WAIT_ALL)//等待所有位
        matched = (matchedFlags == mask);
    else																		 //等待任意位
        matched = (matchedFlags != 0);

    if (matched)
    {
        *result = matchedFlags;
        if (clearType == TINYOS_EVENTFLAG_CLEAR_SET)//清除标志位
            eventFlag->flags &= ~matchedFlags;
        tTaskExitCritical(status);
        return tErrorNoError;
    }

    // 条件不满足，等待
    tEventFlagWaitInfo info;
    info.mask = mask;
    info.waitType = waitType;
    info.clearType = clearType;
    info.result = 0;

    currentTask->waitFlagInfo = &info;

    tEventWait(&(eventFlag->ECB), currentTask, (void *)0, tEventTypeEventFlag, timeout);
    tTaskExitCritical(status);

    tTaskSched();

    *result = info.result;
    currentTask->waitFlagInfo = (struct _tEventFlagWaitInfo *)0;
    return currentTask->waitEventResult;
}

uint32_t tEventFlagSet(tEventFlag *eventFlag, uint32_t flags)
{
    uint32_t status = tTaskEnterCritical();          // 1. 进入临界区
    uint32_t wakeCount = 0;                          // 2. 统计唤醒任务数
    tNode *node;
    tNode *nextNode;

    eventFlag->flags |= flags;                       // 3. 设置标志位

    node = eventFlag->ECB.waitList.firstnode;        // 4. 从等待链表第一个节点开始
    while (node != &(eventFlag->ECB.waitList.headNode))  // 5. 遍历整个等待链表
    {
        nextNode = node->nextNode;                   // 6. 先保存下一个节点
        tTask *task = tNodeParent(node, tTask, linkNode);  // 7. 反推 TCB

        if (task->waitFlagInfo != (struct _tEventFlagWaitInfo *)0)  // 8. 有等待信息？
        {
            tEventFlagWaitInfo *info = task->waitFlagInfo;          // 9. 取出等待条件
            uint32_t matchedFlags = eventFlag->flags & info->mask;  // 10. 过滤
            uint32_t matched = 0;

            if (info->waitType == TINYOS_EVENTFLAG_WAIT_ALL)        // 11. AND 模式
                matched = (matchedFlags == info->mask);
            else                                                     // 12. OR 模式
                matched = (matchedFlags != 0);

            if (matched)                                             // 13. 条件满足
            {
                info->result = matchedFlags;                         // 14. 记录结果

                if (info->clearType == TINYOS_EVENTFLAG_CLEAR_SET)   // 15. 需要清除？
                    eventFlag->flags &= ~matchedFlags;               // 16. 清除被满足的位

                tEventRemoveTask(task, (void *)0, tErrorNoError);    // 17. 从等待链表移除
                tTaskSchedRdy(task);                                 // 18. 加回就绪表
                wakeCount++;                                         // 19. 唤醒计数 +1
            }
        }

        node = nextNode;                                             // 20. 下一个节点
    }

    tTaskExitCritical(status);                                       // 21. 退出临界区

    if (wakeCount > 0)                                               // 22. 有任务被唤醒
        tTaskSched();                                                // 23. 请求调度

    return wakeCount;                                                // 24. 返回唤醒数量
}

uint32_t tEventFlagClear(tEventFlag *eventFlag, uint32_t flags)
{
    uint32_t status = tTaskEnterCritical();       // 1. 进入临界区
    eventFlag->flags &= ~flags;                    // 2. 清除指定标志位
    tTaskExitCritical(status);                    // 3. 退出临界区
    return tErrorNoError;                          // 4. 返回成功
}

uint32_t tEventFlagDestroy(tEventFlag *eventFlag)
{
    uint32_t status = tTaskEnterCritical();                         // 1. 进入临界区
    uint32_t count = tEventRemoveAll(&(eventFlag->ECB), (void *)0, tErrorDel);  // 2. 唤醒所有等待者
    eventFlag->flags = 0;                                           // 3. 标志清零
    tTaskExitCritical(status);                                      // 4. 退出临界区

    if (count > 0)                                                  // 5. 有任务被唤醒？
        tTaskSched();                                               // 6. 请求调度

    return count;                                                   // 7. 返回唤醒任务数
}

void tEventFlagGetInfo(tEventFlag *eventFlag, tEventFlagInfo *info)
{
    uint32_t status = tTaskEnterCritical();              // 1. 进入临界区

    info->flags = eventFlag->flags;                       // 2. 当前标志位
    info->taskCount = tEventWaitCount(&(eventFlag->ECB)); // 3. 等待任务数

    tTaskExitCritical(status);                            // 4. 退出临界区
}
