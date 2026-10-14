#ifndef THOOK_H
#define THOOK_H

#include "tConfig.h"

// Hook 函数指针类型：无参数无返回
typedef void (*tHookFunc)(void);

extern tHookFunc tTaskSwitchHook;    // 任务切换 hook：每次任务切换前调用
extern tHookFunc tIdleHook;          // 空闲任务 hook：空闲任务每次循环调用
extern tHookFunc tTickHook;          // tick hook：每个 SysTick 中断调用

void tHookInit(void);                // 初始化 hook（全部置 NULL）

#endif
