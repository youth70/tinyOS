    AREA |.text|, CODE, READONLY
    THUMB
    REQUIRE8
    PRESERVE8

    IMPORT currentTask      ;声明 currentTask 是一个外部符号
    IMPORT nextTask         ;声明 nextTask 是一个外部符号
    EXPORT PendSV_Handler   ;导出 PendSV_Handler 给向量表使用

PendSV_Handler
    MRS     R0, PSP                     ;R0 = PSP，当前任务的栈顶指针，读的是 PSP 这个寄存器的“值”，而这个值本身就是一个地址
    CBZ     R0, PendSVHandler_NoSave    ;如果 R0 = 0，说明是第一次启动，跳过保存

    STMDB   R0!, {R4-R11}               ;先把 R0 减 32，再把 R4~R11 依次存进去，最后 R0 指向新栈顶，压栈

    LDR     R1, =currentTask            ;R1 = &currentTask，当前正在运行任务的 TCB 的指针
    LDR     R1, [R1]                    ;R1 = currentTask（当前任务的 TCB 地址）
    STR     R0, [R1]                    ;currentTask->stack = R0，保存当前任务栈顶

PendSVHandler_NoSave
    LDR     R0, =currentTask            ;R0 = &currentTask，R0 里存的就是 currentTask 这个变量自己的地址
    LDR     R1, =nextTask               ;R1 = &nextTask
    LDR     R1, [R1]                    ;R1 = nextTask（下一个任务的 TCB 地址），把 R1 里的值当地址，去那个地址读数据，读到的值放回 R1
    STR     R1, [R0]                    ;currentTask = nextTask，切换当前任务，把 R1 里的值，写到 R0 指向的那个地址里去

    LDR     R0, [R1]                    ;R0 = nextTask->stack，取出下一个任务的栈顶
    LDMIA   R0!, {R4-R11}               ;从下一个任务的栈恢复 R4~R11，R0 回到栈顶，出栈

    MSR     PSP, R0                     ;把 R0 写回 PSP，更新任务栈指针
    ORR     LR, LR, #0x04               ;确保 EXC_RETURN 的 bit2 = 1，返回后使用 PSP
    BX      LR                          ;异常返回，开始运行下一个任务

    END