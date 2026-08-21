# STM32F407ZGT6 RT-Thread Nano BSP

## 定位

本目录是面向 `STM32F407ZGT6` 的通用 RT-Thread Nano BSP。它只依赖 MCU 型号和 CubeMX 资源配置，不依赖任何开发板品牌、板载器件或原理图。

当前参考配置位于 [Tools/STM32Cubemx-F4](../../../../../Tools/STM32Cubemx-F4)：

- HSE：8 MHz；系统时钟：168 MHz；AHB/APB1/APB2：168/42/84 MHz；
- LSE：32.768 kHz，作为 RTC 时钟；
- USART1：PA9/PA10，115200、8N1；
- RTC：24 小时制。

只有上述已启用资源属于本 BSP 的运行时承诺。其他外设须先在 CubeMX 中启用，并新增相应的 RT-Thread 适配驱动。

## 入口与文件边界

BSP **不包含** `main.c`、`main.h` 或 `main()`。应用入口只允许定义在 `Applications/main.c`；RT-Thread 的 `entry()` 创建主线程后调用该入口。

`Tools/STM32Cubemx-F4` 是 CubeMX 生成文件的参考目录。`Drivers/` 和未发生 RT-Thread 集成修改的 `Core/` 文件可直接复制。以下 BSP 文件属于受控集成层，不能被 CubeMX 输出直接覆盖：

- `Core/Inc/stm32f4xx_conf.h`：最小 HAL 依赖和 `Error_Handler()` 声明，替代 CubeMX 的 `main.h`；
- `Ports/board.c`、`Ports/board.h`：时钟、堆、CubeMX 外设初始化及板级启动；
- `Core/Src/stm32f4xx_it.c`：HAL Tick 与 RT-Thread Tick 的唯一衔接点；
- `startup_stm32f407xx.s`：以 CubeMX 启动文件为基线，仅将复位后的 `bl main` 改为进入 RT-Thread `entry()`；
- `STM32F407xx_FLASH.ld`：F407ZGT6 内存布局、RT-Thread 初始化表和 FinSH 命令表。

CubeMX 重新生成后，应先比较差异：普通驱动与外设文件可复制；涉及 `stm32f4xx_conf.h`、中断、时钟、启动汇编或链接脚本的变更必须手工合并。

## 启动顺序

复位汇编完成数据段初始化后调用 RT-Thread `entry()`。内核调用 `rt_hw_board_init()`，其顺序为：HAL 初始化、168 MHz 时钟配置、RT Tick、GPIO/USART1/RTC 的 CubeMX 初始化、RT-Thread 堆初始化、`INIT_BOARD_EXPORT()` 驱动注册。

`SysTick_Handler()` 同时维护 HAL Tick 与 RT-Thread Tick。`PendSV_Handler()` 和 `HardFault_Handler()` 由 RT-Thread Cortex-M4 上下文汇编实现；CubeMX 占位实现不参与链接。

## 设备接口

- `uart1`：RT-Thread 字符设备及默认控制台。发送为阻塞式，接收采用单字节中断和 256 字节环形缓冲。`DRV_UART_CTRL_CLEAR_RX` 清空接收状态；`DRV_UART_CTRL_GET_RX_DROPS`、`DRV_UART_CTRL_GET_RX_ERRORS` 分别读取环形缓冲溢出数与 HAL 接收错误数。
- `rtc`：RT-Thread RTC 设备。`DRV_RTC_CTRL_GET_EPOCH` 和 `DRV_RTC_CTRL_SET_EPOCH` 读写 Unix epoch 秒，接受 2000–2099 年范围。首次上电或备份域未标记有效时间时，读取返回 `-RT_EEMPTY`。

`rtconfig.h` 已启用 FinSH MSH 和基础 `help`、`ps`、`free` 命令。应用层应经 RT-Thread 公开 API 使用设备，不直接引用 HAL 或 BSP 私有头文件。

## 第三方 HAL 告警策略

`CMakeLists.txt` 仅对 `Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c` 添加 `-Wno-unused-parameter`。该文件由 ST 在多个 STM32F4 型号间共用；它保留了双 Bank 器件所需的 `Banks` 参数，而本 BSP 使用的 `STM32F407xx` 是单 Bank 配置，因此该参数在三个内部函数中不会被引用。

该抑制只作用于这一份供应商 HAL 源文件，不适用于 `Ports/`、`Core/`、应用或 RT-Thread 源码；禁止将 `-Wno-unused-parameter` 提升为 BSP 或工程级选项。升级 HAL、改用双 Bank 器件，或将该文件用于其他 MCU 型号时，必须重新确认参数使用条件和告警范围。

## 构建与验证

本目录提供启动汇编和链接脚本，但当前不提供 CMake、Keil 或其他构建工程。后续构建配置必须包含 Cortex-M4 的 RT-Thread 上下文汇编、BSP 启动汇编、链接脚本、已启用的 HAL 源文件、`Ports/` 驱动和 `Applications/main.c`。

构建工程补齐后，最小运行验收为：串口出现 FinSH 提示符；`help`、`ps`、`free` 可运行；`list_device` 显示 `uart1` 与 `rtc`；UART 接收错误后控制台可继续交互；RTC 设置后复位可恢复读取。

本目录中的任何特定开发板原理图或引脚资料仅可作为外部测试参考，不构成 BSP 接口或行为约束。
