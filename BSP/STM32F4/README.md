# STM32F407ZGT6 RT-Thread Nano BSP

## 使用须知

本目录是面向 `STM32F407ZGT6` 的通用 RT-Thread Nano BSP。它只依赖 MCU 型号和 CubeMX 资源配置，不依赖任何开发板品牌、板载器件或原理图。

注意：

- [普中F407-定通-T200开发板原理图](./Docs/普中F407-定通-T200开发板原理图.pdf) 和 [普中F407-T200_硬件引脚连接关系.md](./Docs/普中F407-T200_硬件引脚连接关系.md) 文档只是临时测试参考使用，根据实际情况调整。
- CubeMX 用于配置裸机环境下的初始化，提供一定的参考，然后通过手动或者Agent来将这部分内容导入到本工程项目，这样可以最大化保证程序可运行。

## CubeMX 移植到 RT-Thread Nano

本文以 `F:/Project/RTOS/Tools/STM32Cubemx-F4/STM32F407` 的当前快照为对照，记录 CubeMX 裸机工程接入本 BSP 时实际发生的改动。CubeMX 工程只作为芯片、时钟、引脚和外设初始化的配置源，不作为本项目的应用程序或构建入口。

BSP **不包含参与构建的** CubeMX `Core/Src/main.c`、`Core/Inc/main.h` 或 CubeMX 顶层 CMake 文件，也不复制 `build/`、`cmake/`、`Scripts/` 等生成或工具目录。原始入口文件分别以 `Core/Src/main.c.cubemx` 和 `Core/Inc/main.h.cubemx` 保存，仅供移植对照。应用入口只允许定义在项目的 `Applications/main.c`；复位后由启动文件进入 RT-Thread 的 `entry()`，RT-Thread 创建主线程后再调用应用 `main()`。

> [!IMPORTANT]
> 不要用新生成的 `Core/` 整体覆盖 BSP，也不要覆盖 `startup_stm32f407xx.s`、`STM32F407xx_FLASH.ld`、`Ports/` 或 BSP 的 `CMakeLists.txt`。这些文件包含 RT-Thread 的启动、中断、内存和设备接入逻辑。CubeMX 重新生成后，应按下面的文件分类逐项复制或合并。

### 参考副本与差异标记

`main.h.cubemx` 和 `main.c.cubemx` 是当前 CubeMX 入口文件的参考副本，文件顶部的说明之外应与 CubeMX 原文件保持一致。它们不会响应 `#include "main.h"`，也没有加入 CMake 源文件列表。后续重新生成 CubeMX 工程时，应同步更新这两个副本，同时保留顶部的用途说明。

对 CubeMX 生成文件进行 RT-Thread 适配时，优先保留原代码以便比较：

- 被替换的单行代码以 `CubeMX original` 注释保留，旁边放置实际生效的 BSP 代码和所有者说明。
- 内部已包含块注释的完整函数不能再用外层 C 注释包裹，因此使用带原因说明的 `#if 0` 保留；这些代码不参与编译。
- 链接脚本属于整体重构，不在 BSP 中嵌入一份被禁用的 CubeMX 链接脚本，仍通过参考工程和本文对照。
- 注释和 `#if 0` 只是移植记录，实际行为始终以未被禁用的代码为准。

### 文件分类

当前两个目录的内容级比对结果如下。这里的“原样复制”仅表示当前快照内容一致；CubeMX 版本或外设配置发生变化后仍需重新比较。

注意：活动代码中 `main.h` 的作用由 `stm32f4xx_conf.h` 替代；`main.h.cubemx` 只是不可包含的参考副本。

| 分类 | 文件 | 处理方式 |
| --- | --- | --- |
| 原样复制 | `Drivers/` 全部内容 | 当前 BSP 中的 1442 个文件与 CubeMX 参考工程一致，可以整体同步。BSP 的 CMake 只编译实际使用的 HAL 源文件，不会编译整个目录。 |
| 原样复制 | `Core/Inc/stm32f4xx_hal_conf.h` | 保留 CubeMX 生成的 HAL 模块配置。 |
| 原样复制 | `Core/Src/gpio.c`、`rtc.c`、`system_stm32f4xx.c` | 当前内容与 CubeMX 生成文件一致，可以按文件同步。 |
| 保留但不编译 | `Core/Src/syscalls.c`、`sysmem.c` | 文件当前与 CubeMX 工程一致，但未加入 BSP 的 CMake 源文件列表；RT-Thread 的堆和控制台不依赖这两个裸机桩文件。 |
| 参考副本 | `Core/Inc/main.h.cubemx`、`Core/Src/main.c.cubemx` | 保存 CubeMX 原始入口文件，不包含、不编译；用于核对时钟、外设初始化和公共声明的迁移。 |
| 复制后修改 | `Core/Inc/gpio.h`、`rtc.h`、`usart.h` | 注释保留原始 `main.h` 包含，并改用 `stm32f4xx_conf.h`。 |
| 复制后修改 | `Core/Inc/stm32f4xx_it.h` | 注释保留由 RT-Thread 或 BSP 驱动接管的 `HardFault_Handler`、`PendSV_Handler` 和 `USART1_IRQHandler` 原始声明。 |
| 复制后修改 | `Core/Src/stm32f4xx_it.c` | 改用 `stm32f4xx_conf.h`，接入 RT-Thread tick，并用 `#if 0` 保留已由 RT-Thread CPU 端口或 UART 驱动接管的 CubeMX 中断函数。 |
| 复制后修改 | `Core/Src/stm32f4xx_hal_msp.c` | 注释保留原始 `main.h` 包含，并改用 `stm32f4xx_conf.h`。其余 MSP 初始化仍由 CubeMX 管理。 |
| 复制后修改 | `Core/Src/usart.c` | 保留 UART 参数及 GPIO 配置，将 CubeMX 的 USART1 NVIC 语句保留为注释，实际优先级和使能改由 RT-Thread UART 驱动管理。 |
| 复制后修改 | `startup_stm32f407xx.s` | 保留 CubeMX 启动和向量表，注释保留原始 `bl main`，实际调用目标改为 RT-Thread `entry`。 |
| 重新整理 | `STM32F407xx_FLASH.ld` | 以 MCU 内存布局为基础，增加 RT-Thread 初始化表、FinSH 符号表、系统堆边界和 MSP 预留区等链接约束。 |
| BSP 新增 | `Core/Inc/stm32f4xx_conf.h` | 替代 `main.h` 作为 HAL 集成边界，提供 `stm32f4xx_hal.h` 和 `Error_Handler()` 声明。 |
| BSP 新增 | `Ports/board.c`、`board.h` | 实现 RT-Thread 板级初始化、时钟、SysTick、系统堆边界和公共 BSP 配置。 |
| BSP 新增 | `Ports/drv_usart.c/.h`、`drv_rtc.c/.h` | 将 CubeMX 初始化后的 UART1、RTC HAL 句柄注册为 RT-Thread 设备。 |
| BSP 新增 | `Ports/rtthread_cpu_port.cmake`、`CMakeLists.txt` | 选择 Cortex-M4F CPU/FPU 参数、RT-Thread CPU 端口、启动文件、HAL/Core/Ports 源文件及链接脚本。 |

### 重点关注

移植或重新生成 CubeMX 代码时，应优先检查以下集成边界。它们不是普通的代码差异，处理错误通常会直接造成链接失败、系统无法启动、调度异常或外设不可用。

1. **启动入口不能改回 `main`**：`startup_stm32f407xx.s` 必须调用 RT-Thread 的 `entry()`。应用 `main()` 由 RT-Thread 主线程调用，不能直接从复位入口运行。
2. **不要启用 CubeMX 的 `main.c/main.h`**：两个原文件只以 `.cubemx` 参考副本保留；从 `main.c.cubemx` 提取时钟配置和 `MX_*_Init()` 调用变化。生成文件原来通过 `main.h` 获得的 HAL 声明，应改由 `stm32f4xx_conf.h` 提供。
3. **时钟和外设初始化必须同步到 `board.c`**：CubeMX 中 `SystemClock_Config()`、新增的 `MX_*_Init()` 或初始化顺序发生变化后，需要同步到 `bsp_clock_config()` 和 `rt_hw_board_init()`，否则 `.ioc` 与实际 BSP 行为会不一致。
4. **每个中断只能有一个有效实现**：`HardFault_Handler`、`PendSV_Handler` 属于 RT-Thread CPU 端口；接入设备框架的外设中断属于对应 BSP 驱动。CubeMX 的重复定义可以在 `#if 0` 中保留供比较，但不能参与编译。
5. **SysTick 同时服务 HAL 和 RT-Thread**：`SysTick_Handler()` 既要调用 `HAL_IncTick()`，也要调用 `bsp_systick_handler()`。修改 `RT_TICK_PER_SECOND` 时必须检查 HAL 超时单位是否仍然正确。
6. **外设初始化与设备驱动职责不同**：CubeMX 文件负责寄存器、GPIO 和 HAL 句柄初始化；`Ports/drv_*.c` 负责中断封装、缓冲、RT-Thread 设备注册和控制接口。不要把驱动层逻辑重新塞回 CubeMX 生成文件。
7. **链接脚本不能由 CubeMX 版本覆盖**：`.rti_fn`、`FSymTab`、`__heap_start__`、`__heap_end__` 和 MSP 预留区是 RT-Thread 启动及组件发现所必需的；CCMRAM 的 `NOLOAD` 策略也必须与启动代码保持一致。
8. **复制文件后还要更新构建清单**：文件存在于 BSP 中不代表会被编译。新增外设时必须同步检查 HAL/Core/Ports 源文件、头文件目录、编译宏及 CPU/FPU 参数，并避免把 `syscalls.c`、`sysmem.c` 等裸机桩误加入构建。

### 关键函数和所有权变化

CubeMX 的 `main()` 不能直接移植，因为 RT-Thread 必须先完成内核和主线程启动。原 `main()` 中与硬件相关的初始化按以下关系迁入 `Ports/board.c`：

| CubeMX 裸机工程 | RT-Thread BSP | 说明 |
| --- | --- | --- |
| `HAL_Init()` | `rt_hw_board_init()` | 初始化 HAL；随后重新配置符合 `RT_TICK_PER_SECOND` 的 SysTick。 |
| `SystemClock_Config()` | `bsp_clock_config()` | 保留 CubeMX 生成的 HSE、LSE、PLL、AHB 和 APB 配置；函数改为 BSP 私有实现。 |
| `MX_GPIO_Init()` | `rt_hw_board_init()` | 继续调用 CubeMX 生成函数。 |
| `MX_USART1_UART_Init()` | `rt_hw_board_init()` | 先完成 HAL 外设初始化，再由 `drv_usart.c` 注册 RT-Thread 字符设备。 |
| `MX_RTC_Init()` | `rt_hw_board_init()` | 先完成 HAL 外设初始化，再由 `drv_rtc.c` 注册 RT-Thread RTC 设备。 |
| `Error_Handler()` | `Ports/board.c` | BSP 提供统一实现，声明位于 `stm32f4xx_conf.h`。 |

中断处理必须只有一个所有者，否则会产生重复符号或绕过 RT-Thread 的中断记账：

- `HardFault_Handler` 和 `PendSV_Handler` 由 `RT-Thread/libcpu/arm/cortex-m4/context_gcc.S` 实现；CubeMX 的原函数在 `stm32f4xx_it.c` 中以 `#if 0` 保留，原声明在 `stm32f4xx_it.h` 中以注释保留。`PendSV_Handler` 负责线程上下文切换，`HardFault_Handler` 负责采集 RT-Thread 异常现场。
- `SysTick_Handler()` 仍保留 `HAL_IncTick()`，并调用 `bsp_systick_handler()`；后者以 `rt_interrupt_enter()`、`rt_tick_increase()`、`rt_interrupt_leave()` 更新内核时基。当前 `RT_TICK_PER_SECOND` 为 1000，与 HAL 的毫秒 tick 一致；若修改该配置，需要同时检查 HAL 超时语义。
- `USART1_IRQHandler()` 移到 `drv_usart.c`，在 `HAL_UART_IRQHandler()` 外包裹 RT-Thread 中断进入和退出通知。USART1 NVIC 优先级及使能也由设备初始化函数统一设置，不再由 `usart.c` 管理。

链接脚本不能继续使用 CubeMX 原文件直接覆盖。BSP 链接脚本除保留 STM32F407ZGT6 的 FLASH、主 SRAM 和 CCMRAM 布局外，还完成以下工作：

- 保存 `.rti_fn` 初始化表及其边界符号，支持 `INIT_*_EXPORT()` 自动初始化。
- 保存 `FSymTab` 及其边界符号，支持 FinSH/MSH 命令发现。
- 以 `.bss` 末尾对齐地址作为 `__heap_start__`，以 MSP 预留区下界作为 `__heap_end__`，供 `rt_hw_board_init()` 初始化 RT-Thread 系统堆。
- 将 MSP 预留量设为 `0x2000`，并在链接期检查静态数据、最小堆和 MSP 空间是否冲突。
- 将 `.ccmram` 定义为 `NOLOAD`。当前启动代码不会复制或清零该区域，使用者必须自行初始化，而且 CCMRAM 不能用于 DMA 缓冲区。

### CubeMX 重新生成后的迁移步骤

1. 在参考工程中更新 `.ioc` 并生成代码，先核对 MCU 型号、时钟树、引脚、外设实例和 NVIC 配置；不要直接把生成结果输出到 BSP。生成完成后同步更新 `main.h.cubemx` 和 `main.c.cubemx`，并保留其顶部用途说明。
2. 比较 `Drivers/` 版本和内容。需要升级时整体同步，再确认许可证文件、CMSIS 设备头文件和 HAL 版本保持配套。
3. 按“原样复制”名单同步未修改的 `Core` 文件。新增外设的 `.c/.h` 文件先作为候选文件加入，不要假定它们不需要 RT-Thread 适配。
4. 对“复制后修改”的文件进行三方或逐段合并，保留 `stm32f4xx_conf.h`、RT-Thread 中断入口以及驱动拥有的 NVIC 配置，禁止直接覆盖。
5. 从新生成的 `main.c` 提取 `SystemClock_Config()` 和 `MX_*_Init()` 调用变化，同步到 `bsp_clock_config()` 与 `rt_hw_board_init()`；不要移植 CubeMX 的 `main()` 和无限循环。
6. 逐个确认新增或变化的 IRQ 由谁实现。调度相关异常归 RT-Thread CPU 端口，接入 RT-Thread 设备框架的外设 IRQ 归对应驱动，其余 IRQ 才保留在 `stm32f4xx_it.c`。
7. 更新 BSP `CMakeLists.txt`：只加入实际需要的 Core、HAL 和 Ports 源文件，并保持 `STM32F407xx`、`USE_HAL_DRIVER`、CPU/FPU 参数、启动文件和链接脚本仍由 BSP 统一发布。
8. 根据当前 `CMakePresets.json` 查询并选择 STM32F4 对应的配置和构建 Preset，完成构建后检查 ELF、HEX、BIN 和 MAP；上板后至少验证系统时钟、RT-Thread tick、主线程、FinSH 控制台、UART 收发以及本次新增的外设。

后续若 CubeMX 增删外设或升级固件包，应重新执行内容比对。本文的文件名单描述当前快照，真正需要长期保持的是启动入口、时基、中断所有权、内存布局和设备注册这些集成边界。
