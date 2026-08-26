# RT-Thread Debugger

本项目用于构建通用的 RT-Thread MCU 调试工具，统一提供 GDB 停机调试、串口运行观测和 FinSH 命令诊断能力。`skill` 负责向 Agent 描述调试流程，`mcp` 中的程序后续编译为独立可执行文件，可由支持 MCP 的不同客户端调用，不限定于 Codex。

当前 RTOS 工程仅作为开发和硬件验证环境，不属于本工具的实现代码。

## 软件架构

```text
rtthread-debugger/
├─ skill/                  Agent Skill 源目录，使用时复制并重命名为 rtthread-debugger
│  ├─ SKILL.md             Skill 入口及调试流程
│  ├─ agents/              Agent 相关元数据
│  ├─ references/          按调试模式拆分的参考说明
│  └─ scripts/             Skill 所需的辅助资源
├─ mcp/                    通用 MCP 调试程序
│  ├─ core/                GDB 与串口共用的运行状态
│  │  ├─ session/          调试会话及资源生命周期
│  │  ├─ state/            MCU、GDB 和串口状态
│  │  └─ events/           调试事件及结果传递
│  ├─ gdb/                 GDB 停机调试
│  │  ├─ gdb-mi/           GDB/MI 通信与命令解析
│  │  ├─ jlink/            J-Link GDB Server 适配
│  │  └─ fault-analysis/   HardFault 等异常分析
│  ├─ uart/                串口在线调试
│  │  ├─ serial-port/      串口连接与数据收发
│  │  ├─ runtime-monitor/  MCU 到 PC 的运行日志和状态观测
│  │  └─ finsh/            PC 到 MCU 的命令及 MCU 返回结果
│  └─ server/              MCP 服务入口
│     ├─ mcp-transport/    MCP 消息传输
│     └─ tool-registry/    对外调试工具注册
├─ docs/                   架构、协议及使用文档
├─ AGENTS.md               本工程的 Agent 开发约束
└─ README.md               项目架构说明
```

整体调用关系为：

```text
用户或 Agent
  → Skill 选择 GDB、运行观测、FinSH 命令或组合调试流程
  → MCP Client 启动编译后的调试程序
  → server 接收工具调用并交给 core 管理会话和状态
  → gdb 或 uart 执行具体调试操作
  → GDB/J-Link 或串口连接目标 MCU
```

`core` 是 GDB 与串口调试的公共协调层。GDB 暂停或复位 MCU 时，`core` 同步更新串口状态，避免把目标暂停误判为串口故障。`gdb` 负责断点、寄存器、内存、调用栈和异常分析；`uart` 中的 `runtime-monitor` 负责 MCU 到 PC 的运行观测，`finsh` 负责 PC 到 MCU 再返回 PC 的命令诊断。

Skill 与 MCP 独立交付：`skill/` 使用时复制到目标 Agent 的 skills 目录；MCP 程序编译为可执行文件并注册到对应客户端。两者通过 MCP 工具名称协作，不依赖固定安装路径。
