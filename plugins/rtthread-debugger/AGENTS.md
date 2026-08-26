# RT-Thread Debugger

`rtthread-debugger` 是一个独立的 Codex Plugin 工程。它通过 Skill 理解和规划调试任务，通过 MCP Server 管理实际的 GDB、调试探针和串口会话，为 RT-Thread MCU 项目提供统一的停机调试与在线调试能力。

当前 RTOS 工程只是本插件的首个开发和硬件验证环境。插件不属于 MCU 固件，不加入固件的 CMake 构建树，也不复制或修改 RT-Thread 内核源码。

## 设计目标

本项目统一的是调试入口、决策逻辑、授权边界、会话状态和诊断结果，而不是把不同调试通道强行合并成同一种协议。

计划支持三种基础能力：

1. GDB 停机调试：通过 J-Link、SWD 和 GDB 完成附加、暂停、断点、单步、寄存器、内存、调用栈及 HardFault 分析。
2. FinSH 运行观测：在 MCU 正常运行时接收 MCU 到 PC 的日志、事件和状态数据。
3. FinSH 命令诊断：由 PC 向 MCU 发送命令，再接收 MCU 返回的执行结果。

组合调试在上述能力之上协调 GDB 与串口状态。例如 GDB 暂停 MCU 后，串口中断和 FinSH 线程也会停止；此时串口超时应解释为目标暂停产生的预期现象，而不能直接判断为通信故障。

本项目不提供任意 Shell 执行接口，不把芯片、BSP、ELF、串口号、探针路径或下载参数写死在插件中，也不把普通调试授权扩大为复位、擦除或下载授权。

## 总体架构

```text
Codex
  |
  +-- Skill：理解意图、选择模式、规划步骤、控制授权边界
  |
  `-- MCP Server：执行结构化操作并维护长连接会话
       |
       +-- GDB / GDB Server / J-Link / SWD
       +-- Serial Monitor
       `-- FinSH Command Channel
              |
              `-- 当前工作区、固件 ELF 与目标 MCU
```

Skill 是决策层。它根据用户目标选择 GDB、运行观测、命令诊断或组合流程，读取目标工程的约束，并在需要改变 MCU 状态前取得相应授权。

MCP Server 是执行层。它负责启动和回收外部进程、管理 GDB 与串口连接、保存会话状态、采集输出并返回结构化结果。Skill 不直接拼接 PowerShell、GDB 或串口命令。

目标工程是被调试对象。MCP 通过 `workspace_root` 访问其 `AGENTS.md`、CMake Preset、构建产物和硬件配置，但插件源码与目标工程保持边界。

## 计划目录

```text
rtthread-debugger/
|-- .codex-plugin/
|   `-- plugin.json
|-- .mcp.json
|-- README.md
|-- skills/
|   `-- mcu-debug/
|       |-- SKILL.md
|       |-- agents/
|       |   `-- openai.yaml
|       `-- references/
|           |-- gdb-debugging.md
|           |-- finsh-runtime.md
|           |-- finsh-command.md
|           `-- combined-debugging.md
|-- mcp-server/
|   |-- pyproject.toml
|   |-- src/
|   |   `-- rtthread_debugger_mcp/
|   |       |-- __init__.py
|   |       |-- __main__.py
|   |       |-- server.py
|   |       |-- tools/
|   |       |-- core/
|   |       |-- backends/
|   |       `-- protocols/
|   `-- tests/
|       |-- unit/
|       |-- integration/
|       `-- hardware/
`-- docs/
    |-- architecture.md
    |-- mcp-tools.md
    `-- hardware-testing.md
```

`.codex-plugin/plugin.json` 是 Plugin 清单，后续声明 `skills` 和 `mcpServers`。`.mcp.json` 注册 MCP Server 的启动入口。这两个文件不存在时，本目录只是设计骨架，不能作为 Plugin 安装。

## Skill 设计

`skills/mcu-debug/SKILL.md` 保存所有模式共有的判断逻辑和安全边界，并根据实际任务按需加载参考资料：

- GDB 调试时读取 `gdb-debugging.md`。
- 串口运行观测时读取 `finsh-runtime.md`。
- FinSH 命令诊断时读取 `finsh-command.md`。
- 同时协调 GDB 和串口时读取 `combined-debugging.md`。

Skill 应先识别目标、当前状态和用户授权，再选择 MCP 工具。它负责解释诊断证据，但不重复实现 MCP 已经提供的确定性操作。

## MCP Server 设计

MCP Server 采用正式 Python 包的 `src` 布局。当前工程的 UCRT64 Python 虚拟环境可以用于开发和验证，但插件依赖必须由自己的 `pyproject.toml` 声明，不能永久依赖目标工程中的 `.venv`。

内部职责划分如下：

- `tools/`：定义对 Codex 暴露的结构化 MCP 工具，只做参数验证、权限前置条件检查和结果转换。
- `core/`：维护工作区、目标、GDB、串口和组合调试会话，是运行状态的唯一可信来源。
- `backends/`：封装 GDB/MI、J-Link GDB Server、Windows 进程和串口等外部能力。
- `protocols/`：解析 FinSH 文本、串口帧和事件格式，不直接管理进程或硬件资源。

如果 MCP 使用标准输入输出传输 JSON-RPC，所有 GDB Server、GDB 和串口输出都必须被捕获到内部缓冲区；不得直接写入 MCP 的标准输出，避免破坏协议流。诊断日志只能写入标准错误或受控日志文件。

## 会话模型

GDB Server、GDB 客户端和串口都是长连接资源，因此 MCP 必须是有状态服务。建议以 `session_id` 标识一次调试，并维护类似状态：

```text
DISCOVERED
  -> SERVER_READY
  -> ATTACHED
  -> RUNNING <-> HALTED
  -> DETACHED
  -> CLOSED
```

串口连接具有独立的 `CLOSED`、`OPEN` 和 `DEGRADED` 状态。当目标被暂停或复位时，组合会话负责更新串口状态，而不是让两个后端互相猜测。

每条事件应至少包含时间戳、来源、会话标识、序号和负载。串口日志与命令响应共用一个物理方向时，还需要区分 `LOG`、`EVENT` 和 `RESPONSE`，防止异步日志被误判为命令结果。

## MCP 工具边界

工具应围绕调试意图设计，而不是暴露一个可执行任意字符串的 `run_shell`。初步工具集合为：

```text
inspect_workspace
open_debug_session
attach_target
read_target_state
read_fault_state
set_breakpoint
resume_target
open_serial_session
read_runtime_events
send_finsh_command
close_debug_session
```

只读发现、运行控制和破坏现场的操作必须在接口上可区分。`halt` 会改变实时行为；`reset`、`load`、擦除和重新下载会破坏现场，必须由 Skill 在调用前取得明确授权，MCP 也应拒绝缺少必要参数或状态前提的请求。

## 与当前 RTOS 工程的关系

开发阶段可以借用当前工程已有的：

- Windows PowerShell 与 MSYS2 UCRT64 工具链；
- CMake Preset、带符号的 ELF、HEX、BIN 和 MAP；
- J-Link、SWD、GDB Server 与真实 STM32F4 目标板；
- RT-Thread、FinSH 和串口环境；
- 项目 `AGENTS.md` 中的构建、下载和调试约束。

这些内容都是测试环境，不是插件的内置默认值。MCP 每次应从目标工作区发现配置，或要求用户明确提供缺失参数。插件不得永久修改 Windows PATH，不得自动选择其他 BSP 的参数，也不得在未经授权时修改固件、下载、复位或擦除 MCU。

## 测试策略

测试分为三个层次：

1. 单元测试使用模拟进程、GDB/MI 数据和串口帧，验证解析、状态机及错误处理。
2. 集成测试启动可控的假 GDB Server 或虚拟串口，验证 MCP 工具、进程生命周期和事件流。
3. 硬件测试使用当前 RTOS 工程和真实开发板，只在明确选择目标并授权相应硬件操作后运行。

硬件测试不得成为默认测试的一部分。测试失败后必须关闭 GDB、调试服务器和串口句柄，避免影响下一次会话。

## 实施顺序

1. 创建并验证 Plugin 清单、MCP 配置和最小 Skill。
2. 建立 Python 包、MCP Server 入口、错误模型和会话管理器。
3. 实现工作区发现以及 GDB/MI、J-Link 后端，先完成只读附加和故障采集。
4. 实现串口运行观测、事件缓冲及游标读取。
5. 实现 FinSH 命令发送、响应关联及副作用分类。
6. 实现 GDB 与串口组合状态协调。
7. 完成模拟测试、当前 STM32F4 硬件验证和 Plugin 安装验证。

每一阶段先建立可观察、可关闭、可失败恢复的最小闭环，再扩展更多命令或调试服务器。当前设计不承诺首版同时支持所有探针、RTOS 和串口协议。

## 当前状态

目前只完成目录骨架和整体设计文档，尚未创建 Plugin 清单、Skill、MCP 配置或 MCP Server 源码，也没有修改、编译或下载 MCU 固件。
