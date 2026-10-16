#include "tinyOS.h"
#include "main.h"
#include "gpio.h"
#include "usart.h"
#include <stdio.h>

// ===== 内核全局变量 =====
tTask *currentTask;
tTask *nextTask;
tTask *idleTask;
tList taskTable[TINYOS_PRO_COUNT];
uint8_t schedLockCount;
tBitmap taskPrioBitmap;
tList tTaskDelayedList;

// ===== 空闲任务 =====
static tTask tTaskIdle;
static tTaskStack idleTaskEnv[TINYOS_IDLETASK_STACK_SIZE];

// ===== 测试任务 =====
static tTask      LED_task;
static tTask      UART_task;
static tTaskStack LED_taskEnv[1024];
static tTaskStack UART_taskEnv[1024];

volatile uint32_t g_t1 = 0;

// ★ 前向声明
__attribute__((noreturn)) void LED_taskEntry(void *param);
__attribute__((noreturn)) void UART_taskEntry(void *param);
__attribute__((noreturn)) void idleTaskEntry(void *param);

// ===== 应用初始化 =====
void tAppInit(void)
{
    tTaskSchedInit();
    tTaskDelayInit();
    tTimerModuleInit();
    tHookInit();

    tTaskInit(&tTaskIdle, idleTaskEntry, (void *)0, TINYOS_PRO_COUNT-1,
              &idleTaskEnv[TINYOS_IDLETASK_STACK_SIZE],
              TINYOS_IDLETASK_STACK_SIZE);
    idleTask = &tTaskIdle;

    tTaskInit(&LED_task, LED_taskEntry, (void *)0, 5, &LED_taskEnv[1024], 1024);
    tTaskInit(&UART_task, UART_taskEntry, (void *)0, 5,&UART_taskEnv[1024], 1024);

    tSetSysTickPeriod(10);
}

void Rtos_start(void)
{
    nextTask = tTaskHighestReady();
    tTaskRunFirst();
}

// ===== 空闲任务 =====
__attribute__((noreturn)) void idleTaskEntry(void *param)
{
    (void)param;
    for (;;)
		{
			if (tIdleHook != (tHookFunc)0)
			tIdleHook();
		}
}

__attribute__((noreturn)) void LED_taskEntry(void *param)
{
    (void)param;
    for (;;) 
	{
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 1);
		tTaskDelay(1000);
		HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, 0);
		tTaskDelay(1000);
	}
}

__attribute__((noreturn)) void UART_taskEntry(void *param)
{
    (void)param;
    for (;;)
    {
        printf("uart ok\r\n");
        tTaskDelay(1000);
    }
}
