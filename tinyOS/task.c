#include "tinyOS.h"


void tTaskRunFirst(void)	//This function'll be called when the tinyOS ran at the first time
{
	__set_PSP(0);//把 PSP 设为 0，是一个标记，表示“现在还没有任务在跑”
	MEM8(NVIC_INIT_SYSPRI2)=NVIC_INIT_PENDSV_PRI;	//把 PendSV 设为最低优先级
	MEM32(NVIC_INIT_CTRL)=NVIC_INIT_PENDSVSET;		//触发 PendSV
}

void tTaskSched(void)
{
    uint32_t status = tTaskEnterCritical();
    tTask *tempTask;

    if (schedLockCount > 0) { tTaskExitCritical(status); return; }

    // 切换前检查当前任务栈底哨兵
    if (currentTask != (tTask *)0 && tTaskCheckStack(currentTask) == 0)
        tTaskStackOverflowHook(currentTask);

    tempTask = tTaskHighestReady();
    if (tempTask != currentTask)
    {
        nextTask = tempTask;
			
			if (tTaskSwitchHook != (tHookFunc)0)
			tTaskSwitchHook();
			
        tTaskSwitch();
    }

    tTaskExitCritical(status);
}

void tTaskSwitch(void)	//Task switch function
{
	MEM32(NVIC_INIT_CTRL)=NVIC_INIT_PENDSVSET;			//Directly trigger PendSV
}

void tTaskYield(void)                          // 主动让出 CPU
{
    uint32_t status = tTaskEnterCritical();    // 进入临界区

    // 把当前任务移到同优先级链表尾部，让下一个同优先级任务先跑
    if (tListCount(&taskTable[currentTask->prio]) > 1)
    {
        tListRemoveFirst(&taskTable[currentTask->prio]);
        tListAddLast(&taskTable[currentTask->prio], &(currentTask->linkNode));
        currentTask->slice = TINYOS_SLICE_MAX;  // 重置时间片
    }

    tTaskExitCritical(status);

    tTaskSched();                              // 触发调度
}

void tTaskInit(tTask *task, void(*entry)(void *),                 // task：TCB 指针；entry：任务入口函数
			   void *param, uint32_t prio,                          // param：传给入口的参数；prio：任务优先级
			   tTaskStack *stack, uint32_t stackSize)               // stack：栈顶指针；stackSize：栈大小（元素个数）
{
	// 保存栈底地址，并在栈底放哨兵
	//   栈从高地址往低地址增长，栈底 = 初始 stack 指针 - stackSize + 1
	//   哨兵放在栈底最低地址，一旦栈溢出就会覆盖它
	task->stackBase = stack - stackSize;               // 计算栈底（最低地址）
	task->stackBase[0] = TINYOS_STACK_SENTINEL;            // 栈底写入哨兵 0xDEADBEEF

	// 以下 8 个字由硬件自动出栈（异常返回时使用 PSP 弹出）
	*(--stack)=(unsigned long)(1<<24);		//xPSR        // 程序状态寄存器（Thumb 位 = 1）
	*(--stack)=(unsigned long)entry;			//PC(R15)     // 任务入口函数地址
	*(--stack)=(unsigned long)0x50C;			//R14         // 链接寄存器 LR（返回地址占位）
	*(--stack)=(unsigned long)0x12;			//R12         // R12 占位
	*(--stack)=(unsigned long)0x3;				//R3          // R3 占位
	*(--stack)=(unsigned long)0x2;				//R2          // R2 占位
	*(--stack)=(unsigned long)0x1;				//R1          // R1 占位
	*(--stack)=(unsigned long)param;			//param(R0)   // R0 = 传给任务入口的参数
	
	// 以下 8 个字由用户手动出栈（PendSV 中 LDMIA 恢复）
	*(--stack)=(unsigned long)0x11;			//R11         
	*(--stack)=(unsigned long)0x10;			//R10         
	*(--stack)=(unsigned long)0x9;				//R9           
	*(--stack)=(unsigned long)0x8;				//R8          
	*(--stack)=(unsigned long)0x7;				//R7          
	*(--stack)=(unsigned long)0x6;				//R6          
	*(--stack)=(unsigned long)0x5;				//R5          
	*(--stack)=(unsigned long)0x4;				//R4          
	
	task->stack=stack;								 // 保存当前栈顶指针
	
	task->delayTicks=0;									// 延时计数清零
	
	tNodeInit(&(task->delayNode));				 // 初始化延时链表节点
	
	tNodeInit(&(task->linkNode));				  // 初始化就绪节点
	                                                        // 初始化就绪链表节点（自环孤立）
	task->prio=prio;										// 保存任务优先级（数值越小越高）
	task->state=TINYOS_TASK_STATUS_RDY;
	task->slice=TINYOS_SLICE_MAX;                 // 初始化时间片为最大值
	task->suspendCount=0;                         // 挂起计数清零
	
	task->clean=(void(*)(void *))0; 		
	
	task->cleanParam=(void *)0;						
	
	task->requestDeleteFlag=0;							// 请求删除标志清零
	                                        
	task->waitEvent = (struct _tEventControlBlock *)0;   // 当前未等待任何事件
	task->eventMsg = (void *)0;                          // 事件消息指针清零
	task->waitEventResult = tErrorNoError;               // 等待结果初始为无错误

	task->waitFlagInfo.mask      = 0;                    // 关心的标志位清零
	task->waitFlagInfo.waitType  = 0;                    // AND/OR 等待方式清零
	task->waitFlagInfo.clearType = 0;                    // 清除类型清零
	task->waitFlagInfo.result    = 0;                    // 结果清零
	
	tListAddFirst(&taskTable[prio], &(task->linkNode));  // 把任务插入对应优先级就绪链表头部

	tBitmapSet(&taskPrioBitmap,prio);                    // 在优先级位图中置位，标记该优先级有就绪任务
}

uint32_t tTaskEnterCritical(void)		//Enter critical section
{
	uint32_t status=__get_PRIMASK();	//读当前中断屏蔽状态，
	__disable_irq();									//关掉所有可屏蔽中断
	return status;
}

void tTaskExitCritical(uint32_t status)		//Exit critical section
{
	__set_PRIMASK(status);									//开中断
}

void tTaskSchedInit(void)					//Task schedule initial function
{
	int i;
	schedLockCount=0;
	tBitmapInit(&taskPrioBitmap);

	for(i=0; i<TINYOS_PRO_COUNT; i++)
	{
		tListInit(&taskTable[i]);
	}
}

void tTaskSchedDisable(void)		//Task schedule disable function(LOCK)
{
	uint32_t status=tTaskEnterCritical();
	
	if(schedLockCount<255)
		schedLockCount++;
	
	tTaskExitCritical(status);
}

void tTaskSchedEnable(void)		//Task schedule enable function(UNLOCK)
{
	uint32_t status=tTaskEnterCritical();
	
	if(schedLockCount>0)
	{
		if(--schedLockCount==0)
			tTaskSched();
	}
	
	tTaskExitCritical(status);
}

tTask *tTaskHighestReady(void)                              // 获取当前最高优先级就绪任务
{
    uint32_t highest = tBitmapGetFirstSet(&taskPrioBitmap);

    if (highest >= TINYOS_PRO_COUNT)
    {
        return idleTask;                                    // 退化为空闲任务（保证系统始终有任务可跑）
    }

    tNode *node = tListFirst(&taskTable[highest]);          // 取该优先级链表的第一个节点
    return tNodeParent(node, tTask, linkNode);              // 反推 TCB
}

void tTaskSchedRdy(tTask *task)			//Put the task into the taskTable and set the corresponding bit of taskPrioBitmap
{
	tListAddFirst(&taskTable[task->prio], &(task->linkNode));
	tBitmapSet(&taskPrioBitmap, task->prio);
}

void tTaskSchedUnRdy(tTask *task)		//Remove the task from the taskTable and clear the corresponding taskPrioBitmap
{
	tListRemoveNode(&taskTable[task->prio], &(task->linkNode));
	
	if(tListCount(&taskTable[task->prio])==0)
	{
		tBitmapClear(&taskPrioBitmap, task->prio);
	}
}

void tTimeTaskWait(tTask *task, uint32_t ms)				//让任务进入延时状态，并把它挂到延时链表上
{
	task->delayTicks=ms/10;
	tListAddLast(&tTaskDelayedList, &(task->delayNode));
	task->state |= TINYOS_TASK_STATUS_DELAYED;
}

void tTimeTaskWakeUp(tTask *task)										//Task wake up function
{
	tListRemoveNode(&tTaskDelayedList, &(task->delayNode));
	task->state &= ~TINYOS_TASK_STATUS_DELAYED;
}

void tTaskSuspend(tTask *task)					//To suspend the task
{
	uint32_t status=tTaskEnterCritical();
	
	if(!(task->state & TINYOS_TASK_STATUS_DELAYED))			//判断任务是否在延时中
	{
		if(++task->suspendCount<=1)												//挂起计数加 1，判断是否第一次挂起
		{
			task->state |= TINYOS_TASK_STATUS_SUSPENDED;
			tTaskSchedUnRdy(task);
		}
		if(task==currentTask)
		{
			tTaskSched();
		}
	}
	
	tTaskExitCritical(status);
}

void tTaskWakeUp(tTask *task)						//To wake up the task
{
	uint32_t status=tTaskEnterCritical();
	
	if(task->state & TINYOS_TASK_STATUS_SUSPENDED)			//The task is suspended
	{
		if(--task->suspendCount==0)
		{
			task->state &= ~TINYOS_TASK_STATUS_SUSPENDED;
			tTaskSchedRdy(task);
			tTaskSched();
		}
	}
	
	tTaskExitCritical(status);
}

void tTimeTaskRemove(tTask *task)										//Remove the task from the tTaskDelayedList
{
	tListRemoveNode(&tTaskDelayedList, &(task->delayNode));
}

void tTaskSchedRemove(tTask *task)		//Remove the task from the taskTable and clear the corresponding taskPrioBitmap
{
	tListRemoveNode(&taskTable[task->prio], &(task->linkNode));
	
	if(tListCount(&taskTable[task->prio])==0)
	{
		tBitmapClear(&taskPrioBitmap, task->prio);
	}
}

void tTaskSetCleanCallFunc(tTask *task, void (*clean)(void *param), void *param)		//Callback function
{
	task->clean=clean;
	task->cleanParam=param;
}

void tTaskForceDelete(tTask *task)                          // 强制删除任务
{
    uint32_t status = tTaskEnterCritical();                 // 进入临界区

    if (task->waitEvent != (tEventControlBlock *)0)
    {
        tEventRemoveTask(task, (void *)0, tErrorDel);       // 从 ECB.waitList 移除，结果设为被删除
    }

    if (task->state & TINYOS_TASK_STATUS_DELAYED)           // 在延时链表？
    {
        tTimeTaskRemove(task);                              // 从延时链表移除
    }
    else if (!(task->state & TINYOS_TASK_STATUS_SUSPENDED)) // 不在挂起状态
    {
        tTaskSchedRemove(task);                             // 从就绪表移除
    }

    if (task->clean)                                        // 有清理回调？
    {
        task->clean(task->cleanParam);                      // 调用清理函数
    }

    if (currentTask == task)                                // 删除的是自己
    {
        tTaskSched();                                       // 让出 CPU
    }

    tTaskExitCritical(status);
}

void tTaskRequestDelete(tTask *task)		//请求删除一个任务
{
	uint32_t status=tTaskEnterCritical();
	
	task->requestDeleteFlag=1;
	
	tTaskExitCritical(status);
}

uint32_t tTaskIsRequestDeleted(void)			//检查当前任务是否被别的任务“请求删除”
{
	uint32_t deleteFlag;
	
	uint32_t status=tTaskEnterCritical();
	
	deleteFlag=currentTask->requestDeleteFlag;
	
	tTaskExitCritical(status);
	
	return deleteFlag;
}

void tTaskDeleteSelf(void)                                  // 任务删除自己
{
    uint32_t status = tTaskEnterCritical();

    if (currentTask->waitEvent != (tEventControlBlock *)0)
    {
        tEventRemoveTask(currentTask, (void *)0, tErrorDel);
    }

    tTaskSchedRemove(currentTask);                          // 从就绪表移除

    if (currentTask->clean)
    {
        currentTask->clean(currentTask->cleanParam);
    }

    tTaskSched();                                           // 让出 CPU（不再返回）

    tTaskExitCritical(status);
}

void tTaskGetInfo(tTask *task, tTaskInfo *taskinfo)		//Get a task's information
{
	taskinfo->delayTicks=task->delayTicks;					//Task delay counter
	taskinfo->prio=task->prio;									//Task priority
	taskinfo->state=task->state;									//Present the delay state of the task
	taskinfo->slice=task->slice;									//Time slice counter
	taskinfo->suspendCount=task->suspendCount;							//Task suspended conunter
}

uint32_t tTaskCheckStack(tTask *task)              // 返回 1=正常，0=溢出
{
    return (task->stackBase[0] == TINYOS_STACK_SENTINEL) ? 1 : 0;
}

__attribute__((weak)) void tTaskStackOverflowHook(tTask *task)
{
    (void)task;
    while (1) { }                                 
}

