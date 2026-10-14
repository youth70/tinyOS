#ifndef TASK_H
#define TASK_H

#include "stdint.h"
#include "tLib.h"


#define TINYOS_TASK_STATUS_RDY            0U                    // 就绪状态
#define TINYOS_TASK_STATUS_DESTROYED      (1U<<1)               // 已销毁
#define TINYOS_TASK_STATUS_DELAYED        (1U<<2)               // 延时中
#define TINYOS_TASK_STATUS_SUSPENDED      (1U<<3)               // 已挂起
#define TINYOS_TASK_WAIT_MASK             (0xFFU<<16)           // 等待状态的掩码位

// 栈底哨兵值
#define TINYOS_STACK_SENTINEL             0xDEADBEEFUL

struct _tEventControlBlock;                                     // ECB 前向声明（避免循环包含）

typedef struct _tEventFlagWaitInfo
{
    uint32_t mask;                                              // 关心哪些标志位
    uint32_t waitType;                                          // AND / OR 等待方式
    uint32_t clearType;                                         // 唤醒后是否清除标志
    uint32_t result;                                            // 唤醒后满足的标志位
} tEventFlagWaitInfo;

typedef uint32_t tTaskStack;                                    // 任务栈数据类型

typedef struct _tTask                                           // 任务控制块 TCB
{
    uint32_t *stack;                                            // 任务栈顶指针（保存现场后指向栈顶）
		uint32_t *stackBase;																				// 栈底
    tNode linkNode;                                             // 就绪/等待链表节点
    uint32_t delayTicks;                                        // 延时剩余 tick 数
    tNode delayNode;                                            // 延时链表节点
    uint32_t prio;                                              // 任务优先级（数值越小越高）
    uint32_t state;                                             // 任务状态位（RDY/DELAYED/SUSPENDED...）
    uint32_t slice;                                             // 剩余时间片
    uint32_t suspendCount;                                      // 挂起嵌套计数

    void (*clean)(void *param);                                 // 删除回调函数指针
    void *cleanParam;                                           // 回调参数
    uint32_t requestDeleteFlag;                                 // 请求删除标志

    struct _tEventControlBlock *waitEvent;                      // 正在等待的事件控制块
    void *eventMsg;                                             // 事件消息 / 队列接收缓冲区地址
    uint32_t waitEventResult;                                   // 等待结果（成功 / 超时 / 被删）

    tEventFlagWaitInfo waitFlagInfo;

}tTask;

typedef struct _tTaskInfo                                        // 任务信息查询结构
{
    uint32_t delayTicks;                                         // 延时剩余 tick
    uint32_t prio;                                               // 优先级
    uint32_t state;                                              // 状态位
    uint32_t slice;                                              // 剩余时间片
    uint32_t suspendCount;                                       // 挂起计数
}tTaskInfo;

void tAppInit(void);                                             // 应用初始化
void Rtos_start(void);                                           // 启动 RTOS
void tTaskSuspend(tTask *task);                                  // 挂起任务
void tTaskWakeUp(tTask *task);                                   // 唤醒任务

void tTaskSetCleanCallFunc(tTask *task, void (*clean)(void *param),
                            void *param);                        // 设置删除回调
void tTaskForceDelete(tTask *task);                              // 强制删除任务
void tTaskRequestDelete(tTask *task);                            // 请求删除任务
uint32_t tTaskIsRequestDeleted(void);                            // 查询是否被请求删除

void tTimeTaskRemove(tTask *task);                               // 从延时链表移除
void tTaskSchedRemove(tTask *task);                              // 从就绪表移除并清位图
void tTaskDeleteSelf(void);                                      // 任务删除自己

void tTaskGetInfo(tTask *task, tTaskInfo *taskinfo);             // 查询任务信息

uint32_t tTaskCheckStack(tTask *task);              // 返回 1=正常，0=溢出
void tTaskStackOverflowHook(tTask *task);           // 溢出回调


#endif
