#ifndef TINYOS_H
#define TINYOS_H

#include <stdint.h>
#include "tLib.h"
#include "stm32f1xx.h"
#include <stddef.h>
#include "tConfig.h"	
#include "tEvent.h"	
#include "task.h"
#include "tSem.h"
#include "tMbox.h"
#include "tMutex.h"
#include "tEventFlag.h"
#include "tTimer.h"
#include "tMemBlock.h"
#include "tQueue.h"  
#include "tHook.h"


typedef  enum _tTaskError						//Errors of tinyOS
{
	tErrorNoError=0,
	tErrorTimeout,
	tErrorResourceUnavailable,
	tErrorResourceFull,
	tErrorDel,
}tTaskError;


#define NVIC_INIT_CTRL				0xE000ED04// ICSR 寄存器
#define NVIC_INIT_PENDSVSET		0x10000000//bit28 = PENDSVSET，写 1 触发 PendSV

#define NVIC_INIT_SYSPRI2			0xE000ED22// PendSV 优先级寄存器
#define NVIC_INIT_PENDSV_PRI	0x000000FF//要写入的优先级值

#define MEM8(addr)            *(volatile unsigned char *)addr
#define MEM32(addr)           *(volatile unsigned long *)addr
	

extern tTask *currentTask;//一个指向当前正在运行任务的 TCB 的指针
extern tTask *nextTask;//一个指向下一个要运行任务的 TCB 的指针
extern tTask *idleTask;//空闲任务的指针
extern tList taskTable[TINYOS_PRO_COUNT];//一个包含 2 个 tTask * 的数组，也就是“任务表”
extern uint8_t schedLockCount;
extern uint32_t shareCount;
extern tBitmap taskPrioBitmap;
extern tList tTaskDelayedList;


void tTaskRunFirst(void);//第一次启动任务
void tTaskSched(void);	//选择下一个要运行的任务，然后请求切换	
void tTaskYield(void);
void tTaskSwitch(void);	//软件触发 PendSV，请求任务切换
void tTaskInit(tTask *task, void(*entry)(void *),void *param, uint32_t prio, tTaskStack *stack, uint32_t stackSize);
void tSetSysTickPeriod(uint32_t ms);
void SysTick_Handler (void);
void tTaskDelay(uint32_t ms);
void triggerpendSVC(void);
uint32_t tTaskEnterCritical(void);
void tTaskExitCritical(uint32_t status);
void tTaskSchedInit(void);
void tTaskSchedDisable(void);
void tTaskSchedEnable(void);
void tTaskDelayInit(void);
tTask * tTaskHighestReady(void);
void tTaskSchedRdy(tTask *task);
void tTaskSchedUnRdy(tTask *task);
void tTimeTaskWait(tTask *task, uint32_t ms);
void tTimeTaskWakeUp(tTask *task);
void task1DestroyFunc(void *param);

	

#endif
