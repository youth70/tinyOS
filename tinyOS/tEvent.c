#include "tinyOS.h"

void tEventInit(tEventControlBlock *ECB, tEventType eventtype)	//The Event Control Block initial function
{
	ECB->type=eventtype;
	tListInit(&(ECB->waitList));
}

void tEventWait(tEventControlBlock *ECB, tTask *task,
                void *msg, uint32_t state, uint32_t timeout)//等待事件
{
    uint32_t status = tTaskEnterCritical();              // 1. 进入临界区

    task->waitEvent = ECB;                                // 2. 记录在等哪个事件
    task->eventMsg = msg;                                 // 3. 保存消息
    task->state |= state;                                 // 4. 设置等待状态
    task->waitEventResult=tErrorNoError;                 // 5. 结果：无错误

    tTaskSchedUnRdy(task);                                // 6. 从就绪表移除
    tListAddLast(&(ECB->waitList), &(task->linkNode));    // 7. 挂到 ECB 等待链表

    if (timeout != 0)                                     // 8. 有超时？
    {
        tTimeTaskWait(task, timeout);                     // 9. 同时加入延时链表
    }

    tTaskExitCritical(status);                            // 10. 退出临界区
}

tTask * tEventWakeUp(tEventControlBlock *ECB, void *msg, uint32_t result)
{
    tNode *node = (tNode *)0;                              // 1. 临时变量
    tTask *task = (tTask *)0;

    uint32_t status = tTaskEnterCritical();                 // 2. 进入临界区

    if ((node = tListRemoveFirst(&(ECB->waitList))) != (tNode *)0)  // 3. 取第一个任务并删除
    {
        task = tNodeParent(node, tTask, linkNode);          // 4. 反推 TCB

        task->waitEvent = (tEventControlBlock *)0;          // 5. 清等待事件
        task->eventMsg = msg;                               // 6. 设消息
        task->state &= ~TINYOS_TASK_WAIT_MASK;              // 7. 清等待状态
        task->waitEventResult = result;                     // 8. 设结果

        if (task->delayTicks != 0)                          // 9. 有超时？
        {
            tTimeTaskWakeUp(task);                          // 10. 从延时链表移除
        }

        tTaskSchedRdy(task);                                // 11. 加回就绪表
    }

    tTaskExitCritical(status);                              // 12. 退出临界区
    return task;                                            // 13. 返回被唤醒的任务
}

void tEventRemoveTask(tTask *task, void *msg, uint32_t result)
{
    uint32_t status = tTaskEnterCritical();                    // 1. 进入临界区

    tListRemoveNode(&(task->waitEvent->waitList), &(task->linkNode));  // 2. 从等待链表移除

    task->waitEvent = (tEventControlBlock *)0;                 // 3. 清除等待事件
    task->eventMsg = msg;                                      // 4. 设置消息
    task->state &= ~TINYOS_TASK_WAIT_MASK;                     // 5. 清除等待状态
    task->waitEventResult = result;                            // 6. 设置等待结果

    tTaskExitCritical(status);                                 // 7. 退出临界区
}

uint32_t tEventRemoveAll(tEventControlBlock *ECB, 
					   void *msg, uint32_t result)				//唤醒等待某个事件的所有任务
{
	tNode *node=(tNode *)0;
	tTask *task=(tTask *)0;
	uint32_t count;
	
	uint32_t status=tTaskEnterCritical();
	
	count=tListCount(&(ECB->waitList));
	
	while((node=tListRemoveFirst(&(ECB->waitList))) != (tNode*)0)//删除并返回
	{
		task=tNodeParent(node, tTask, linkNode);
		task->waitEvent=(tEventControlBlock *)0;
		task->eventMsg=msg;
		task->state &= ~TINYOS_TASK_WAIT_MASK;
		task->waitEventResult=result;
	
		if(task->delayTicks!=0)
		{
			tTimeTaskWakeUp(task);
		}
		
		tTaskSchedRdy(task);
	}
	
	tTaskExitCritical(status);
	return count;
}

uint32_t tEventWaitCount(tEventControlBlock *ECB)					//The function of returning the number of tasks in the waitList of ECB
{
	uint32_t count;
	
	uint32_t status=tTaskEnterCritical();
	
	count=tListCount(&(ECB->waitList));
	
	tTaskExitCritical(status);
	return count;
}
