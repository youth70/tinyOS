#include "tinyOS.h"

// 初始化内存块池
void tMemBlockInit(tMemBlock *memBlock, uint8_t *memStart,
                   uint32_t blockSize, uint32_t blockCnt)
{
    uint8_t *blockStart;
    uint32_t i;

    tEventInit(&(memBlock->ECB), tEventTypeMemBlock);    // 1. 初始化 ECB

    memBlock->memStart = memStart;                        // 2. 保存内存池起始地址
    memBlock->blockSize = blockSize;                      // 3. 保存每块大小
    memBlock->maxCount = blockCnt;                        // 4. 保存总块数
    memBlock->count = 0;                                  // 5. 空闲计数清零
    tListInit(&(memBlock->freeList));                     // 6. 初始化空闲链表

    blockStart = memStart;                                // 7. 从内存池起始地址开始
    for (i = 0; i < blockCnt; i++)                        // 8. 遍历每一块
    {
        tNode *node = (tNode *)blockStart;                // 9. 把块地址当节点
        tNodeInit(node);                                  // 10. 初始化节点为孤立
        tListAddLast(&(memBlock->freeList), node);        // 11. 加到空闲链表尾部

        blockStart += blockSize;                          // 12. 下一块
        memBlock->count++;                                // 13. 空闲计数 +1
    }
}

// 阻塞分配一个内存块
void *tMemBlockWait(tMemBlock *memBlock, uint32_t timeout)
{
    uint32_t status = tTaskEnterCritical();

    if (memBlock->count > 0)
    {
        // 有空闲块，直接取
        tNode *node = tListRemoveFirst(&(memBlock->freeList));
        memBlock->count--;
        tTaskExitCritical(status);
        return (void *)node;
    }
    else
    {
        // 没空闲块，等待
        tEventWait(&(memBlock->ECB), currentTask, (void *)0,
                   tEventTypeMemBlock, timeout);
        tTaskExitCritical(status);

        tTaskSched();
        return currentTask->eventMsg;
    }
}

// 非阻塞分配
void *tMemBlockNoWaitGet(tMemBlock *memBlock)
{
    uint32_t status = tTaskEnterCritical();
    void *result = (void *)0;

    if (memBlock->count > 0)
    {
        tNode *node = tListRemoveFirst(&(memBlock->freeList));
        memBlock->count--;
        result = (void *)node;
    }

    tTaskExitCritical(status);
    return result;
}

// 释放一个内存块
void tMemBlockNotify(tMemBlock *memBlock, void *memBlockAddr)
{
    uint32_t status = tTaskEnterCritical();
    tNode *node = (tNode *)memBlockAddr;

    // 重新初始化节点
    tNodeInit(node);

    if (tEventWaitCount(&(memBlock->ECB)) > 0)
    {
        // 有任务在等，直接把块给它
        tTask *task = tEventWakeUp(&(memBlock->ECB), memBlockAddr, tErrorNoError);
        if (task->prio < currentTask->prio)
        {
            tTaskSched();
        }
    }
    else
    {
        // 没任务等，放回空闲链表
        tListAddLast(&(memBlock->freeList), node);
        memBlock->count++;
    }

    tTaskExitCritical(status);
}

// 销毁内存块池
void tMemBlockDestroy(tMemBlock *memBlock)
{
    uint32_t status = tTaskEnterCritical();

    uint32_t count = tEventRemoveAll(&(memBlock->ECB), (void *)0, tErrorDel);

    tTaskExitCritical(status);

    if (count > 0)
    {
        tTaskSched();
    }
}

// 查询信息
void tMemBlockGetInfo(tMemBlock *memBlock, tMemBlockInfo *info)
{
    uint32_t status = tTaskEnterCritical();                          // 1. 进入临界区

    info->count = memBlock->count;                                    // 2. 当前空闲块数
    info->maxCount = memBlock->maxCount;                              // 3. 总块数
    info->blockSize = memBlock->blockSize;                            // 4. 每块大小
    info->taskCount = tEventWaitCount(&(memBlock->ECB));              // 5. 等待任务数

    tTaskExitCritical(status);                                        // 6. 退出临界区
}
