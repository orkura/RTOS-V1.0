# SEGGER RTT character-device component

This component provides the STM32H743VIT BSP with a bidirectional RT-Thread
character device over J-Link Real Time Transfer (RTT).  It registers the
device `jlinkRtt` without taking ownership of the runtime console.  The
physical USART2 device `uart2` is the STM32H743VIT default console, while
`jlinkRtt` remains available for direct device access or a later runtime
console switch.

## Source and integration boundary

The files under `RTT/` are copied without functional changes from:

```text
F:\Project\GitOpen\RTTHREAD_SEGGER_TOOL\RTT
```

They contain SEGGER RTT version 7.20a and retain the original SEGGER license
notices.  The project-specific files are `CMakeLists.txt` and
`adapter/drv_rtt.c`.  The adapter deliberately uses the RT-Thread 4.1.1 base
character-device API because this repository does not include the RT-Thread
Serial V1/V2 framework used by the upstream adapter.

When refreshing the upstream component, replace only the five files under
`RTT/`, review the SEGGER version and license headers, then rebuild both Debug
and Release.  Do not overwrite the adapter or CMake integration with the
upstream SConscript implementation.

## Runtime configuration

- Up channel: 0, 1024-byte buffer, non-blocking skip mode
- Down channel: 0, 16-byte buffer
- Device name: `jlinkRtt`
- Default console: no (`uart2` owns the STM32H743VIT console)
- Receive polling: static RT-Thread worker, 5 ms interval
- RTT control block: `_SEGGER_RTT` in the normal zero-initialized data section

For STM32H743VIT the control block and buffers are linked into DTCM.  D-Cache
is currently disabled, so no RTT cache-maintenance policy is required.

## J-Link RTT Viewer

Use `D:\App\Code\JLink\JLink_V968a\JLinkRTTViewer.exe` with:

```text
Device:    STM32H743VI
Interface: SWD
Speed:     100 kHz
RTT CB:    Auto detection
```

If automatic detection fails, obtain `_SEGGER_RTT` from the ELF or MAP file and
enter its address explicitly.  Close a standalone J-Link Commander download
session before opening RTT Viewer, unless the viewer is attached through an
already active debugger connection.
