#include "tinyOS.h"

static tList tTimerList;                 // 全局定时器链表

// 初始化定时器模块
void tTimerModuleInit(void)
{
    tListInit(&tTimerList);
}

// 初始化一个定时器
void tTimerInit(tTimer *timer, uint32_t startDelayTicks, uint32_t durationTicks,
                void (*timerFunc)(void *arg), void *arg, uint32_t mode)
{
    tNodeInit(&(timer->linkNode));           // 1. 初始化链表节点
    timer->startDelayTicks = startDelayTicks; // 2. 首次延时
    timer->durationTicks = durationTicks;     // 3. 周期时长
    timer->delayTicks = 0;                    // 4. 当前剩余 tick
    timer->timerFunc = timerFunc;             // 5. 回调函数
    timer->arg = arg;                         // 6. 回调参数
    timer->state = TINYOS_TIMER_STATE_STOPPED;// 7. 状态：停止
    timer->mode = mode;                       // 8. 模式：单次 / 周期
}

// 启动定时器
void tTimerStart(tTimer *timer)
{
    uint32_t status = tTaskEnterCritical();//进入临界区

    switch (timer->state)
    {
        case TINYOS_TIMER_STATE_STOPPED:			//停止
            timer->delayTicks = timer->startDelayTicks;
            break;

        case TINYOS_TIMER_STATE_RUNNING:			//正在运行
            timer->delayTicks = timer->durationTicks;
            break;
    }

    if (timer->linkNode.nextNode == &(timer->linkNode))    // 判断是否是孤立节点（还没加入链表）
		{
			tListAddLast(&tTimerList, &(timer->linkNode));      // 加入全局定时器链表尾部
		}

    timer->state = TINYOS_TIMER_STATE_RUNNING;

    tTaskExitCritical(status);
}

// 停止定时器
void tTimerStop(tTimer *timer)
{
    uint32_t status = tTaskEnterCritical();

    if (timer->state == TINYOS_TIMER_STATE_RUNNING)
    {
        tListRemoveNode(&tTimerList, &(timer->linkNode));   // 从链表移除
        tNodeInit(&(timer->linkNode));                       // 让节点重新孤立
        timer->state = TINYOS_TIMER_STATE_STOPPED;           // 状态改为停止
    }

    tTaskExitCritical(status);
}

// 销毁定时器
void tTimerDestroy(tTimer *timer)
{
    tTimerStop(timer);
}

// 查询定时器信息
void tTimerGetInfo(tTimer *timer, tTimerInfo *info)
{
    uint32_t status = tTaskEnterCritical();

    info->startDelayTicks = timer->startDelayTicks;
    info->durationTicks = timer->durationTicks;
    info->delayTicks = timer->delayTicks;
    info->timerFunc = timer->timerFunc;
    info->arg = timer->arg;
    info->state = timer->state;
    info->mode = timer->mode;

    tTaskExitCritical(status);
}

// SysTick 调用：每 tick 递减
void tTimerModuleTickNotify(void)
{
    uint32_t status = tTaskEnterCritical();          // 1. 进入临界区
    tNode *node;
    tNode *nextNode;

    node = tTimerList.firstnode;                     // 2. 从链表第一个节点开始
    while (node != &(tTimerList.headNode))           // 3. 遍历整个链表
    {
        tTimer *timer = tNodeParent(node, tTimer, linkNode);  // 4. 反推定时器
        nextNode = node->nextNode;                   // 5. 先存下一个节点

        if (--(timer->delayTicks) == 0)              // 6. 递减倒计时，到 0？
        {
            // 7. 到时间了，调用回调
            if (timer->timerFunc)
                timer->timerFunc(timer->arg);

            // 8. 根据模式决定后续行为
            if (timer->mode == TINYOS_TIMER_MODE_PERIODIC)//周期模式
            {
                timer->delayTicks = timer->durationTicks;   // 周期：重装
            }
            else
            {
                tListRemoveNode(&tTimerList, &(timer->linkNode));  // 单次：移除
                timer->state = TINYOS_TIMER_STATE_STOPPED;
            }
        }

        node = nextNode;                             // 9. 下一个节点
    }

    tTaskExitCritical(status);                       // 10. 退出临界区
}
