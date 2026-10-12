\# tinyOS - 基于STM32F1xx的微型RTOS



\## 项目简介

本项目是一个从零编写的嵌入式实时操作系统（RTOS）内核，基于 STM32F103 微控制器。



\## 功能特性

\* 优先级调度与时间片轮转

\* 任务延时与阻塞机制

\* 信号量 (Semaphore)

\* 互斥锁 (Mutex) 与优先级继承

\* 邮箱 (Mbox) 与消息队列 (Queue)

\* 事件标志组 (Event Flag)

\* 内存块管理 (MemBlock)

\* 软件定时器 (Timer)



\## 开发环境

\* 硬件平台：STM32F103

\* 图形配置：STM32CubeMX

\* IDE：Keil MDK v5

\* 编译器：ARMCC



\## 如何编译与运行

1\. 使用 STM32CubeMX 打开 `rtos\_test.ioc` 可查看外设配置。

2\. 使用 Keil MDK 打开 `MDK-ARM/rtos\_test.uvprojx`。

3\. 编译并烧录到 STM32F103 开发板。

4\. 串口连接 USART1 (波特率 115200)，可查看任务调度与消息打印。

