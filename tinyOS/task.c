#include "tinyOS.h"


void tTaskRunFirst(void)	//This function'll be called when the tinyOS ran at the first time
{
	__set_PSP(0);//把 PSP 设为 0，是一个标记，表示“现在还没有任务在跑”
	MEM8(NVIC_INIT_SYSPRI2)=NVIC_INIT_PENDSV_PRI;	//把 PendSV 设为最低优先级
	MEM32(NVIC_INIT_CTRL)=NVIC_INIT_PENDSVSET;		//触发 PendSV
}

void tTaskSched(void)	//Task schedule function
{
	uint32_t status=tTaskEnterCritical();
	tTask *tempTask;
	
	if(schedLockCount>0)		//Task schedule has been locked
	{
		tTaskExitCritical(status);
		return;
	}
	
	tempTask=tTaskHighestReady();
	if(tempTask!=currentTask)
	{
		nextTask=tempTask;
		tTaskSwitch();
	}

	tTaskExitCritical(status);
}

void tTaskSwitch(void)	//Task switch function
{
	MEM32(NVIC_INIT_CTRL)=NVIC_INIT_PENDSVSET;			//Directly trigger PendSV
}

void tTaskInit(tTask *task, void(*entry)(void *), 
			   void *param, uint32_t prio, tTaskStack *stack)			//Task initial function
{
	//These data are pushed or popped using PSP pointer by the hardware automaticly
	*(--stack)=(unsigned long)(1<<24);		//xPSR
	*(--stack)=(unsigned long)entry;			//PC(R15)
	*(--stack)=(unsigned long)0x50C;			//R14
	*(--stack)=(unsigned long)0x12;			//R12
	*(--stack)=(unsigned long)0x3;				//R3
	*(--stack)=(unsigned long)0x2;				//R2
	*(--stack)=(unsigned long)0x1;				//R1
	*(--stack)=(unsigned long)param;			//param(R0)
	
	//These data are pushed or popped using task structure stack pointer by user manually
	*(--stack)=(unsigned long)0x11;			//R11
	*(--stack)=(unsigned long)0x10;			//R10
	*(--stack)=(unsigned long)0x9;				//R9
	*(--stack)=(unsigned long)0x8;				//R8
	*(--stack)=(unsigned long)0x7;				//R7
	*(--stack)=(unsigned long)0x6;				//R6
	*(--stack)=(unsigned long)0x5;				//R5
	*(--stack)=(unsigned long)0x4;				//R4
	
	task->stack=stack;										//Initiate the task stack pointer
	task->delayTicks=0;										//Initiate the task delay counter
	tNodeInit(&(task->delayNode));				//Initiate the task's delay node
	tNodeInit(&(task->linkNode));				  // 初始化就绪节点
	task->prio=prio;											//Initiate the task priority
	task->state=TINYOS_TASK_STATUS_RDY;		//Initiate the task delay state: ready
	task->slice=TINYOS_SLICE_MAX;
	task->suspendCount=0;
	
	task->clean=(void(*)(void *))0; 		//Initiate the callback function pointer
	task->cleanParam=(void *)0;						//Initiate the callback function parameter
	task->requestDeleteFlag=0;						//Initiate the the flag of requesting to delete task
	task->waitEvent = (struct _tEventControlBlock *)0;
	task->eventMsg = (void *)0;
	task->waitEventResult = tErrorNoError;
	task->waitFlagInfo = (struct _tEventFlagWaitInfo *)0;
	
	tListAddFirst(&taskTable[prio], &(task->linkNode));

	tBitmapSet(&taskPrioBitmap,prio);
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

tTask *tTaskHighestReady(void)
{
    uint32_t highest = tBitmapGetFirstSet(&taskPrioBitmap);
    tNode *node = tListFirst(&taskTable[highest]);
    return tNodeParent(node, tTask, linkNode);
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

void tTaskForceDelete(tTask *task)			//Force to delete the task
{
	uint32_t status=tTaskEnterCritical();
	if(task->state & TINYOS_TASK_STATUS_DELAYED)//判断是否延时
	{
		tTimeTaskRemove(task);
	}
	else if(!(task->state & TINYOS_TASK_STATUS_SUSPENDED))//判断是否挂起
	{
		tTaskSchedRemove(task);
	}
	if(task->clean)
	{
		task->clean(task->cleanParam);
	}
	if(currentTask==task)
	{
		tTaskSched();
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

void tTaskDeleteSelf(void)			//任务自己删除自己。
{
	uint32_t status=tTaskEnterCritical();
	
	tTaskSchedRemove(currentTask);
	if(currentTask->clean)
	{
		currentTask->clean(currentTask->cleanParam);
	}
	tTaskSched();
	
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
