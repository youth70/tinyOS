#include "tinyOS.h"

void tSemInit(tSem *sem, uint32_t startCount, uint32_t maxCount)	//The Counting Semaphores initial function
{
	tEventInit(&(sem->ECB), tEventTypeSem);
	
	sem->maxCount=maxCount;
	if(maxCount==0)
	{
		sem->Count=startCount;
	}
	else
	{
		sem->Count=(startCount>maxCount) ? maxCount:startCount;
	}
}

uint32_t tSemWait(tSem *sem, uint32_t timeout)
{
    uint32_t status = tTaskEnterCritical();              // 1. 进入临界区

    if (sem->Count > 0)                                   // 2. 有资源？
    {
        sem->Count--;                                     // 3. 计数减 1
        tTaskExitCritical(status);
        return tErrorNoError;                             // 4. 返回成功
    }
    else                                                  // 5. 没资源
    {
        tEventWait(&(sem->ECB), currentTask, (void *)0, tEventTypeSem, timeout);  // 6. 等
        tTaskExitCritical(status);

        tTaskSched();                                     // 7. 让出 CPU
        return currentTask->waitEventResult;              // 8. 返回等待结果
    }
}

uint32_t tSemNoWaitGet(tSem *sem)                 //不做等待
{
    uint32_t status = tTaskEnterCritical();       // 1. 进入临界区

    if (sem->Count > 0)                            // 2. 有资源？
    {
        sem->Count--;                              // 3. 拿走一个，计数减 1
        tTaskExitCritical(status);
        return tErrorNoError;                      // 4. 返回成功
    }
    else                                           // 5. 没资源
    {
        tTaskExitCritical(status);
        return tErrorResourceUnavailable;          // 6. 立刻返回错误
    }
}

void tSemNotify(tSem *sem)
{
    tTask *task;
    uint32_t status = tTaskEnterCritical();          // 1. 进入临界区

    if (tEventWaitCount(&(sem->ECB)) > 0)             // 2. 有任务在等？
    {
        task = tEventWakeUp(&(sem->ECB), (void *)0, tErrorNoError);  // 3. 唤醒一个
        if (task->prio < currentTask->prio)           // 4. 唤醒的任务优先级更高？
        {
            tTaskSched();                             // 5. 让出 CPU
        }
    }
    else                                              // 6. 没任务在等
    {
        sem->Count++;                                 // 7. 计数加 1
        if ((sem->maxCount) != 0 && (sem->Count) > (sem->maxCount))
        {
            sem->Count = sem->maxCount;               // 8. 超过上限就截断
        }
    }
    tTaskExitCritical(status);                        // 9. 退出临界区
}

void tSemGetInfo(tSem *sem, tSemInfo *info)
{
    uint32_t status = tTaskEnterCritical();              // 1. 进入临界区

    info->Count = sem->Count;                             // 2. 当前计数
    info->maxCount = sem->maxCount;                       // 3. 最大计数
    info->taskCount = tEventWaitCount(&(sem->ECB));       // 4. 等待任务数

    tTaskExitCritical(status);                            // 5. 退出临界区
}

uint32_t tSemDestroy(tSem *sem)
{
    uint32_t status = tTaskEnterCritical();              // 1. 进入临界区

    uint32_t RemovedCount = tEventRemoveAll(&(sem->ECB), (void *)0, tErrorDel);  // 2. 唤醒所有等待者
    sem->Count = 0;                                       // 3. 计数清零

    tTaskExitCritical(status);                            // 4. 退出临界区

    if (RemovedCount > 0)                                 // 5. 如果唤醒了任务
    {
        tTaskSched();                                     // 6. 请求调度
    }

    return RemovedCount;                                  // 7. 返回唤醒数量
}
