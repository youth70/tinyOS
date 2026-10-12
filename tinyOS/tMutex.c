#include "tinyOS.h"

void tMutexInit(tMutex *mutex)
{
    tEventInit(&(mutex->ECB), tEventTypeMutex);   // 1. 初始化内嵌 ECB
    mutex->owner = (tTask *)0;                     // 2. 没有持有者
    mutex->lockCount = 0;                          // 3. 嵌套计数清零
    mutex->originalPrio = 0;                       // 4. 原始优先级清零
}

uint32_t tMutexWait(tMutex *mutex, uint32_t timeout)//等待资源
{
    uint32_t status = tTaskEnterCritical();

    // 情况1：没人持有，直接拿到锁
    if (mutex->owner == (tTask *)0)
    {
        mutex->owner = currentTask;
        mutex->lockCount = 1;
        mutex->originalPrio = currentTask->prio;
        tTaskExitCritical(status);
        return tErrorNoError;
    }

    // 情况2：自己已经持有，嵌套加锁
    if (mutex->owner == currentTask)
    {
        mutex->lockCount++;
        tTaskExitCritical(status);
        return tErrorNoError;
    }
    // 优先级继承：如果当前任务优先级比持有者高，临时提升持有者优先级
    if (currentTask->prio < mutex->owner->prio)//数值越小越高
    {
        tTaskSchedUnRdy(mutex->owner);

        // 如果持有者已经继承了别的优先级，保留最小的那个
        if (mutex->owner->prio > currentTask->prio)
            mutex->owner->prio = currentTask->prio;

        tTaskSchedRdy(mutex->owner);
    }

    // 挂到等待链表
    tEventWait(&(mutex->ECB), currentTask, (void *)0, tEventTypeMutex, timeout);
    tTaskExitCritical(status);

    tTaskSched();
    return currentTask->waitEventResult;
}

uint32_t tMutexNoWaitGet(tMutex *mutex)
{
    uint32_t status = tTaskEnterCritical();

    if (mutex->owner == (tTask *)0)
    {
        mutex->owner = currentTask;
        mutex->lockCount = 1;
        mutex->originalPrio = currentTask->prio;
        tTaskExitCritical(status);
        return tErrorNoError;
    }

    if (mutex->owner == currentTask)
    {
        mutex->lockCount++;
        tTaskExitCritical(status);
        return tErrorNoError;
    }

    tTaskExitCritical(status);
    return tErrorResourceUnavailable;
}

uint32_t tMutexNotify(tMutex *mutex)
{
    uint32_t status = tTaskEnterCritical();

    // 不是持有者，不能解锁
    if (mutex->owner != currentTask)
    {
        tTaskExitCritical(status);
        return tErrorResourceUnavailable;
    }

    // 嵌套锁：只减计数
    if (--mutex->lockCount > 0)
    {
        tTaskExitCritical(status);
        return tErrorNoError;
    }

    // 恢复持有者原优先级
    if (mutex->owner->prio != mutex->originalPrio)
    {
        tTaskSchedUnRdy(mutex->owner);
        mutex->owner->prio = mutex->originalPrio;
        tTaskSchedRdy(mutex->owner);
    }

    // 有等待者：把锁交给它
    if (tEventWaitCount(&(mutex->ECB)) > 0)
    {
        tTask *task = tEventWakeUp(&(mutex->ECB), (void *)0, tErrorNoError);

        mutex->owner = task;
        mutex->lockCount = 1;
        mutex->originalPrio = task->prio;

        if (task->prio < currentTask->prio)
        {
            tTaskSched();
        }
    }
    else
    {
        // 没等待者，释放锁
        mutex->owner = (tTask *)0;
        mutex->lockCount = 0;
    }

    tTaskExitCritical(status);
    return tErrorNoError;
}

uint32_t tMutexDestroy(tMutex *mutex)                                    // 销毁互斥锁，返回被唤醒的任务数
{
    uint32_t status = tTaskEnterCritical();                              // 进入临界区，关中断

    if (mutex->owner != (tTask *)0 &&                                   // 如果锁当前被持有
        mutex->owner->prio != mutex->originalPrio)                      // 且持有者优先级被继承过
    {
        tTaskSchedUnRdy(mutex->owner);                                  // 从当前优先级链表移除持有者
        mutex->owner->prio = mutex->originalPrio;                       // 恢复持有者的原始优先级
        tTaskSchedRdy(mutex->owner);                                    // 按原优先级重新加入就绪表
    }

    uint32_t count = tEventRemoveAll(&(mutex->ECB),                     // 唤醒所有等待该锁的任务
                                     (void *)0,                          // 消息参数为 NULL
                                     tErrorDel);                         // 结果设为“被删除”

    mutex->owner = (tTask *)0;                                          // 清空持有者
    mutex->lockCount = 0;                                               // 清空嵌套计数

    tTaskExitCritical(status);                                          // 退出临界区，恢复中断

    if (count > 0)                                                      // 如果有任务被唤醒
    {
        tTaskSched();                                                   // 请求调度，让高优先级任务抢占
    }

    return count;                                                       // 返回被唤醒的任务数
}

void tMutexGetInfo(tMutex *mutex, tMutexInfo *info)
{
    uint32_t status = tTaskEnterCritical();

    info->owner = mutex->owner;
    info->lockCount = mutex->lockCount;
    info->taskCount = tEventWaitCount(&(mutex->ECB));

    tTaskExitCritical(status);
}
