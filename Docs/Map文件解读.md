# Map 文件解读

本文以本项目 STM32F4 BSP 和 GNU ld 生成的 Map 文件为讨论对象。Map 中的具体地址与大小属于某次构建快照；源码、配置、工具链或链接脚本变化后，应以同一次构建生成的 ELF 和 Map 为准。

## 概述

Map 文件是链接器在生成 ELF 文件时输出的链接映射报告。它记录目标文件、静态库、输入段、输出段、符号和内存区域之间的对应关系。对嵌入式工程而言，Map 文件的主要用途不是查看程序的执行流程，而是回答以下问题：程序被放到了哪一个地址，Flash 和 RAM 分别使用了多少空间，某个函数或变量由哪个源文件产生，以及某个库为什么被链接进来。

本文重点建立从“源代码对象—输入段—输出段—物理内存区域—运行时使用”的完整认识。相关概念和项目内存布局可结合 [`编译链接与运行时内存布局.md`](编译链接与运行时内存布局.md) 阅读。

## Map 文件生成流程

本项目使用 ARM GCC、CMake 和 Ninja 构建 STM32F4 固件。构建过程可以抽象为：

```text
.c/.s/.S 源文件
        │ 编译或汇编
        ▼
可重定位目标文件 .o/.obj
        │ 与静态库链接
        ▼
rtthread-stm32f4.elf + rtthread-stm32f4.map
        │ objcopy 转换
        ├── rtthread-stm32f4.hex
        └── rtthread-stm32f4.bin
```

Map 文件在链接阶段生成。当前工程的链接选项包含：

```text
-TF:/.../BSP/STM32F4/STM32F407xx_FLASH.ld
-Wl,--gc-sections
-Wl,-Map=.../build/stm32f4-debug/rtthread-stm32f4.map
```

其中，链接脚本决定内存布局，`--gc-sections` 删除不可达且未被强制保留的输入段，`-Map` 指定 Map 文件的输出路径。

构建前应在 PowerShell 中**临时**加入 UCRT64 工具目录并查看当前实际 Preset：

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
cmake --list-presets=configure
cmake --list-presets=build
```

确认 STM32F4 对应的配置和构建 Preset 后，再执行相应的 `cmake --preset <configure-preset>` 与 `cmake --build --preset <build-preset>`。Map、ELF、BIN 和 HEX 的实际路径应根据 Preset 的 `binaryDir` 和构建输出确认，不能只凭文档中的示例路径推断。

## 解析 Map 文件

Map 文件不应简单地从第一行读到最后一行。更有效的阅读顺序是：

```text
Archive member included
        │ 查看静态库中被提取的成员及其触发符号
        ▼
Discarded input sections
        │ 查看被垃圾回收删除的输入段
        ▼
Memory Configuration
        │ 查看芯片提供的物理内存范围
        ▼
Linker script and memory map
        │ 查看最终输出段、地址、大小和来源
        ▼
符号、堆栈边界及调试信息节
```

这几个部分分别回答“链接了什么”“删除了什么”“可以放到哪里”和“最终放到了哪里”。

**Archive member included to satisfy reference by file (symbol)**

> 该部分说明链接器为了满足符号引用，从静态库 `.a` 中提取了哪些目标文件成员。例如：
>
> ```text
> .../libgcc.a(_aeabi_uldivmod.o)
>     .../stm32f4xx_hal_rcc.c.obj (__aeabi_uldivmod)
> ```
>
> 这表示 `stm32f4xx_hal_rcc.c.obj` 引用了 `__aeabi_uldivmod`，链接器因此从 `libgcc.a` 中提取 `_aeabi_uldivmod.o`。该符号通常是 GCC 为整数除法生成的 ARM EABI 辅助函数。
>
> 静态库不会因为出现在 `LOAD` 行中就全部进入固件。链接器通常**按需提取**其中的目标文件成员，例如从 `libc_nano.a` 中提取 `memset.o`、`strlen.o` 或其他实际需要的模块。因此，这一部分适合用于回答“为什么某个库被引入”和“是谁触发了这个依赖”。
>
> 需要注意，目标文件成员被提取后，其中的函数段仍可能因为 `--gc-sections` 被进一步删除。因此，`Archive member included` 表示它**参与了解析过程，不等于其中全部代码最终保留**。
>

**Discarded input sections**

> 该部分列出**没有**进入最终输出段的输入段。例如：
>
> ```text
> .text.HAL_DeInit
>                 0x00000000       0x5c  stm32f4xx_hal.c.obj
> ```
>
> 这里的 `0x00000000` 不是函数实际运行地址，而是该输入段没有被分配到最终 Flash 或 RAM 的标志。`0x5c` 是它在原始目标文件中的大小，但它已经被丢弃，因此不计入当前固件的实际代码占用。
>
> 造成输入段被丢弃的常见原因包括：函数或数据无法从链接根节点沿引用关系到达，或者链接脚本显式将其丢弃。本项目启用 `-ffunction-sections`、`-fdata-sections` 和 `--gc-sections` 后，链接器可以按函数或数据对象粒度回收输入段。中断向量表、RT-Thread 自动初始化表和 FinSH/MSH 命令表等通过边界符号枚举的内容，通常需要链接脚本中的 `KEEP()` 防止被误删。
>
> 因此，`Discarded input sections` 适合用于查找无效代码、确认垃圾回收结果，以及分析“源文件中明明有这个函数，但为什么最终地址为零”的问题。
>

**Memory Configuration**

> 本项目链接脚本 `BSP/STM32F4/STM32F407xx_FLASH.ld` 定义了：
>
> ```text
> RAM      0x20000000   0x00020000   128 KiB
> CCMRAM   0x10000000   0x00010000    64 KiB
> FLASH    0x08000000   0x00100000  1024 KiB
> ```
>
> `ORIGIN` 是区域起始地址，`LENGTH` 是区域容量。属性中的 `r`、`w`、`x` 分别表示可读、可写和可执行。
>
> 这部分只描述芯片的可用物理内存，并不表示当前程序已经使用了这些空间。当前程序的实际使用量要到后面的输出段中查看。
>

**Linker script and memory map**

> 这是 Map 文件最重要的部分。`Linker script and memory map` 不是一个段名，而是“链接脚本及其生成的内存映射”这一报告区域的标题。它显示链接器如何**把保留的输入段组合成输出段，并为输出段和符号分配最终地址**。
>
> 阅读这一部分前，需要区分三个层次：
>
> > **输入段**存在于各个**可重定位目标文件**中，例如 `.text.main`、`.data.device`；**输出段**由链接脚本合并输入段形成，例如最终的 `.text`、`.data`；`MEMORY` 中的 `FLASH`、`RAM` 和 `CCMRAM` 则是输出段最终占用的物理内存区域。
>
> 典型 Map 片段如下：
>
> ```text
> .text           0x080001c0       0x8ad8
>  *(.text)
>  *(.text*)
>  .text.main     0x08008c60         0x20  main.c.obj
>                 0x08008c60                main
>  *fill*         0x08008c80          0x4
> ```
>
> 其层次关系为：
>
> ```text
> .text                         输出段
> ├── *(.text)、*(.text*)       链接脚本的输入段匹配规则
> ├── .text.main                被匹配的输入段
> │   ├── main.c.obj            输入段来源
> │   └── main                  输入段内的符号
> └── *fill*                    为满足对齐而插入的填充
> ```
>
> 括号外的 `*` 表示匹配所有输入文件，括号内 `.text*` 的 `*` 表示匹配段名的任意后缀。因此 `*(.text*)` 可以收集 `.text`、`.text.main` 和 `.text.HAL_RTC_MspInit` 等输入段。多个输入段被收集后只形成一个 `.text` 输出段，各输入段的地址由链接器根据脚本顺序、大小和对齐要求确定。
>
> 下面是一份构建快照中的主要输出段示例，数值只用于说明 Map 行格式：
>
> ```text
> .isr_vector     0x08000000      0x188
> .text           0x080001c0     0x8ad8
> .rodata         0x08008c98      0xe78
> .rti_fn         0x08009b10       0x1c
> FSymTab         0x08009b2c       0x84
> .data           0x20000000       0xa4  load address 0x08009bc0
> .ccmram         0x10000000        0x0
> .bss            0x200000a4      0xd10
> ```
>
> 输出段行通常采用以下形式：
>
> ```text
> 段名称          起始地址       段大小
> ```
>

**`.isr_vector`**

> `.isr_vector` 是 Cortex-M4 的中断向量表。本工程将它放在 `0x08000000`，即 Flash 起始位置。复位后的处理器会从向量表读取**初始主栈指针**和 `Reset_Handler` 地址，因此该段的位置具有硬件约束，不能随意移动。

**`.text`**

> `.text` 保存可执行代码，包括启动文件、HAL 驱动、RT-Thread 内核、CPU 移植层和应用函数。输出段总大小为 `0x8ad8`，也就是 35544 字节。
>
> 在 `.text` 内部可以继续按函数查看：
>
> ```text
> .text.HAL_UART_IRQHandler
>                 0x080028a0      0x54c  stm32f4xx_hal_uart.c.obj
>                 0x080028a0              HAL_UART_IRQHandler
> 
> .text.main      0x08008c60       0x20  main.c.obj
>                 0x08008c60              main
> ```
>
> 对函数而言，第二列的十六进制数是函数输入段大小。例如 `0x54c` 等于 1356 字节，`0x20` 等于 32 字节。函数的结束地址可以用“起始地址加大小”计算，但**结束地址本身不属于该函数**。
>

**`.rodata`**

> `.rodata` 保存字符串、常量和只读数组，例如应用中的输出字符串、FinSH 命令名称和驱动配置表。上述快照中的大小为 `0xe78`，即 3704 字节。增加大量日志文本或查找表时，应重点观察这一段的增长。

**`.rti_fn` 与 `FSymTab`**

> `.rti_fn` 是 RT-Thread 自动初始化函数表。RT-Thread 使用 `.rti_fn.*` 输入段登记初始化项，链接脚本按照阶段顺序排列这些输入段，启动阶段再通过边界符号依次调用。
>
> `FSymTab` 是 FinSH/MSH 命令描述表。`MSH_CMD_EXPORT()` 等宏会生成包含命令名、说明和函数地址的描述对象，并将其放入 `FSymTab` 输入段。链接脚本使用：
>
> ```ld
> FSymTab :
> {
>     __fsymtab_start = .;  /* 命令表起始地址 */
> 
>     KEEP(*(FSymTab))      /* 放置命令描述对象 */
> 
>     __fsymtab_end = .;    /* 命令表结束地址 */
> } > FLASH
> ```
>
> 其中 `.` 是链接器的位置计数器，表示“当前正在放置内容的地址”。这行代码把当前地址命名为 `__fsymtab_start`：
>
> `KEEP()` 表示即使启用了 `--gc-sections`，也必须保留匹配的输入段。FinSH 在运行时遍历 `__fsymtab_start` 到 `__fsymtab_end` 之间的连续描述对象来发现命令。描述对象中保存的函数地址又会形成对命令函数的引用，因此相应函数通常也会被保留。`RT_USED` 主要防止编译器省略对象，`KEEP()` 则**防止链接器删除输入段**，二者作用阶段不同。
>
> 同类机制也用于中断向量表和 RT-Thread 自动初始化表。分析“宏已经写了但命令或初始化项不存在”时，应同时检查：输入段是否生成、所属目标文件是否参与链接、链接脚本是否匹配，以及 `KEEP()` 是否覆盖了正确段名。

**ARM 异常信息和 C 运行库数组**

> `.ARM.extab`、`.ARM.exidx` 保存 ARM 异常展开相关数据，`.preinit_array`、`.init_array` 和 `.fini_array` 保存 C/C++ 运行库使用的函数指针数组。它们可能实际位于 Flash 并进入运行映像，不能因为名称不像普通代码就当作调试信息删除或忽略。相反，`.debug_*` 通常只是 ELF 中的非装载调试信息，二者必须区分。

**`.data` 的运行地址和加载地址**

> Map 中的 `.data` 是最容易误读的段之一：
>
> ```text
> .data  0x20000000  0xa4  load address 0x08009bc0
> ```
>
> 这里同时出现两个地址：
>
> - `0x20000000` 是运行地址 VMA，程序运行时变量位于 RAM；
> - `0x08009bc0` 是加载地址 LMA，变量的初始值存放在 Flash。
>
> 复位启动代码会将 Flash 中的初始值复制到 RAM：
>
> ```text
> Flash 0x08009bc0  ──复制──>  RAM 0x20000000
> ```
>
> 因此，`.data` **同时消耗 Flash 和 RAM**。上述快照中的 `.data` 大小为 `0xa4`，即 164 字节。
>

**`.bss`、`COMMON`、堆和栈边界**

> `.bss` 保存未初始化或初始化为零的全局变量和静态变量：
>
> ```text
> .bss  0x200000a4  0xd10
> ```
>
> 它**占用 RAM，但不需要在 Flash 中保存初始数据**。启动代码会在进入 RT-Thread 前将该区域清零。示例中的 `.bss` 大小为 `0xd10`，即 3344 字节。
>
> 在 `.bss` 内常会看到：
>
> ```text
> *(COMMON)
> ```
>
> `COMMON` 是旧式 tentative definition（暂定定义）使用的特殊输入类别，例如采用 `-fcommon` 时某些未初始化全局变量可能出现在这里。链接脚本把它们并入 `.bss`。较新的 GCC 默认使用 `-fno-common`，所以该规则可能没有实际匹配内容，但为了兼容不同代码和工具链通常仍会保留。
>
> 下面的 Map 片段集中展示了位置计数器、对齐、符号、堆栈边界、`PROVIDE()` 和 `ASSERT()`：
>
> ```text
> *(COMMON)
>                 0x20000df4                        . = ALIGN (0x4)
>                 0x20000df4                        _ebss = .
>                 0x20000df4                        __bss_end__ = _ebss
>                 0x20000df8                        __heap_start__ = ALIGN (_ebss, 0x8)
>                 0x2001e000                        __heap_end__ = (_estack - _Min_Stack_Size)
>                 0x2001e000                        __msp_stack_limit__ = __heap_end__
>                 [!provide]                        PROVIDE (end = __heap_start__)
>                 [!provide]                        PROVIDE (_end = __heap_start__)
>                 0x00000001                        ASSERT ((__heap_end__ >= (__heap_start__ + _Min_Heap_Size)), ...)
> ```
>
> 逐行解读如下：
>
> - `.` 是链接器位置计数器；`. = ALIGN(0x4)` 将当前位置向上按 4 字节对齐。本例原地址已经满足要求，因此仍为 `0x20000df4`。
> - `_ebss` 和 `__bss_end__` 是 `.bss` 结束边界的两个符号名；边界地址本身不属于 `.bss` 有效区间。
> - `ALIGN(_ebss, 0x8)` 把 RT-Thread 系统堆起点按 8 字节对齐，从 `0x20000df4` 推进到 `0x20000df8`，中间 4 字节是对齐空隙。
> - `__heap_end__ = _estack - _Min_Stack_Size` 从 RAM 顶部扣除 MSP 预算，得到系统堆上界；`__msp_stack_limit__` 是同一边界的描述性符号。
> - `PROVIDE()` 仅在符号被引用且尚未由其他位置定义时提供兼容符号。`[!provide]` 表示本次链接没有实际提供该符号，常见原因是没有引用或已经存在定义，不是链接错误。
> - `ASSERT()` 前面的 `0x00000001` 表示表达式为真，静态布局至少留下了 `_Min_Heap_Size` 要求的空间；若为假，链接会以给定消息失败。
>
> 本项目链接脚本定义：
>
> ```ld
> _Min_Heap_Size  = 0x200;
> _Min_Stack_Size = 0x2000;
> ```
>
> `_Min_Heap_Size` 是链接检查要求的最小系统堆容量，即 512 字节，并不把 RT-Thread 堆限制为 512 字节。`_Min_Stack_Size` 则从主 SRAM 顶部实际划出 8 KiB 的 MSP 地址预算。按照上面的示例地址，系统堆的可管理范围为：
>
> ```text
> [0x20000df8, 0x2001e000)
> 大小 = 0x1d208 = 119304 字节，约 116.5 KiB
> ```
>
> 这是边界符号之间的原始地址跨度；内存管理器还需要块头、对齐和空闲链表等元数据，所以应用实际能够申请到的有效载荷会略小。
>
> 主 SRAM 的关系为：
>
> ```text
> 低地址  0x20000000
>         ├── .data、.bss 等静态数据
>         ├── 0x20000df4  _ebss
>         ├── 4 字节对齐空隙
>         ├── 0x20000df8  __heap_start__
>         │       RT-Thread 系统堆地址范围
>         ├── 0x2001e000  __heap_end__ / __msp_stack_limit__
>         │       MSP 预留地址范围，栈从高地址向低地址增长
> 高地址  └── 0x20020000  _estack
> ```
>
> 这些符号只描述地址规划。`__msp_stack_limit__` 不是 Cortex-M4 的硬件越界保护，`ASSERT()` 也不能证明运行时不会堆耗尽或栈溢出。RT-Thread 动态线程栈通常从系统堆中申请，静态线程栈通常已经计入 `.bss`；统计实际 RAM 使用时不能把它们与整个堆容量再次相加，否则会重复计算。
>

**`.ccmram` 与 `NOLOAD`**

> 本项目链接脚本把 `.ccmram` 定义为位于 STM32F407 CCMRAM 的自定义输出段：
>
> ```ld
> .ccmram (NOLOAD) :
> {
>     _sccmram = .;
>     *(.ccmram)
>     *(.ccmram*)
>     _eccmram = .;
> } > CCMRAM
> ```
>
> `*(.ccmram*)` 可以收集 `.ccmram`、`.ccmram.filter`、`.ccmram.control` 等多个输入段，但它们最终仍被连续合并到一个 `.ccmram` 输出段，并不自动形成相互隔离的物理分区。Map 中应继续展开输入段和来源目标文件，才能判断具体对象的最终地址和贡献。
>
> `NOLOAD` 表示固件没有需要下载到该输出段的加载载荷，不代表这块 RAM 不会被使用。本项目启动代码也不会复制或清零 `.ccmram`，所以对象必须由程序显式初始化。CCMRAM 只适合 CPU 访问，不能用作 DMA 缓冲区。若 Map 中 `.ccmram` 大小为零，表示当前没有保留下来的匹配对象，64 KiB 物理 CCMRAM 仍然存在，但尚未被该输出段占用。

**Flash 和 RAM 占用的计算**

> 对本工程，应分别计算 Flash 和 RAM，不能把所有 Map 文件中的数字简单相加。
>
> Flash 主要包括：
>
> ```text
> .isr_vector + .text + .rodata + .rti_fn + FSymTab
> + 其他 Flash 输出段 + .data 的初始值镜像
> ```
>
> RAM 分析应区分“地址空间预算”和“某一时刻实际使用量”：
>
> ```text
> 静态占用：.data + .bss + 其他 NOLOAD/自定义静态段
> 地址预算：静态区域 + 系统堆管理范围 + MSP 预留范围 + 未分配空洞
> 运行时使用：静态占用 + 已分配堆块 + 实际 MSP 使用量
> ```
>
> 动态线程栈如果来自系统堆，已经包含在“已分配堆块”中；静态线程栈如果由全局数组提供，通常已经包含在 `.bss` 中。二者都不应重复相加。
>
> 一份构建快照的 `arm-none-eabi-size` 输出可能类似：
>
> ```text
> text    39808
> data      172
> bss      3344
> ```
>
> 这个摘要适合快速比较构建前后的变化；Map 文件适合进一步追查变化来自哪个段、哪个函数或哪个变量。`text`、`data`、`bss` 的分类口径由工具和段属性决定，自定义段、对齐空洞和 `NOLOAD` 区域可能使简单相加产生误差。
>
> 精确判断下载载荷时，应以 ELF 程序头中的可加载段为准：位于 Flash 的代码和只读数据只计一次，`.data` 的 RAM VMA 与 Flash LMA 分别计入 RAM 和 Flash，`.bss` 与 `.ccmram (NOLOAD)` 只计入对应 RAM 地址空间，`.debug_*` 不计入下载载荷。Map 主要负责解释“这些空间由哪个输入段、符号和目标文件贡献”。
>

**ELF 调试信息在 Map 中的含义**

> 启用 `-g` 后，编译器会生成 DWARF 调试信息，最终 ELF 中常见：
>
> | ELF 节 | 主要内容 | GDB 中的意义 |
> |---|---|---|
> | `.debug_info` | 函数、变量、作用域和类型 | 解释源码对象和结构体成员 |
> | `.debug_line` | 源码行号与指令地址的映射 | 源码断点、单步和故障地址定位 |
> | `.debug_abbrev`、`.debug_str` | 调试条目格式和字符串 | 辅助解析名称及类型信息 |
> | `.debug_frame` | 调用栈展开规则 | 恢复上层调用者和寄存器位置 |
> | `.symtab`、`.strtab` | 符号及其名称 | 地址符号化；严格说不属于 DWARF `.debug_*` |
>
> 这些节通常不带 `SHF_ALLOC` 属性，**不会映射到 MCU 的运行地址，也不会进入 BIN 或 HEX**。Map 中可能在 `Linker script and memory map` 区域看到地址为 `0x00000000` 的 `.debug_*` 输出节；这通常表示它是非装载信息，并不等于被丢弃。只有结合它是否位于 `Discarded input sections`、是否属于可加载段以及 ELF 节属性，才能正确解释零地址。
>
> 调试时 GDB 应加载与目标板固件来自同一次链接的 ELF：`.debug_line` 将源码断点转换成机器地址，`.debug_info` 解释变量位置和类型，符号表把 HardFault 栈帧中的 PC、LR 映射到函数名。重新构建后继续使用旧 ELF，即使源码变化不大，也可能因地址布局改变而得到错误断点、变量和调用栈。较高优化级别还会内联函数、合并代码或消除变量，因此 ELF 含有调试信息也不保证每个源码变量始终可观察。
>
> `.ARM.exidx`、`.ARM.extab` 等可能被运行库使用的异常展开信息与 `.debug_*` 不同：前者可能属于可加载运行映像并实际占用 Flash，不能仅凭“用于展开或调试”这一表面用途排除。

**`START GROUP`、`LOAD` 和运行库**

> Map 中还会出现：
>
> ```text
> LOAD .../libm.a
> START GROUP
> LOAD .../libgcc.a
> LOAD .../libc_nano.a
> END GROUP
> LOAD .../crtend.o
> LOAD .../crtn.o
> ```
>
> `LOAD` 表示链接器处理了某个目标文件或库文件。`libm.a` 提供数学函数，`libgcc.a` 提供 GCC 生成的底层辅助函数，`libc_nano.a` 是面向嵌入式系统的精简 C 库，`crtend.o` 和 `crtn.o` 属于 C 运行时辅助目标文件。
>
> `START GROUP` 与 `END GROUP` 对应静态库循环依赖的重复扫描范围。它们表示链接器可以反复检查组内库，以解决库之间相互引用的问题。出现 `LOAD libxxx.a` 并不意味着整个静态库都被复制进固件；应结合 `Archive member included` 和最终输出段判断实际进入镜像的内容。

## 与 ELF 工具交叉验证

Map 是面向人工阅读的文本报告，适合追踪来源，但其格式可能随 GNU ld 版本和链接选项变化。需要确认实际加载属性或编写自动分析工具时，应以 ELF 结构为主、Map 为辅。

在 PowerShell 中临时加入项目规定的 UCRT64 工具目录后，可对同一次构建的 ELF 执行：

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
$firmwareElf = '<实际 ELF 路径>'
& arm-none-eabi-size $firmwareElf
& arm-none-eabi-objdump -h $firmwareElf
& arm-none-eabi-readelf -S -l $firmwareElf
& arm-none-eabi-nm -n $firmwareElf
```

各工具侧重点如下：

| 工具 | 适合确认的内容 |
|---|---|
| `size` | 总体 `text/data/bss` 变化趋势 |
| `objdump -h` | ELF 各节的大小、VMA、LMA 和属性 |
| `readelf -S -l` | 节表、可加载程序段及二者映射关系 |
| `nm -n` | 按地址排序的函数、变量和链接器符号 |
| Map | 输出段规则、输入段来源、填充、丢弃段和库成员贡献 |

查询某个函数时，Map 可以说明其输入段和来源目标文件，`nm` 可以确认最终符号地址，`objdump` 可以查看实际机器指令；三者结果应与同一次构建相互吻合。

## 如何定位空间增长

当固件变大时，应先判断增长发生在 Flash 还是 RAM，然后再定位来源。

如果 Flash 增长，优先检查：

1. `.text` 是否增长，重点搜索函数大小较大的条目；
2. `.rodata` 是否增长，重点检查字符串、常量数组和查找表；
3. `FSymTab`、`.rti_fn` 是否增长，检查新增命令或自动初始化项；
4. `Archive member included` 是否新增了运行库模块；
5. `Discarded input sections` 是否因为链接脚本或 `KEEP()` 变化而减少。

如果 RAM 增长，优先检查：

1. `.bss` 中是否新增了大数组、缓冲区或设备对象；
2. `.data` 中是否新增了带初值的全局变量；
3. 线程栈大小和线程数量是否增加；
4. 堆和主栈边界之间是否仍有足够余量；
5. DMA 缓冲区是否被错误放入 CCMRAM 或不可访问的内存区域。

## 常见误读

第一，`Discarded input sections` 中的大小不是当前固件占用，因为这些段没有进入最终镜像。

第二，`LOAD libxxx.a` 不等于整个库都被链接进来，静态库通常按符号引用提取成员。

第三，`.data` 的运行地址和加载地址不同是正常现象，它需要从 Flash 复制到 RAM。

第四，`.bss` 不占用 Flash 的初始化镜像，但会真实占用 RAM。

第五，`.debug_*` 通常是 ELF 调试信息，不应直接算作 `.bin` 或 `.hex` 的烧录空间；但 `.ARM.exidx` 等运行时展开信息可能需要下载。

第六，不能脱离上下文解释 `0x00000000`：它在 `Discarded input sections` 中通常表示输入段未获最终地址，在 `.debug_*` 输出节中则可能只是非装载节的地址。

第七，输出段行中的“地址 + 大小”描述半开区间 `[start, start + size)`，结束地址本身不属于该段；还应考虑 `*fill*` 和 `ALIGN()` 产生的空隙。

第八，`_Min_Heap_Size` 是链接断言的最低要求，不一定是 RT-Thread 系统堆的实际容量；`_Min_Stack_Size` 是 MSP 地址预算，也不是每个线程的栈大小或硬件保护边界。

第九，Map 能确定静态对象、系统堆边界和栈预算，但不能给出运行时堆分配峰值、线程栈水位或最大中断嵌套深度。

## 小结

Map 文件描述的是链接器如何把多个目标文件和库组合成一个具有固定地址布局的嵌入式程序。阅读时应先区分“输入过程”和“最终布局”：`Archive member included` 说明静态库成员因符号依赖被提取，`Discarded input sections` 说明输入段被删除，`Memory Configuration` 说明可用内存，`Linker script and memory map` 则给出最终保留内容的地址和大小。

对 STM32F4 工程，最重要的分析对象是 `.isr_vector`、`.text`、`.rodata`、`.data`、`.bss`、堆栈边界以及具体函数和变量符号。只有把这些内容与链接脚本中的 `MEMORY` 和 `SECTIONS` 规则结合起来，才能正确判断固件的 Flash、RAM 使用情况和潜在的空间风险。
