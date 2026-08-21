#!/usr/bin/env python3
"""Convert this project's Kconfig .config file into RT-Thread's rtconfig.h."""

from __future__ import annotations

import argparse
from pathlib import Path


def macros_from_config(config_path: Path) -> list[tuple[str, str | None]]:
    """Return all enabled configuration macros in the order stored by Kconfig."""
    macros: list[tuple[str, str | None]] = []

    for raw_line in config_path.read_text(encoding="utf-8").splitlines():
        if not raw_line.startswith("CONFIG_") or "=" not in raw_line:
            continue

        config_name, value = raw_line.split("=", 1)
        macro_name = config_name.removeprefix("CONFIG_")
        if value == "n":
            continue
        if value == "y":
            macros.append((macro_name, None))
        else:
            macros.append((macro_name, value))

    return macros


def render_header(macros: list[tuple[str, str | None]], source: Path) -> str:
    lines = [
        "/*",
        f" * Generated from {source.as_posix()} by tools/kconfig_to_rtconfig.py.",
        " * Run menuconfig Kconfig, then rerun this script after changing options.",
        " */",
        "",
        "#ifndef __RTTHREAD_CFG_H__",
        "#define __RTTHREAD_CFG_H__",
        "",
    ]
    for name, value in macros:
        lines.append(f"#define {name}" if value is None else f"#define {name} {value}")
    lines.extend(["", "#endif /* __RTTHREAD_CFG_H__ */", ""])
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate RT-Thread rtconfig.h from a Kconfig .config file."
    )
    parser.add_argument("--config", type=Path, default=Path(".config"))
    parser.add_argument("--output", type=Path, default=Path("rtconfig.h"))
    args = parser.parse_args()

    if not args.config.is_file():
        parser.error(f"configuration file not found: {args.config}")

    args.output.write_text(
        render_header(macros_from_config(args.config), args.config), encoding="utf-8"
    )
    print(f"Generated {args.output} from {args.config}.")


if __name__ == "__main__":
    main()
