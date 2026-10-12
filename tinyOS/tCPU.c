#include "tinyOS.h"

extern volatile uint32_t g_idleCount;
extern volatile uint32_t g_cpuUsage;

void tSetSysTickPeriod(uint32_t ms)								//SysTick Timer initial function
{
	SysTick->LOAD=ms*SystemCoreClock/1000-1;					//Set the reload register
	NVIC_SetPriority(SysTick_IRQn,1<<(__NVIC_PRIO_BITS-1));		//The priority of "-" is higher than that of "<<"
	SysTick->VAL=0;
	SysTick->CTRL=SysTick_CTRL_CLKSOURCE_Msk |
				  SysTick_CTRL_TICKINT_Msk |
				  SysTick_CTRL_ENABLE_Msk;						//Enable SysTick IRQ and SysTick Time
}

void SysTick_Handler(void)
{
    tNode *node;
    tTask *task;
    uint32_t status = tTaskEnterCritical();          // 1. 进入临界区
	
	
	
	  static uint32_t tickCount = 0;
    static uint32_t lastIdle = 0;

    tickCount++;
    if (tickCount >= 100)               // 1 秒
    {
        uint32_t idle = g_idleCount - lastIdle;   // 1 秒内增量
        lastIdle = g_idleCount;
        tickCount = 0;

        g_cpuUsage = 100 - (idle * 100 / MAX_IDLE);
    }
		
		

    for (node = tTaskDelayedList.firstnode;          // 2. 遍历延时链表
         node != &(tTaskDelayedList.headNode);
         node = node->nextNode)
    {
        task = tNodeParent(node, tTask, delayNode);  // 3. 反推 TCB
        if (--(task->delayTicks) == 0)               // 4. 递减延时计数
        {
            if (task->waitEvent)                     // 5. 在等事件？
            {
                tEventRemoveTask(task, (void *)0, tErrorTimeout);  // 超时移除
            }

            tTimeTaskWakeUp(task);                   // 6. 从延时链表移除
            tTaskSchedRdy(task);                     // 7. 加回就绪表
        }
    }

    if (--(currentTask->slice) == 0)                 // 8. 时间片递减
    {
        if (tListCount(&taskTable[currentTask->prio]) > 0)
        {
            tListRemoveFirst(&taskTable[currentTask->prio]);       // 9. 移到尾部
            tListAddLast(&taskTable[currentTask->prio], &(currentTask->linkNode));
            currentTask->slice = TINYOS_SLICE_MAX;
        }
    }

    tTaskExitCritical(status);                       // 10. 退出临界区
		tTimerModuleTickNotify();
    tTaskSched();                                    // 11. 请求调度
}
