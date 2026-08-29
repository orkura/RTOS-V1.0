# STM32H743VIT6 RT-Thread Nano BSP

本 BSP 以 `F:/Project/RTOS/Tools/STM32Cubemx-H7/STM32H743` 的 CubeMX 工程为配置源，
面向 STM32H743VIT6（LQFP100）。参考工程当前使用 16 MHz HSE、LDO、VOS0、480 MHz
系统时钟，以及 PA2/PA3 上的 USART2（115200-8-N-1、内部上拉）。

## 集成边界

- CubeMX 的 `Drivers/` 整体保留，但 CMake 只编译当前配置需要的 HAL 源文件。
- `Core/Src/main.c.cubemx` 和 `Core/Inc/main.h.cubemx` 仅用于迁移对照，不参与构建。
- `syscalls.c` 和 `sysmem.c` 保留但不编译；RT-Thread 管理堆和控制台。
- 复位入口由启动文件进入 RT-Thread `entry()`，应用入口仍为项目的 `Applications/main.c`。
- `Ports/board.c` 拥有 MPU、时钟、SysTick、堆和外设初始化顺序。
- USART2 注册为 `uart2` 并作为默认 FinSH/`rt_kprintf` 控制台；SEGGER RTT 仍注册为
  `jlinkRtt`，但不主动切换控制台。
- `HardFault_Handler`、`PendSV_Handler` 属于 RT-Thread Cortex-M7 端口；USART2 IRQ 属于
  `Ports/drv_usart.c`；SysTick 同时服务 HAL 和 RT-Thread。
- 首版保持 CubeMX 行为，不启用 I-Cache/D-Cache。引入 DMA 或启用 D-Cache 前必须重新设计
  缓冲区所在内存域和 cache clean/invalidate 规则。

## 内存布局

链接脚本保留 DTCM、AXI SRAM、D2 SRAM、D3 SRAM、ITCM 和 2 MiB Flash 的物理定义。
当前 `.data`、`.bss`、RT-Thread 系统堆与 MSP 均位于 128 KiB DTCM；MSP 预留 8 KiB。
其他 SRAM 域暂不自动分配，后续使用时必须同步补充段初始化和外设可访问性约束。

## CubeMX 重新生成

不要用重新生成的 `Core/`、启动文件、链接脚本或顶层 CMake 直接覆盖本 BSP。应同步
`Drivers/` 和未修改的生成文件，并逐段合并 `main.c` 中的 MPU、时钟与 `MX_*_Init()`
变化，同时保留 RT-Thread 的入口、初始化表、堆边界和中断所有权。
