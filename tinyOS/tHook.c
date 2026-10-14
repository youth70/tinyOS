#include "tHook.h"

tHookFunc tTaskSwitchHook = (tHookFunc)0;    // 任务切换 hook，默认无
tHookFunc tIdleHook       = (tHookFunc)0;    // 空闲 hook，默认无
tHookFunc tTickHook       = (tHookFunc)0;    // tick hook，默认无

// 初始化所有 hook 为 NULL
void tHookInit(void)
{
    tTaskSwitchHook = (tHookFunc)0;          // 清空任务切换 hook
    tIdleHook       = (tHookFunc)0;          // 清空空闲 hook
    tTickHook       = (tHookFunc)0;          // 清空 tick hook
}
