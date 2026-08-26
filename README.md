# README

本项目基于RT-Thread nano版开发，主要面向嵌入式低端产品，不进行复杂功能的开发，嵌入式中端产品由RT-Thread标准版来实现。

## 软件架构

```text
V1.0/
├─ Applications/         产品应用
├─ Components/           可复用的功能组件
└─ RT-Thread/            RTOS 内核
├─ BSP/                  底层板卡
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
├─ CMakeLists.txt        顶层构建入口；通过 TARGET_BSP 选择唯一 BSP
├─ CMakePresets.json     STM32F4、CC2538 等 BSP 的构建预设
├─ Kconfig               系统功能配置入口
├─ rtconfig.h            由 .config 生成的 RT-Thread 编译配置
└─ Scripts               存放脚本，如编译、下载
```

系统功能裁剪配置过程为：`Kconfig`（含各级子 `Kconfig`）→ `menuconfig` → `.config` → `Scripts/kconfig_to_rtconfig.py` → `rtconfig.h`。CMake 只负责选择 BSP 和组织编译；`board.h` 保存板卡固定硬件参数，并通过 `rtthread.h` 读取 `rtconfig.h` 中的宏定义。

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

底层 BSP 负责完成设备功能的注册：

```text
BSP/<boards> → Ports →  RT-Thread/include/rtthread.h(rt_device_register) → RT-Thread
```

顶层访问底层 BSP 的设备功能：

```text
RT-Thread 应用或组件 → RT-Thread/include/rtthread.h(rt_device_find/open/read/write/control) → RT-Thread/src/device.c → BSP/<boards>/Ports → Core/HAL/CMSIS
```

在工程构建中，CMake 通过 `TARGET_BSP` 选择并切换目标 BSP，并决定该 BSP 的启动文件、链接脚本、CPU 配置和驱动源码。`BSP/<boards>/Ports/board.h` 用于描述板卡固定硬件参数，并可读取顶层 `rtconfig.h` 的配置宏以控制板级驱动功能（注意 `board.h` 手动维护配置，只有 `rtconfig.h` 是由 Kconfig 进行维护）。系统通用功能（如 RT-Thread 内核、组件及应用功能）由 Kconfig 配置并生成顶层 `rtconfig.h`，供各层源码进行条件编译与裁剪。

程序启动流程：

```text
复位入口
  → RT-Thread entry()
  → rt_hw_board_init()
  → 时钟、Tick、堆、底层外设初始化
  → INIT_BOARD_EXPORT() 注册设备
  → 启动调度器和 main 线程
  → Applications/main.c
```

设备调用路径则是：

```text
Application / Component
  → rt_device_find/open/read/write/control
  → RT-Thread device 层
  → BSP/Ports 设备适配
  → HAL 或芯片厂商驱动
  → 硬件
```

## 工程环境

### 程序构建编译

本项目通过 CMake 缓存变量 `TARGET_BSP` 选择目标 BSP。配置阶段，CMake 根据该变量仅引入对应的 BSP 目录，并生成由 Ninja 执行的构建规则；该 BSP 负责提供启动文件、链接脚本、CPU 编译选项、RT-Thread CPU Port 及板级驱动。构建时，Ninja 按依赖关系调用 GNU Arm Embedded Toolchain 的 `arm-none-eabi-gcc`，完成 C、汇编源码的交叉编译和链接，并生成目标芯片所需的固件文件。因此，切换 BSP 时无需修改 `Applications/`、`Components/` 或顶层 `rtconfig.h`。

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"

# 先查看当前实际可用的配置和构建 Preset
cmake --list-presets=configure
cmake --list-presets=build

# STM32F4 Debug
cmake --preset stm32f4-debug
cmake --build --preset stm32f4-debug

# STM32F4 Release
cmake --preset stm32f4-release
cmake --build --preset stm32f4-release

# CC2538 Debug
cmake --preset cc2538-debug
cmake --build --preset cc2538-debug

# CC2538 Release
cmake --preset cc2538-release
cmake --build --preset cc2538-release
```

Debug 使用 `-Og -g3`，便于源码级调试；Release 使用 CMake 为 GNU 工具链提供的发布优化并定义 `NDEBUG`。四种组合分别写入 `build/<bsp>-debug` 和 `build/<bsp>-release`，不得让不同构建类型共用同一个构建目录。需要清理当前组合并全量重编时，在对应构建命令末尾添加 `--clean-first`。

切换不同 BSP 的核心逻辑：

```txt
if(TARGET_BSP STREQUAL "cc2538")
    add_subdirectory(CC2538)
elseif(TARGET_BSP STREQUAL "stm32f4")
    add_subdirectory(STM32F4)
else()
    message(FATAL_ERROR
        "Unsupported TARGET_BSP='${TARGET_BSP}'. Supported values: cc2538, stm32f4.")
```

### 程序功能裁剪

系统功能通过 Kconfig 进行配置和裁剪。开发者使用 `menuconfig` 修改配置项后，会生成 `.config`；随后执行 `Scripts/kconfig_to_rtconfig.py`，将已启用的配置转换为顶层 `rtconfig.h` 中的宏定义。各模块通过这些宏进行条件编译，使未启用功能的代码不参与最终固件；链接阶段再配合 `--gc-sections` 移除未被引用的函数和数据，从而减小固件体积。

裁剪流程为：

```text
Kconfig → menuconfig → .config → Scripts/kconfig_to_rtconfig.py → rtconfig.h → 条件编译与链接裁剪Kconfig 负责系统通用功能的选择，例如 RT-Thread 内核能力、组件和应用功能；TARGET_BSP 则只负责选择硬件 BSP。二者相互独立：前者决定“构建哪些功能”，后者决定“为哪块板卡构建”。
```

### GDB 调试

本项目通过 VS Code 的 Cortex-Debug 扩展配合 SEGGER J-Link 进行 STM32F4 BSP 的在线调试。调试配置位于 `.vscode/launch.json`：启动调试前会依次执行 `Configure STM32F4 Debug` 和 `Build STM32F4 Debug` 任务，生成 `build/stm32f4-debug/rtthread-stm32f4.elf`，随后启动 J-Link GDB Server，并由 `gdb-multiarch` 连接目标芯片。

在 VS Code 中选择 **Debug STM32F4 (J-Link)** 并按 `F5` 即可启动调试。程序会在 `main` 函数处暂停，之后可设置断点、单步执行、查看调用栈、寄存器和变量。

调试前应确认 J-Link 已通过 SWD 连接至 STM32F407ZG，且 `.vscode/launch.json` 中的 `serverpath`、`armToolchainPath` 和 `gdbPath` 与本机实际安装路径一致。当前配置使用 SWD 接口，调试速度为 4000 kHz。

### 工具下载

本项目在 Windows 上使用 MSYS2 的 UCRT64 环境构建。CMake、Ninja、GNU Arm 交叉编译器、Python 和 GDB 应安装在同一 UCRT64 环境中，避免混用 Windows 原生工具、MSYS 环境工具或其他 MinGW 环境工具。

#### 1. 安装 MSYS2 和构建工具链

从 [MSYS2 官网](https://www.msys2.org/) 下载并安装 MSYS2。安装完成后，在 PowerShell 中启动 UCRT64 Bash：

```powershell
$env:MSYSTEM = 'UCRT64'
C:\msys64\usr\bin\bash.exe -li
```

首次使用时，先在 UCRT64 Bash 中完整更新软件包；若更新过程要求关闭终端，重新打开 UCRT64 Bash 后再次执行同一命令，直到不再有待更新的软件包：

```bash
pacman -Suy
```

随后安装项目所需的构建工具。`gdb-multiarch` 仅在使用 GDB 调试时需要：

```bash
pacman -S --needed \
  mingw-w64-ucrt-x86_64-arm-none-eabi-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-python \
  mingw-w64-ucrt-x86_64-python-pip \
  mingw-w64-ucrt-x86_64-gdb-multiarch
```

其中，`arm-none-eabi-gcc` 负责生成 ARM 裸机程序，CMake 负责生成构建规则，Ninja 负责执行构建，Python 用于运行项目脚本和 Kconfig 配置工具。MSYS2 采用滚动更新模式，应按其[更新说明](https://www.msys2.org/docs/updating/)执行完整升级；ARM 交叉编译器的具体包信息可见 [MSYS2 软件包页](https://packages.msys2.org/packages/mingw-w64-ucrt-x86_64-arm-none-eabi-gcc)。

#### 2. 创建 Python 虚拟环境并安装 menuconfig

在项目根目录、仍处于 UCRT64 Bash 的前提下，创建并激活虚拟环境。`menuconfig` 由 Python 包 `kconfiglib` 提供；本项目当前使用版本 `14.1.0`。

```bash
python -m venv .venv
. .venv/bin/activate
python -m pip install --upgrade pip
python -m pip install "kconfiglib==14.1.0"
```

安装完成后，可通过以下命令确认 `menuconfig` 可用：

```bash
menuconfig --help
```

执行 `menuconfig Kconfig` 可修改功能配置，保存后会更新 `.config`。随后运行下列脚本生成供 C/C++ 源码使用的 `rtconfig.h`：

```bash
menuconfig Kconfig
python Scripts/kconfig_to_rtconfig.py
```

完成配置后可执行 `deactivate` 退出虚拟环境。后续运行 `Scripts/build.py`、`Scripts/download.py` 或 Kconfig 工具前，均应先重新激活 `.venv`。`kconfiglib` 的安装包和版本信息见 [PyPI](https://pypi.org/project/kconfiglib/)。

#### 3. 安装调试工具

需要下载固件或进行在线调试时，安装 [SEGGER J-Link Software and Documentation Pack](https://www.segger.com/downloads/jlink/)。该软件包提供项目所用的 `JLink.exe` 和 `JLinkGDBServerCL.exe`。安装后，根据实际安装目录更新 `.vscode/launch.json` 中的 `serverpath`，并在执行 `Scripts/download.py` 时通过 `--jlink` 显式指定 `JLink.exe` 路径。

如使用 VS Code 调试，还应安装 [Visual Studio Code](https://code.visualstudio.com/) 以及以下扩展：

- [Cortex-Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)：通过 J-Link GDB Server 连接目标芯片。
- [CMake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)：读取本项目的 CMake Preset 并提供编辑器配置。

#### 4. 验证安装

在 UCRT64 Bash 中执行以下命令。除 J-Link 外，所有命令均应能显示版本或帮助信息：

```bash
cmake --version
ninja --version
arm-none-eabi-gcc --version
python --version
gdb-multiarch --version
menuconfig --help
```
