# AGENTS

## 变更确认规则

除非用户在当前请求中明确授权，否则不得自动修改任何文件内容，也不得执行任何 Git 操作（包括但不限于暂存、提交、切换分支、合并、拉取、推送、重置或清理）。在准备进行此类操作前，必须说明拟执行的具体操作及其影响，并等待用户明确确认。

## 终端环境

Agent 默认在 Windows PowerShell 中执行构建、测试和其他命令。常规任务不得启动 MSYS2 Bash 或登录 Shell，以避免沙箱账户映射、HOME 初始化和临时目录权限问题。

项目使用 MSYS2 UCRT64 提供的 Windows 原生工具。执行构建前，将其 `bin` 目录加入**当前 PowerShell 进程的 PATH**：

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
```

该设置只影响当前 PowerShell 进程及其子进程，不得为此永久修改 Windows PATH。不要把 `C:\msys64\usr\bin` 加入 PATH。若任务确实依赖 Bash 或其他 MSYS2 POSIX 工具，先说明原因和影响并等待用户确认，不得自动切换环境。

## Python 虚拟环境

项目的 Python 虚拟环境位于 `.venv`，由 UCRT64 Python 创建。PowerShell 中无需激活虚拟环境，直接调用其解释器：

```powershell
& .\.venv\bin\python.exe <参数>
```

运行 pip 时使用 `& .\.venv\bin\python.exe -m pip <参数>`，不得改用系统 Python，也不得执行 Bash 风格的 `. .venv/bin/activate`。

## 构建与下载

Preset 的统一语法为 `--preset <preset-name>`。可用 BSP 及 Preset 名称不得在本文件中写成固定枚举；每次以当前 `CMakePresets.json` 为准，先通过 `cmake --list-presets=configure` 和 `cmake --list-presets=build` 获取实际可用模式。

### BSP 与构建类型选择及会话状态

- 每个新对话都独立确定一次 BSP 和构建类型。若用户已经明确指定，则在当前 Preset 列表中核对并使用；若未指定，则列出实际可用模式，并在首次构建、测试或下载前询问，不得自行选择默认值。
- 用户明确要求源码调试、断点或变量观察时，使用 Debug；明确要求发布、性能或最终固件时，使用 Release。仅要求“构建”而没有足够上下文时，仍需确认构建类型。
- BSP 与构建类型的组合一经确认，即作为本轮对话的当前构建组合。后续构建、测试和下载均沿用该组合，不重复询问。
- 只有用户明确要求切换 BSP、切换构建类型，或后续请求与当前组合明显冲突时，才重新确认。切换后，后续操作统一使用新的构建组合。

### 构建

构建使用当前 BSP 与构建类型对应的 CMake Preset，并在项目根目录通过 PowerShell 执行。配置 Preset 和构建 Preset 可能同名，也可能不同，应以 `CMakePresets.json` 中的关联关系为准：

- 配置：`cmake --preset <configure-preset>`。
- 增量构建：`cmake --build --preset <build-preset>`。
- 清理当前构建组合的已知构建产物并全量重编：先执行配置命令，再执行 `cmake --build --preset <build-preset> --clean-first`。
- 构建成功后，根据 Preset 的 `binaryDir` 和实际构建输出确认当前构建组合所需的 ELF、HEX、BIN、MAP 或其他产物，不得假定固定目录或文件名，并同时报告 BSP 与构建类型。

### 下载

- 仅在用户明确要求下载固件时执行下载操作，并始终沿用本轮对话已经确认的当前 BSP 与构建类型，使用该构建组合实际生成的固件。
- 下载前先从当前 BSP 的项目配置或文档确定下载工具、固件产物、芯片、接口和速度。若项目尚未定义这些信息，则先询问用户，不得沿用其他 BSP 的参数。
- 只有当前 BSP 明确使用 J-Link 时，才使用以下 J-Link 流程。`Scripts/download.py` 不提供 J-Link、固件、设备、接口或速度的默认值，必须由用户选择或确认，并显式传入与当前 BSP 和目标硬件匹配的全部参数；缺少任何参数时不得执行下载。
- 使用 J-Link 时，将其安装目录临时加入当前 PowerShell 进程的 PATH，并解析出已验证的 Commander 路径。以下尖括号参数必须替换为当前 BSP 的实际值：

  ```powershell
  $jlinkDirectory = 'D:\App\Code\JLink\JLink_V968a'
  $env:Path = "$jlinkDirectory;$env:Path"
  $jlinkCommander = (Get-Command JLink.exe -ErrorAction Stop).Source
  
  & .\.venv\bin\python.exe Scripts\download.py `
      --jlink $jlinkCommander `
      --firmware '<实际固件路径>' `
      --device '<J-Link 设备名>' `
      --interface '<调试接口>' `
      --speed <速度_kHz>
  ```

  该 PATH 修改只影响当前 PowerShell 进程及其子进程，不得永久修改 Windows PATH。当前下载脚本不会自动读取 `JLINK` 环境变量，因此必须保留 `--jlink` 参数。
- 若上述路径不可用，先运行 `& .\.venv\bin\python.exe Scripts\download.py --help`，再将 `$jlinkDirectory` 改为已验证的 J-Link 安装目录。
- 若下载脚本失败或输出不足以定位问题，可在用户已经授权的下载目标、固件和硬件范围内直接调用 `$jlinkCommander`，通过 J-Link Commander CLI 观察连接、擦除、写入和复位过程并纠正参数。不得借此扩大芯片、接口、速度、固件或其他硬件操作范围；需要改变这些条件时先向用户说明并确认。

## 单片机调试

调试沿用本轮对话已经确认的当前 BSP，并使用对应的 Debug 构建。若尚未确认 BSP 或构建类型，先按照“BSP 与构建类型选择及会话状态”处理；不得把其他 BSP 或构建类型的芯片、接口、速度、端口、ELF 或调试服务器参数直接套用到当前目标。

### 调试前检查与授权边界

- 先明确故障现象、复现条件及调试目标，并区分“保留运行现场”和“复位后调试启动过程”。调试服务器连接、暂停 CPU、单步和断点都会影响实时行为，执行前应向用户说明影响并取得调试授权。
- 连接正在故障状态中的设备时，默认优先保留现场。在读取关键状态前不得下载固件、复位或擦除目标；若调试服务器可能在连接时自动复位或暂停，先查阅当前版本的帮助信息并显式选择符合本次目标的连接行为。
- `reset`、`load`、擦除以及任何重新下载操作会破坏当前运行现场，只有用户明确要求或授权相应操作后才能执行。已有的一般调试授权不自动包含擦除或重新下载。
- 使用与板上固件完全匹配且包含调试符号的 Debug ELF。根据 Debug Preset 的 `binaryDir` 和实际构建输出定位文件，不得误用 Release 目录中的 ELF，也不得假定固定路径或名称。若需要修改优化级别、调试信息或其他构建配置，按“变更确认规则”另行说明并等待授权。

### 工具与连接

- 默认在两个 Windows PowerShell 会话中分别运行调试服务器和 GDB，不启动 MSYS2 Bash 或登录 Shell。需要 UCRT64 工具时，仅为相关 PowerShell 进程临时追加 `C:\msys64\ucrt64\bin`。
- 使用 `Get-Command arm-none-eabi-gdb -ErrorAction Stop` 和 `arm-none-eabi-gdb --version` 验证 GDB。使用当前 BSP 已确认的调试服务器；采用 J-Link 时，先在已验证的安装目录中解析实际存在的 GDB Server 程序并查看其帮助，不得把 J-Link Commander `JLink.exe` 当作 GDB Server，也不得假定 GDB Server 可执行文件名固定。
- 调试服务器的设备名、接口、速度和 GDB 端口必须来自当前 BSP 配置或用户确认。启动服务器后，先确认探针、目标电压、目标芯片和连接状态正常，再从 GDB 加载当前 ELF 的符号并连接对应端口。
- 保留现场模式下不执行 `load` 或复位；连接后仅在采集状态确有需要时暂停 CPU。启动过程调试可在获得授权后执行复位并暂停，再在 `Reset_Handler`、`main` 或实际相关入口设置断点。断点和观察点数量受 Cortex-M 硬件资源限制，应按定位需要最小化使用。

### 定位、验证与收尾

- 常规问题按“复现现象—设置最小断点或观察点—采集调用栈、寄存器、变量和内存—缩小触发条件”的顺序定位，避免仅凭单次停机位置下结论。
- HardFault 等异常应在复位前优先采集 PC、LR、xPSR、MSP、PSP、异常栈帧以及 CFSR、HFSR、MMFAR、BFAR；结合 `EXC_RETURN` 判断异常前使用的栈及是否存在扩展栈帧，再将故障地址映射到 ELF 的源码和反汇编。
- 调试 RTOS 问题时先确认调试服务器是否真正支持当前内核的线程感知；不支持时依据 ELF 符号检查当前线程、线程栈、调度器、中断嵌套和 IPC 对象，不得把不完整的 `info threads` 输出当作全部系统状态。
- 定位完成后先向用户报告证据链和失效条件。需要修改代码时按“变更确认规则”处理；需要重新构建、下载和复测时分别遵守本文件对应规则。
- 结束调试前确认用户期望的目标状态（继续运行、保持暂停或复位），再断开 GDB 并关闭调试服务器。响应中简要记录所用 BSP、构建类型、ELF、调试服务器参数、是否发生暂停/复位/下载、关键断点及诊断结论；失败时只保留直接相关的服务器和 GDB 输出。

## 构建日志与响应策略

构建时默认使用普通输出，不主动启用 `--verbose`、`-v` 或其他展开完整编译命令的选项，以避免产生无必要的日志和上下文消耗。

- 构建成功时，不复述逐文件编译过程，只报告构建结果、目标 BSP、构建类型、固件产物路径和尺寸信息。
- 构建失败时，优先保留并分析直接相关的错误、警告及必要上下文，省略与问题无关的正常编译进度。
- 仅当普通日志不足以定位问题时，才启用 verbose 输出；定位完成后恢复普通输出。
