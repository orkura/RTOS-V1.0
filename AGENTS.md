# AGENTS

## 变更确认规则

- 除非用户在当前请求中明确授权，否则不得修改任何文件，也不得执行任何 Git 操作，包括暂存、提交、切换分支、合并、拉取、推送、重置和清理等。
- 在创建、修改或删除文件前，必须列出拟操作文件的名称和完整路径。修改内容较短时，直接给出拟修改内容；内容较长时，简要说明修改范围、主要内容及影响。
- 说明拟执行的操作及其影响后，必须等待用户明确确认，方可执行。

## 终端环境

本项目统一使用 PowerShell 作为命令行环境，包括 Windows PowerShell（`powershell.exe`）和 PowerShell 7+（`pwsh.exe`），并在其中使用 MSYS2 UCRT64 提供的工具。不得使用 MSYS2 Bash 或登录 Shell。

每次新建 PowerShell 进程或会话后，必须确保 `C:\msys64\ucrt64\bin` 位于当前进程 `PATH` 的最前面。若终端配置尚未提供该环境，应在同一会话或同一次命令调用中执行：

```powershell
$env:Path = "C:\msys64\ucrt64\bin;$env:Path"
```

不得假定其他终端或先前进程中的环境修改仍然有效。该设置只能影响当前 PowerShell 进程及其子进程，不得永久修改 Windows 的系统或用户 `PATH`，也不得将 `C:\msys64\usr\bin` 加入 `PATH`。

确需 Bash 或其他 MSYS2 POSIX 工具时，必须先说明原因和影响，并等待用户明确确认。

## 虚拟环境

本项目的 Python 虚拟环境位于 `.venv`，由 UCRT64 Python 创建。PowerShell 中无需激活虚拟环境，直接调用其解释器：

```powershell
& .\.venv\bin\python.exe <参数>
```

运行 pip 时使用 `& .\.venv\bin\python.exe -m pip <参数>`，不得改用系统 Python，也不得执行 Bash 风格的 `. .venv/bin/activate`。

## 构建与下载

### 预设信息查询

Preset 统一使用 `--preset <preset-name>` 语法。每轮对话首次执行配置、构建、测试或下载前，必须在项目根目录运行以下命令，获取当前实际可用的 Preset：

```powershell
cmake --list-presets=configure
cmake --list-presets=build
```

后续操作应以本次查询结果及项目根目录下的 `CMakePresets.json` 为准。

预设配置的名字一般为`<BSP>-Debug/Release`，BSP选择不同的地板，Debug 和 Release 为构建类型。

### 配置

每个新对话都独立确定一次 BSP 和构建类型。若用户已经明确指定，则在当前 Preset 列表中核对并使用；若未指定，则列出实际可用模式，并在首次构建、测试或下载前询问，不得自行选择默认值。

用户明确要求源码调试、断点或变量观察时，构建类型选择 Debug；明确要求发布、性能或最终固件时，构建类型选择使用 Release。一般情况下，默认使用 Debug。

### 构建

构建使用当前 BSP 与构建类型对应的 CMake Preset，并在项目根目录通过 PowerShell 执行。配置 Preset 和构建 Preset 可能同名，也可能不同，应以 `CMakePresets.json` 中的关联关系为准：

- 配置：`cmake --preset <configure-preset>`。配置成功后，根据该 configure Preset 的实际 `binaryDir` 定位其 `compile_commands.json`；若文件已生成，必须执行 `cmake -E copy_if_different "<binaryDir>/compile_commands.json" "<sourceDir>/build/compile_commands.json"`，将其同步到 VS Code 使用的固定路径。不得依赖 VS Code CMake Tools 的 `cmake.copyCompileCommands` 自动复制，也不得假定 configure Preset 名称与目录名固定；切换构建组合并重新配置后同样执行此同步。
- 增量构建：`cmake --build --preset <build-preset>`。
- 清理当前构建组合的已知构建产物并全量重编：先执行配置命令，再执行 `cmake --build --preset <build-preset> --clean-first`。
- 构建成功后，根据 Preset 的 `binaryDir` 和实际构建输出确认当前构建组合所需的 ELF、HEX、BIN、MAP 或其他产物，不得假定固定目录或文件名，并同时报告 BSP 与构建类型。

### 下载

- 仅在用户明确要求时下载固件，并沿用本轮已确认的 BSP、构建类型及其实际产物。

- 下载前从当前 BSP 的配置或文档确认下载工具、固件、芯片、接口和速度；信息缺失时先询问用户，不得借用其他 BSP 的参数。

- 当前 BSP 明确使用 J-Link 时，临时将其安装目录加入本次 PowerShell 进程的 `PATH`，并向 `Scripts/download.py` 显式传入全部参数：

  ```powershell
  $jlinkDirectory = 'D:\App\Code\JLink\JLink_V968a'
  $env:Path = "$jlinkDirectory;$env:Path"
  $jlinkCommander = (Get-Command JLink.exe -ErrorAction Stop).Source
  
  & .\.venv\bin\python.exe Scripts\download.py 
      --jlink $jlinkCommander 
      --firmware '<实际固件路径>' 
      --device '<J-Link 设备名>' 
      --interface '<SWD 或 JTAG，一般使用 SWD>' 
      --speed <速度_kHz，一般使用 4000>
  ```
  
- 本项目可用的设备名如下：

  - `STM32F407ZG`
  - `CC2538SF53`

- 脚本失败或信息不足时，可在已授权的参数范围内直接使用 J-Link Commander 排查；改变固件或硬件参数前须重新确认。
