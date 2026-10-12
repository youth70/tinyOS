#ifndef TSEM_H
#define TSEM_H

#include "tEvent.h"

typedef struct _tSem		//The Counting Semaphores data type
{
	tEventControlBlock ECB;
	
	uint32_t Count;
	uint32_t maxCount;
}tSem;

typedef struct _tSemInfo	//The Counting Semaphores information type
{
	uint32_t Count;			//当前计数
	uint32_t maxCount;		//最大计数
	uint32_t taskCount;		//等待任务数
}tSemInfo;

void tSemInit(tSem *sem, uint32_t startCount, uint32_t maxCount);	//The Counting Semaphores initial function
uint32_t tSemWait(tSem *sem, uint32_t timeout);
uint32_t tSemNoWaitGet(tSem *sem);
void tSemNotify(tSem *sem);
void tSemGetInfo(tSem *sem, tSemInfo *info);
uint32_t tSemDestroy(tSem *sem);

#endif
