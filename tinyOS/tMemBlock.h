#ifndef TMEMBLOCK_H
#define TMEMBLOCK_H

#include "tConfig.h"
#include "tEvent.h"

typedef struct _tMemBlock
{
    tEventControlBlock ECB;         // 用于任务等待
    void *memStart;                 // 内存池起始地址
    uint32_t blockSize;             // 每个块的大小（字节）
    uint32_t maxCount;              // 总块数
    uint32_t count;                 // 当前空闲块数
    tList freeList;                 // 空闲块链表
} tMemBlock;

typedef struct _tMemBlockInfo
{
    uint32_t count;                 // 当前空闲块数
    uint32_t maxCount;              // 总块数
    uint32_t blockSize;             // 每块大小
    uint32_t taskCount;             // 等待任务数
} tMemBlockInfo;

void tMemBlockInit(tMemBlock *memBlock, uint8_t *memStart,
                   uint32_t blockSize, uint32_t blockCnt);
void *tMemBlockWait(tMemBlock *memBlock, uint32_t timeout);
void *tMemBlockNoWaitGet(tMemBlock *memBlock);
void tMemBlockNotify(tMemBlock *memBlock, void *memBlockAddr);
void tMemBlockDestroy(tMemBlock *memBlock);
void tMemBlockGetInfo(tMemBlock *memBlock, tMemBlockInfo *info);

#endif
