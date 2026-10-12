#ifndef TEVENT_H
#define TEVENT_H

#include "tLib.h"
#include "task.h"

typedef enum _tEventType								//事件类型枚举
{
	tEventTypeUnknown,
	tEventTypeTimeout,
	tEventTypeSem,
	tEventTypeMbox,
  tEventTypeMutex,
	tEventTypeEventFlag,	
	tEventTypeMemBlock,
	tEventTypeQueue, 
}tEventType;

typedef struct _tEventControlBlock						//事件控制块
{
	tEventType type;														//事件类型
	tList waitList;															//等待该事件的任务链表
}tEventControlBlock;

void tEventInit(tEventControlBlock *ECB, tEventType eventtype);	//The Event Control Block initial function
void tEventWait(tEventControlBlock *ECB, tTask *task, void *msg, uint32_t state, uint32_t timeout);	//The function of waiting event
tTask * tEventWakeUp(tEventControlBlock *ECB, void *msg, uint32_t result);				//The function of waking the ECB up
void tEventRemoveTask(tTask *task, void *msg, uint32_t result);//The function of remove the task from the waitList of its' ECB
uint32_t tEventRemoveAll(tEventControlBlock *ECB, void *msg, uint32_t result);
uint32_t tEventWaitCount(tEventControlBlock *ECB);

#endif
