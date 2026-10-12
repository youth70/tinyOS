#include "tinyOS.h"


void tMboxInit(tMbox *mbox, void **msgBuffer, uint32_t maxCount)
{
    tEventInit(&(mbox->ECB), tEventTypeMbox);   // 1. 初始化内嵌 ECB

    mbox->msgBuffer = msgBuffer;                // 2. 保存消息缓冲区
    mbox->maxCount = maxCount;                  // 3. 保存最大消息数
    mbox->Count = 0;                            // 4. 消息数清零
    mbox->read = 0;                             // 5. 读索引清零
    mbox->write = 0;                            // 6. 写索引清零
}

uint32_t tMboxWait(tMbox *mbox, void **msg, uint32_t timeout)
{
    uint32_t status = tTaskEnterCritical();              // 1. 进入临界区

    if (mbox->Count > 0)                                  // 2. 有消息？
    {
        *msg = mbox->msgBuffer[mbox->read++];             // 3. 取消息
        if (mbox->read >= mbox->maxCount)
            mbox->read = 0;                               // 4. 环形回绕
        mbox->Count--;                                    // 5. 消息数减 1
        tTaskExitCritical(status);
        return tErrorNoError;                             // 6. 返回成功
    }
    else                                                  // 7. 没消息
    {
        tEventWait(&(mbox->ECB), currentTask, (void *)0, tEventTypeMbox, timeout);  // 8. 等
        tTaskExitCritical(status);

        tTaskSched();                                     // 9. 让出 CPU
        *msg = currentTask->eventMsg;                     // 10. 取消息
        return currentTask->waitEventResult;              // 11. 返回结果
    }
}

uint32_t tMboxNoWaitGet(tMbox *mbox, void **msg)
{
    uint32_t status = tTaskEnterCritical();          // 1. 进入临界区

    if (mbox->Count > 0)                              // 2. 有消息？
    {
        *msg = mbox->msgBuffer[mbox->read++];         // 3. 取消息
        if (mbox->read >= mbox->maxCount)
            mbox->read = 0;                           // 4. 环形回绕
        mbox->Count--;                                // 5. 消息数减 1
        tTaskExitCritical(status);
        return tErrorNoError;                         // 6. 返回成功
    }
    else                                              // 7. 没消息
    {
        tTaskExitCritical(status);
        return tErrorResourceUnavailable;             // 8. 立刻返回错误
    }
}

uint32_t tMboxNotify(tMbox *mbox, void *msg, uint32_t notifyOption)   //邮箱的发送操作
{
    uint32_t status = tTaskEnterCritical();//进入临界区
    uint32_t err = tErrorNoError;

    if (tEventWaitCount(&(mbox->ECB)) > 0)
    {
        // 有任务在等，直接把消息给它
        tTask *task = tEventWakeUp(&(mbox->ECB), msg, tErrorNoError);
        if (task->prio < currentTask->prio)
        {
            tTaskSched();
        }
    }
    else
    {
        // 没有等待者，放入缓冲区
        if (mbox->Count >= mbox->maxCount)
        {
            // 缓冲区满
            if (notifyOption & TINYOS_MBOX_NOTIFY_OVERWRITE)
            {
                // 覆盖最旧的消息
                mbox->msgBuffer[mbox->write++] = msg;
                if (mbox->write >= mbox->maxCount)
                    mbox->write = 0;

                mbox->read++;
                if (mbox->read >= mbox->maxCount)
                    mbox->read = 0;
            }
            else
            {
                // 非覆盖模式：返回错误
                err = tErrorResourceFull;
            }
        }
        else
        {
            mbox->msgBuffer[mbox->write++] = msg;
            if (mbox->write >= mbox->maxCount)
                mbox->write = 0;
            mbox->Count++;
        }
    }

    tTaskExitCritical(status);
    return err;
}

void tMboxFlush(tMbox *mbox)                   //把邮箱里的旧消息全部扔掉,邮箱清空
{
    uint32_t status = tTaskEnterCritical();    // 1. 进入临界区

    if (mbox->Count > 0)                       // 2. 邮箱里有消息？
    {
        mbox->Count = 0;                       // 3. 消息数清零
        mbox->read = 0;                        // 4. 读索引归零
        mbox->write = 0;                       // 5. 写索引归零
    }

    tTaskExitCritical(status);                 // 6. 退出临界区
}

uint32_t tMboxDestroy(tMbox *mbox)
{
    uint32_t status = tTaskEnterCritical();                          // 1. 进入临界区

    uint32_t count = tEventRemoveAll(&(mbox->ECB), (void *)0, tErrorDel);  // 2. 唤醒所有等待者

    mbox->Count = 0;                                                  // 3. 消息数清零
    mbox->read = 0;                                                   // 4. 读索引归零
    mbox->write = 0;                                                  // 5. 写索引归零

    tTaskExitCritical(status);                                        // 6. 退出临界区

    if (count > 0)                                                    // 7. 如果有任务被唤醒
    {
        tTaskSched();                                                 // 8. 请求调度
    }
    return count;                                                     // 9. 返回被唤醒的任务数
}

void tMboxGetInfo(tMbox *mbox, tMboxInfo *info)
{
    uint32_t status = tTaskEnterCritical();              // 1. 进入临界区

    info->Count = mbox->Count;                            // 2. 当前消息数
    info->maxCount = mbox->maxCount;                      // 3. 最大消息数
    info->taskCount = tEventWaitCount(&(mbox->ECB));      // 4. 等待任务数

    tTaskExitCritical(status);                            // 5. 退出临界区
}
