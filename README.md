# README

## 软件架构

```text
V1.0/
├─ CMakeLists.txt        顶层构建入口；通过 TARGET_BSP 选择唯一 BSP
├─ CMakePresets.json     STM32F4、CC2538 等 BSP 的构建预设
├─ Kconfig               系统功能配置入口
├─ rtconfig.h            由 .config 生成的 RT-Thread 编译配置
├─ Scripts               存放脚本，如编译、下载
├─ Applications/         产品应用、任务编排与启动流程
├─ Components/           可复用的功能组件与第三方组件集成
├─ BSP/                  板卡构建与硬件适配
│  ├─ CMakeLists.txt     TARGET_BSP 注册与唯一 BSP 选择
│  ├─ STM32F4/           STM32F407ZGT6 BSP
│  │  ├─ Core/           CubeMX/板级基础代码
│  │  ├─ Drivers/        CMSIS、HAL 等厂商库
│  │  └─ Ports/          board.h、RT-Thread 适配及设备驱动注册
│  ├─ CC2538/            CC2538 BSP
│  │  ├─ Core/           板级基础代码
│  │  ├─ Drivers/        芯片厂商驱动库
│  │  └─ Ports/          board.h、RT-Thread 适配及设备驱动注册
│  └─ CC2652/            预留 BSP（尚未接入构建）
└─ RT-Thread/            RTOS 内核及其功能配置树
   ├─ Kconfig            RT-Thread 配置入口
   ├─ src/Kconfig        内核、IPC、内存和软件定时器配置
   └─ components/
      ├─ Kconfig         控制台与组件配置入口
      └─ finsh/Kconfig   FinSH/MSH 配置
```

功能配置链路为：`Kconfig`（含各级子 `Kconfig`）→ `menuconfig` → `.config` → `Scripts/kconfig_to_rtconfig.py` → `rtconfig.h`。CMake 只负责选择 BSP 和组织编译；`board.h` 保存板卡固定硬件参数，并可读取 `rtconfig.h` 中的功能开关。

底板通过 `BSP/<boards>/Ports` 适配 RT-Thread，并借助 RT-Thread 的设备层（device）将底层硬件能力接入系统（如uart、spi、sensors），由 RT-Thread 统一管理。Applications 与 Components 仅通过 RT-Thread 提供的 API 调用这些能力，从而与具体 BSP 实现解耦。

```txt
Linux 应用
  ↓ open/read/write/ioctl（系统调用）
Linux 的 VFS / TTY / 网络 / 块设备等子系统
  ↓
Linux 具体驱动 + 总线设备模型
  ↓
硬件

RT-Thread 应用或组件
  ↓ rt_device_find/open/read/write/control
RT-Thread 设备框架
  ↓
BSP/Ports 中的具体驱动适配
  ↓
厂商 HAL/CMSIS
  ↓
硬件
```

```text
BSP/<boards> → Ports →  RT-Thread/include/rtthread.h(rt_device_register) → RT-Thread
```

```text
RT-Thread 应用或组件 → RT-Thread/include/rtthread.h(rt_device_find/open/read/write/control) → RT-Thread/src/device.c → BSP/<boards>/Ports
```

在工程构建中，CMake 通过 `TARGET_BSP` 选择并切换目标 BSP，并决定该 BSP 的启动文件、链接脚本、CPU 配置和驱动源码。`BSP/<boards>/Ports/board.h` 用于描述板卡固定硬件参数，并可读取顶层 `rtconfig.h` 的配置宏以控制板级驱动功能。系统通用功能（如 RT-Thread 内核、组件及应用功能）由 Kconfig 配置并生成顶层 `rtconfig.h`，供各层源码进行条件编译与裁剪。

## 基于 CMake 的 BSP 选择与配置

本项目通过 CMake 缓存变量 `TARGET_BSP` 选择目标 BSP。配置阶段，`BSP/CMakeLists.txt` 仅引入对应的 BSP 目录；该 BSP 随后负责提供启动文件、链接脚本、CPU 编译选项、RT-Thread CPU Port 及板级驱动。因此，切换 BSP 时无需修改 `Applications/`、`Components/` 或顶层 `rtconfig.h`。

推荐使用 CMake Preset 进行切换：

```powershell
# 构建 STM32F4 BSP
cmake --preset stm32f4
cmake --build --preset stm32f4
# 清理当前 BSP 的构建产物并重新构建，同时逐行显示简洁的编译进度
cmake --build --preset stm32f4 --clean-first 2>&1 | cat

# 构建 CC2538 BSP
cmake --preset cc2538
cmake --build --preset cc2538
# 清理当前 BSP 的构建产物并重新构建，同时逐行显示简洁的编译进度
cmake --build --preset cc2538 --clean-first 2>&1 | cat
```

## 构建、下载与调试资源依赖

### 支持范围

当前 CMake 工程可构建 `stm32f4`（STM32F407ZGT6）和 `cc2538`（CC2538F512）两个 BSP；构建会在 `build/<BSP>/` 下生成同名的 `.elf`、`.hex`、`.bin` 与 `.map` 文件。

VS Code 的 `Download STM32F4` 任务通过 J-Link 以 SWD、4 MHz 将已生成的 `rtthread-stm32f4.hex` 写入 STM32F407。CC2538 目前只有构建与串口验证支持；请按所用开发板及下载器的文档选择并配置其烧录、调试工具。

### 构建环境（必需）

- Windows 与 PowerShell；
- [CMake 3.20 或更高版本](https://cmake.org/download/)；顶层工程使用 CMake Preset v2；
- [Ninja](https://github.com/ninja-build/ninja/releases)；
- GNU Arm Embedded Toolchain，且必须提供 `arm-none-eabi-gcc`、`arm-none-eabi-objcopy` 和 `arm-none-eabi-size`。

默认 Preset 固定使用 `C:/msys64/ucrt64/bin` 下的工具。建议安装 [MSYS2 UCRT64](https://www.msys2.org/docs/environments/)，并在 UCRT64 终端中完成完整系统更新后安装下列包（包管理说明见 [MSYS2 文档](https://www.msys2.org/docs/package-management/)）：

```bash
pacman -Suy
pacman -S --needed \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-arm-none-eabi-gcc
```

安装完成后，确认以下文件可用：

```text
C:\msys64\ucrt64\bin\cmake.exe
C:\msys64\ucrt64\bin\ninja.exe
C:\msys64\ucrt64\bin\arm-none-eabi-gcc.exe
```

从 PowerShell 使用 Preset 时，`cmake.exe` 还必须位于 `PATH` 中。若工具未安装在上述默认位置，可改用构建脚本并显式传入路径：

```powershell
.\Scripts\build.ps1 -TargetBsp stm32f4 `
  -ArmToolchainRoot 'D:\tools\msys64\ucrt64' `
  -NinjaExe 'D:\tools\msys64\ucrt64\bin\ninja.exe'
```

### STM32F4 下载与串口验证

下载 STM32F4 BSP 还需要：

- 原装或获授权的 SEGGER J-Link/J-Trace 调试器；
- [SEGGER J-Link Software and Documentation Pack](https://www.segger.com/downloads/jlink/)，其中的 `JLink.exe` 用于下载任务；
- 目标板、USB 线，以及 J-Link 到目标板的 SWD 连接：`VTref`、`GND`、`PA13/SWDIO`、`PA14/SWCLK`；`NRST` 可选。普中 F407 定通 T200 板的调试口引脚详见 [硬件连接关系](BSP/STM32F4/Docs/普中F407-T200_硬件引脚连接关系.md)。

先生成 `build/stm32f4/rtthread-stm32f4.hex`，然后在 VS Code 中运行 `Tasks: Run Task` → `Download STM32F4`。该任务只下载，不会构建。J-Link 的路径在 `.vscode/tasks.json` 中配置。

该 BSP 的 FinSH 控制台使用 USART1：`PA9`（TX）、`PA10`（RX），115200、8N1。T200 板可经板载 CH340C/USB 使用串口终端；其他板卡须使用 3.3 V USB 转串口模块并共地。下载启动后，查看 `RT-Thread BSP ready.`，再执行 `help`、`ps`、`free` 与 `list_device`；预期设备为 `uart1`、`rtc`。

### STM32F4 源码级调试（可选）

除上述下载依赖外，源码级调试需要：

- J-Link Software Pack 中的 `JLinkGDBServerCL.exe`（该软件包同时包含 GDB Server 与 Commander）；
- GDB 客户端。MSYS2 UCRT64 可安装 [gdb-multiarch 包](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-gdb-multiarch)：

  ```bash
  pacman -S --needed mingw-w64-ucrt-x86_64-gdb-multiarch
  ```

先生成带调试信息的 ELF：

```powershell
cmake --preset stm32f4 -DCMAKE_BUILD_TYPE=Debug
cmake --build --preset stm32f4
```

在一个终端启动 GDB Server：

```powershell
& 'C:\Program Files\SEGGER\JLink\JLinkGDBServerCL.exe' `
  -device STM32F407ZG -if SWD -speed 4000 -port 2331
```

在另一个终端连接 GDB。安装位置不同则相应替换两个可执行文件路径：

```powershell
& 'C:\msys64\ucrt64\bin\gdb-multiarch.exe' .\build\stm32f4\rtthread-stm32f4.elf
```

```gdb
target extended-remote :2331
monitor reset
monitor halt
break main
continue
```

`main()` 是当前应用入口；如需重新写入固件，可先运行下载脚本，或在已连接的 GDB 会话中使用 `load`。本仓库尚未提供 VS Code、Ozone、OpenOCD 或 CC2538 专用的调试启动配置。
