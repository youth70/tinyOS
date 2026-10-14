#include "tinyOS.h"

void tMboxInit(tMbox *mbox, void **msgBuffer, uint32_t maxCount)
{
    // 参数合法性校验：任一非法直接返回
    if (mbox == (tMbox *)0 ||                                                 // mbox 为空指针
        msgBuffer == (void **)0 ||                                            // 消息缓冲区为空
        maxCount == 0)                                                        // 最大消息数为 0
    {
        return;
    }

    tEventInit(&(mbox->ECB), tEventTypeMbox);                                 // 初始化内嵌 ECB，类型为邮箱

    mbox->msgBuffer = msgBuffer;                                              // 保存消息缓冲区
    mbox->maxCount = maxCount;                                                // 保存最大消息数
    mbox->Count = 0;                                                          // 消息数清零
    mbox->read = 0;                                                           // 读索引清零
    mbox->write = 0;                                                          // 写索引清零
}

uint32_t tMboxWait(tMbox *mbox, void **msg, uint32_t timeout)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    if (mbox->Count > 0)                                                      // 有消息？
    {
        *msg = mbox->msgBuffer[mbox->read++];                                 // 取消息（指针）
        if (mbox->read >= mbox->maxCount)                                     // 读索引回绕
            mbox->read = 0;
        mbox->Count--;                                                        // 消息数减 1
        tTaskExitCritical(status);
        return tErrorNoError;                                                 // 返回成功
    }
    else                                                                      // 没消息，等待
    {
        tEventWait(&(mbox->ECB), currentTask, (void *)0, tEventTypeMbox, timeout); // 挂到邮箱等待链表
        tTaskExitCritical(status);

        tTaskSched();                                                         // 让出 CPU
        *msg = currentTask->eventMsg;                                         // 取出消息指针
        return currentTask->waitEventResult;                                  // 返回结果
    }
}

uint32_t tMboxNoWaitGet(tMbox *mbox, void **msg)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    if (mbox->Count > 0)                                                      // 有消息？
    {
        *msg = mbox->msgBuffer[mbox->read++];                                 // 取消息
        if (mbox->read >= mbox->maxCount)
            mbox->read = 0;
        mbox->Count--;
        tTaskExitCritical(status);
        return tErrorNoError;                                                 // 返回成功
    }
    else                                                                      // 没消息
    {
        tTaskExitCritical(status);
        return tErrorResourceUnavailable;                                     // 立即返回错误
    }
}

uint32_t tMboxNotify(tMbox *mbox, void *msg, uint32_t notifyOption)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区
    uint32_t err = tErrorNoError;                                             // 默认成功

    if (tEventWaitCount(&(mbox->ECB)) > 0)                                    // 有任务在等？
    {
        tTask *task = tEventWakeUp(&(mbox->ECB), msg, tErrorNoError);         // 唤醒一个等待者，并把消息指针传给它
        if (task->prio < currentTask->prio)                                   // 被唤醒任务优先级更高？
        {
            tTaskSched();                                                     // 触发调度
        }
    }
    else                                                                      // 无等待者
    {
        if (mbox->Count >= mbox->maxCount)                                    // 缓冲区满？
        {
            if (notifyOption & TINYOS_MBOX_NOTIFY_OVERWRITE)                  // 覆盖模式
            {
                mbox->msgBuffer[mbox->write++] = msg;                         // 覆盖最旧的消息
                if (mbox->write >= mbox->maxCount)
                    mbox->write = 0;

                mbox->read++;                                                 // 读索引后移（丢弃旧消息）
                if (mbox->read >= mbox->maxCount)
                    mbox->read = 0;
            }
            else                                                              // 非覆盖模式
            {
                err = tErrorResourceFull;                                     // 返回错误
            }
        }
        else                                                                  // 有空位
        {
            mbox->msgBuffer[mbox->write++] = msg;                             // 存入消息
            if (mbox->write >= mbox->maxCount)
                mbox->write = 0;
            mbox->Count++;                                                    // 消息数加 1
        }
    }

    tTaskExitCritical(status);
    return err;
}

void tMboxFlush(tMbox *mbox)                                                  // 清空邮箱消息
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    if (mbox->Count > 0)                                                      // 有消息？
    {
        mbox->Count = 0;                                                      // 消息数清零
        mbox->read = 0;                                                       // 读索引归零
        mbox->write = 0;                                                      // 写索引归零
    }

    tTaskExitCritical(status);
}

uint32_t tMboxDestroy(tMbox *mbox)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    uint32_t count = tEventRemoveAll(&(mbox->ECB), (void *)0, tErrorDel);     // 唤醒所有等待者

    mbox->Count = 0;                                                          // 消息数清零
    mbox->read = 0;                                                           // 读索引归零
    mbox->write = 0;                                                          // 写索引归零

    tTaskExitCritical(status);

    if (count > 0)                                                            // 有任务被唤醒？
    {
        tTaskSched();                                                         // 请求调度
    }
    return count;
}

void tMboxGetInfo(tMbox *mbox, tMboxInfo *info)
{
    uint32_t status = tTaskEnterCritical();                                   // 进入临界区

    info->Count = mbox->Count;                                                // 当前消息数
    info->maxCount = mbox->maxCount;                                          // 最大消息数
    info->taskCount = tEventWaitCount(&(mbox->ECB));                          // 等待任务数

    tTaskExitCritical(status);
}
