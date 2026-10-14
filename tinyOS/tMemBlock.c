#include "tinyOS.h"

// 初始化内存块池
void tMemBlockInit(tMemBlock *memBlock, uint8_t *memStart,                   // memBlock：内存块池对象；memStart：内存池起始地址
                   uint32_t blockSize, uint32_t blockCnt)                    // blockSize：每块大小；blockCnt：总块数
{
    uint8_t *blockStart;                                                     // 遍历各块起始地址的临时指针
    uint32_t i;                                                              // 循环变量

    // 参数合法性校验：任一非法直接返回，避免越界写坏相邻内存
    if (memBlock == (tMemBlock *)0 ||                                        // memBlock 为空指针
        memStart == (uint8_t *)0 ||                                          // memStart 为空指针
        blockCnt == 0 ||                                                     // 总块数为 0
        blockSize < sizeof(tNode))                                           // 每块放不下一个 tNode（8 字节）
    {
        return;
    }

    tEventInit(&(memBlock->ECB), tEventTypeMemBlock);                        // 初始化内嵌 ECB，类型为内存块，等待链表置空

    memBlock->memStart = memStart;                                           // 保存内存池起始地址
    memBlock->blockSize = blockSize;                                         // 保存每个块的字节大小
    memBlock->maxCount = blockCnt;                                           // 保存内存块池总块数（容量）
    memBlock->count = 0;                                                     // 当前空闲块计数清零，稍后循环累加
    tListInit(&(memBlock->freeList));                                        // 初始化空闲链表为空链表

    blockStart = memStart;                                                   // 从内存池起始地址开始遍历
    for (i = 0; i < blockCnt; i++)                                           // 对每一块依次建立节点并挂入空闲链表
    {
        tNode *node = (tNode *)blockStart;                                   // 把当前块的首地址直接当作 tNode 使用
        tNodeInit(node);                                                     // 初始化节点为孤立节点（前后指针指向自己）
        tListAddLast(&(memBlock->freeList), node);                           // 把该节点插入空闲链表尾部

        blockStart += blockSize;                                             // 指针后移一个 blockSize，指向下一块首地址
        memBlock->count++;                                                   // 空闲块计数加 1
    }
}

// 阻塞分配一个内存块
void *tMemBlockWait(tMemBlock *memBlock, uint32_t timeout)                    // memBlock：内存池对象；timeout：超时 tick 数（0 表示无限等待）
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区，关中断并保存原中断状态

    if (memBlock->count > 0)                                                  // 判断是否还有空闲块
    {
        tNode *node = tListRemoveFirst(&(memBlock->freeList));                // 从空闲链表头部摘下一个节点（节点地址即块首地址）
        memBlock->count--;                                                    // 空闲块计数减 1
        tTaskExitCritical(status);                                            // 退出临界区，恢复原中断状态
        return (void *)node;                                                  // 返回块首地址给调用者
    }
    else                                                                      // 没有空闲块，需要等待
    {
        tEventWait(&(memBlock->ECB), currentTask, (void *)0,                  // 把当前任务挂到内存池等待链表
                   tEventTypeMemBlock, timeout);                              // 事件类型为内存块，可选超时
        tTaskExitCritical(status);                                            // 退出临界区

        tTaskSched();                                                         // 主动让出 CPU，切换到其他就绪任务
        return currentTask->eventMsg;                                         // 被唤醒后取出块地址（超时/被删则为 NULL）
    }
}

// 非阻塞分配
void *tMemBlockNoWaitGet(tMemBlock *memBlock)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区
    void *result = (void *)0;                                                 // 默认返回 NULL

    if (memBlock->count > 0)                                                  // 有空闲块？
    {
        tNode *node = tListRemoveFirst(&(memBlock->freeList));                // 从空闲链表头部摘一个
        memBlock->count--;                                                    // 空闲块计数减 1
        result = (void *)node;                                                // 记录块首地址
    }

    tTaskExitCritical(status);                                                // 退出临界区
    return result;                                                            // 返回块地址或 NULL
}

// 释放一个内存块
void tMemBlockNotify(tMemBlock *memBlock, void *memBlockAddr)                 // memBlockAddr：要归还的块首地址
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区，关中断并保存原中断状态
    tNode *node = (tNode *)memBlockAddr;                                      // 把块首地址强制转为 tNode 指针

    tNodeInit(node);                                                          // 初始化节点为孤立节点（清除旧的链表残留）

    if (tEventWaitCount(&(memBlock->ECB)) > 0)                                // 判断是否有任务在等待空闲块
    {
        tTask *task = tEventWakeUp(&(memBlock->ECB), memBlockAddr, tErrorNoError); // 唤醒一个等待者，并把块地址作为消息传给它
        if (task->prio < currentTask->prio)                                   // 被唤醒任务优先级是否比当前更高
        {
            tTaskSched();                                                     // 是则立即触发调度
        }
    }
    else                                                                      // 没有任务等待
    {
        tListAddLast(&(memBlock->freeList), node);                            // 把该块插回空闲链表尾部
        memBlock->count++;                                                    // 空闲块计数加 1
    }

    tTaskExitCritical(status);                                                // 退出临界区
}

// 销毁内存块池
void tMemBlockDestroy(tMemBlock *memBlock)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    uint32_t count = tEventRemoveAll(&(memBlock->ECB), (void *)0, tErrorDel); // 唤醒所有等待者，结果设为 tErrorDel

    tTaskExitCritical(status);                                                // 退出临界区

    if (count > 0)                                                            // 有任务被唤醒？
    {
        tTaskSched();                                                         // 触发调度
    }
}

// 查询信息
void tMemBlockGetInfo(tMemBlock *memBlock, tMemBlockInfo *info)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    info->count = memBlock->count;                                            // 当前空闲块数
    info->maxCount = memBlock->maxCount;                                      // 总块数
    info->blockSize = memBlock->blockSize;                                    // 每块大小
    info->taskCount = tEventWaitCount(&(memBlock->ECB));                      // 等待任务数

    tTaskExitCritical(status);                                                // 退出临界区
}
