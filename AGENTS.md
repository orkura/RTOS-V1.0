# AGENTS

## 变更确认规则

除非用户在当前请求中明确授权，否则不得自动修改任何文件内容，也不得执行任何 Git 操作（包括但不限于暂存、提交、切换分支、合并、拉取、推送、重置或清理）。在准备进行此类操作前，必须说明拟执行的具体操作及其影响，并等待用户明确确认。

## 终端环境

默认终端为 PowerShell，但本项目需要在 MSYS2 的 UCRT64 环境中执行构建、测试和其他命令。

需要进入该环境时，在 PowerShell 中执行：

```powershell
$env:MSYSTEM = 'UCRT64'
C:\msys64\usr\bin\bash.exe -li
```

其中，`-l` 表示以登录 Shell 方式加载 MSYS2 环境配置，`-i` 表示交互模式。进入后优先使用 UCRT64 提供的 `bash`、`make`、`gcc` 等工具。

## Python 虚拟环境

项目的 Python 虚拟环境位于 `.venv`，由 UCRT64 Python 创建。因此应先进入 UCRT64 Bash，再在项目根目录激活该环境：

```bash
. .venv/bin/activate
```

激活后再执行 `python`、`pip` 或项目的 Python 工具。完成后可执行 `deactivate` 退出虚拟环境。

## 构建与下载脚本

构建和下载操作只使用 `Scripts` 目录中的 Python 脚本；运行前必须已进入 UCRT64 Bash 并激活 `.venv`。

- 构建：`python Scripts/build.py --target-bsp <cc2538|stm32f4>`；默认目标为 `stm32f4`。
- 下载：构建成功后执行 `python Scripts/download.py`。默认下载 `build/stm32f4/rtthread-stm32f4.hex`，并使用 J-Link、`STM32F407ZG`、SWD 和 4000 kHz。
- 自动化会话可能不加载用户的 `.bashrc`。下载前应在 UCRT64 Bash 中设置 J-Link 路径并传给脚本：

  ```bash
  export JLINK=/d/App/Code/JLink/JLink_V968a/JLink.exe
  python Scripts/download.py --jlink "$JLINK"
  ```

  当前下载脚本不会自动读取 `JLINK`，因此必须保留 `--jlink "$JLINK"` 参数。
- 若上述路径不可用，先运行 `python Scripts/download.py --help`，再以 `--jlink` 显式指定已验证的 Commander 路径。

需要非默认 BSP、固件路径、J-Link 程序路径、芯片型号、调试接口或速度时，先运行相应脚本的 `--help`，再传入所需参数。不得使用非 Python 脚本进行构建或下载。
