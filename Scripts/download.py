#!/usr/bin/env python3
"""Program an explicitly selected firmware image with SEGGER J-Link.

This is the cross-platform Python counterpart of download.ps1.  It only
downloads an existing image; invoke build.py first when a new image is needed.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from typing import Iterable


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download an explicitly selected firmware image using SEGGER J-Link."
    )
    parser.add_argument(
        "--jlink",
        required=True,
        help="J-Link Commander executable or absolute path; must be specified explicitly.",
    )
    parser.add_argument(
        "--firmware",
        type=Path,
        required=True,
        help="Firmware file, relative to the project root when not absolute; must be specified explicitly.",
    )
    parser.add_argument(
        "--device",
        required=True,
        help="J-Link device name; must be specified explicitly.",
    )
    parser.add_argument(
        "--interface",
        required=True,
        help="Debug interface; must be specified explicitly.",
    )
    parser.add_argument(
        "--speed",
        type=int,
        required=True,
        help="Debug speed in kHz; must be specified explicitly.",
    )
    return parser.parse_args()


def executable_names(name: str) -> Iterable[str]:
    yield name
    if os.name == "nt" and not name.lower().endswith(".exe"):
        yield f"{name}.exe"


def find_on_path(names: Iterable[str]) -> Path:
    for name in names:
        for candidate in executable_names(name):
            found = shutil.which(candidate)
            if found:
                return Path(found).resolve()
    names_text = ", ".join(names)
    raise FileNotFoundError(f"J-Link Commander is not available on PATH: {names_text}")


def resolve_jlink(value: str) -> Path:
    supplied = Path(value).expanduser()
    if supplied.is_file():
        return supplied.resolve()
    return find_on_path((value,))


def main() -> int:
    arguments = parse_arguments()
    project_root = Path(__file__).resolve().parent.parent
    jlink = resolve_jlink(arguments.jlink)

    firmware = arguments.firmware
    if not firmware.is_absolute():
        firmware = project_root / firmware
    firmware = firmware.resolve()
    if not firmware.is_file():
        raise FileNotFoundError(f"Firmware HEX not found: {firmware}")

    # J-Link's command file accepts forward slashes on all supported host OSes.
    firmware_for_jlink = firmware.as_posix()
    commands = "\n".join(
        (
            "RSetType 0",
            "r",
            "h",
            f'loadfile "{firmware_for_jlink}"',
            "r",
            "sleep 100",
            "g",
            "exit",
            "",
        )
    )

    command_file_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="ascii", newline="\n", suffix=".jlink", delete=False
        ) as command_file:
            command_file.write(commands)
            command_file_path = Path(command_file.name)

        command = [
            str(jlink),
            "-Device",
            arguments.device,
            "-If",
            arguments.interface,
            "-Speed",
            str(arguments.speed),
            "-AutoConnect",
            "1",
            "-CommanderScript",
            str(command_file_path),
        ]
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as error:
        raise RuntimeError(f"J-Link download failed with exit code {error.returncode}.") from error
    finally:
        if command_file_path is not None:
            command_file_path.unlink(missing_ok=True)

    print(f"Downloaded: {firmware}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (FileNotFoundError, RuntimeError) as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(1)
