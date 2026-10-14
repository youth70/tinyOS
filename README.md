# tinyOS — 从零实现的 ARM Cortex-M3 实时操作系统内核

一个手写的嵌入式 RTOS，涵盖任务调度、任务同步、任务通信、
内存管理、软件定时器五大内核模块。

## 特性

- 任务调度：优先级抢占 + 时间片轮转
- 同步：信号量、互斥锁（优先级继承）、事件标志
- 通信：邮箱、队列
- 内存：固定大小内存块池
- 定时：软件定时器（单次 / 周期）
- 调试：栈溢出检测、Hook 机制

## 平台

- STM32F103（Cortex-M3，72MHz）
- Keil MDK-ARM V5.06
- STM32CubeMX

## 目录结构

- `Core/` — STM32CubeMX 生成的核代码
- `Drivers/` — STM32 HAL 库
- `MDK-ARM/` — Keil 工程
- `tinyOS/` — RTOS 内核源码

## 快速开始

1. Keil µVision 5 打开 `MDK-ARM/rtos_test.uvprojx`
2. 编译、烧录到 STM32F103
3. 串口（115200）观察输出

## License

MIT