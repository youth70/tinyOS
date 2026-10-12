#include "tinyOS.h"
#include "main.h"
#include "gpio.h"
#include "usart.h"
#include <stdio.h>
#include "dwt.h"

#define DWT_GET_CYCLES()   (DWT->CYCCNT)

// ===== 全局变量 =====
tTask *currentTask;
tTask *nextTask;
tTask *idleTask;
tList taskTable[TINYOS_PRO_COUNT];
uint8_t schedLockCount;
tBitmap taskPrioBitmap;
tList tTaskDelayedList;

static tTask tTask1;
static tTask tTask2;
static tTask tTaskIdle;
static tTaskStack task1Env[1024];
static tTaskStack task2Env[1024];
static tTaskStack idleTaskEnv[TINYOS_IDLETASK_STACK_SIZE];

static volatile int task1Flag;
static volatile int task2Flag;

// ===== 队列 =====
tQueue queue;
static uint8_t queueBuffer[16 * 32] __attribute__((aligned(8)));

volatile uint32_t g_sendCount;
volatile uint32_t g_recvCount;
volatile int g_recvValue;

typedef struct _MyMsg
{
    int id;
    int value;
    char data[24];
} MyMsg;

// ===== 应用初始化 =====
void tAppInit(void)
{
    tTaskSchedInit();
    tTaskDelayInit();
    tTimerModuleInit();

    // 初始化队列：16 条消息，每条 sizeof(MyMsg) 字节
    tQueueInit(&queue, queueBuffer, 16, sizeof(MyMsg));

    tTaskInit(&tTask1, task1Entry, (void *)0, 0, &task1Env[1024]);  // 发送者
    tTaskInit(&tTask2, task2Entry, (void *)0, 1, &task2Env[1024]);  // 接收者
    tTaskInit(&tTaskIdle, idleTaskEntry, (void *)0, TINYOS_PRO_COUNT-1,
              &idleTaskEnv[TINYOS_IDLETASK_STACK_SIZE]);

    idleTask = &tTaskIdle;
}

// ===== 启动 RTOS =====
void Rtos_start(void)
{
    nextTask = tTaskHighestReady();
    tTaskRunFirst();
}

// ===== 任务1：发送消息 =====
__attribute__((noreturn)) void task1Entry(void *param)
{
    (void)param;
    tSetSysTickPeriod(10);

    int count = 0;
    MyMsg msg;

    for (;;)
    {
        msg.id = count;
        msg.value = count * 10;

        if (tQueueNotify(&queue, &msg) == tErrorNoError)
        {
            g_sendCount++;
            count++;
        }

        tTaskDelay(100);   
    }
}

// ===== 任务2：接收消息 =====
__attribute__((noreturn)) void task2Entry(void *param)
{
    (void)param;
    MyMsg msg;

    for (;;)
    {
        if (tQueueWait(&queue, &msg, 100) == tErrorNoError)
        {
            g_recvCount++;
            g_recvValue = msg.value;
            printf("Recv: id=%d, value=%d\r\n", msg.id, msg.value);
        }

        tTaskDelay(100);
    }
}

// ===== 空闲任务 =====
__attribute__((noreturn)) void idleTaskEntry(void *param)
{
    (void)param;
    for (;;)
    {
    }
}