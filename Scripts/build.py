#!/usr/bin/env python3
"""Configure and build one RT-Thread BSP with CMake and Ninja.

This helper intentionally does not select a BSP or build type implicitly:
--target-bsp and --build-type must be supplied by the caller and are passed to
CMake for project-level validation.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Iterable


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Configure and build one RT-Thread BSP with CMake and Ninja."
    )
    parser.add_argument(
        "--target-bsp",
        required=True,
        help=(
            "BSP selected through CMake TARGET_BSP; must be specified explicitly "
            "and is validated by the project configuration."
        ),
    )
    parser.add_argument(
        "--build-type",
        required=True,
        choices=("Debug", "Release"),
        help="CMake build type; must be Debug or Release.",
    )
    parser.add_argument(
        "--arm-toolchain-root",
        type=Path,
        help=(
            "Toolchain root whose bin directory contains arm-none-eabi-gcc. "
            "When omitted, the compiler is found on PATH."
        ),
    )
    parser.add_argument(
        "--cmake",
        help="CMake executable or absolute path (default: find cmake on PATH).",
    )
    parser.add_argument(
        "--ninja",
        help="Ninja executable or absolute path (default: find ninja on PATH).",
    )
    parser.add_argument(
        "--build-directory",
        type=Path,
        help="Build directory, relative to the project root when not absolute.",
    )
    parser.add_argument(
        "--fresh",
        action="store_true",
        help="Reconfigure from a fresh CMake cache.",
    )
    return parser.parse_args()


def executable_names(name: str) -> Iterable[str]:
    """Yield platform-appropriate executable candidates, without duplicates."""
    yield name
    if os.name == "nt" and not name.lower().endswith(".exe"):
        yield f"{name}.exe"


def find_on_path(name: str, description: str) -> Path:
    for candidate in executable_names(name):
        found = shutil.which(candidate)
        if found:
            return Path(found).resolve()
    raise FileNotFoundError(f"{description} is not available on PATH: {name}")


def resolve_executable(value: str | None, name: str, description: str) -> Path:
    """Resolve an explicit executable path or find the named command on PATH."""
    if value is None:
        return find_on_path(name, description)

    supplied = Path(value).expanduser()
    if supplied.is_file():
        return supplied.resolve()

    # Supplying a command name such as "ninja" is also convenient in shells.
    return find_on_path(value, description)


def resolve_compiler(toolchain_root: Path | None) -> tuple[Path, Path | None]:
    if toolchain_root is None:
        return find_on_path("arm-none-eabi-gcc", "ARM GCC"), None

    bin_directory = toolchain_root.expanduser().resolve() / "bin"
    for name in executable_names("arm-none-eabi-gcc"):
        compiler = bin_directory / name
        if compiler.is_file():
            return compiler, bin_directory
    raise FileNotFoundError(
        "ARM GCC not found below the supplied toolchain root: "
        f"{bin_directory}"
    )


def checked_run(command: list[str], environment: dict[str, str] | None = None) -> None:
    try:
        subprocess.run(command, check=True, env=environment)
    except subprocess.CalledProcessError as error:
        raise RuntimeError(
            f"Command failed with exit code {error.returncode}: {error.cmd[0]}"
        ) from error


def main() -> int:
    arguments = parse_arguments()
    project_root = Path(__file__).resolve().parent.parent

    cmake = resolve_executable(arguments.cmake, "cmake", "CMake")
    ninja = resolve_executable(arguments.ninja, "ninja", "Ninja")
    compiler, toolchain_bin = resolve_compiler(arguments.arm_toolchain_root)

    if arguments.build_directory is None:
        build_directory = (
            project_root
            / "build"
            / f"{arguments.target_bsp}-{arguments.build_type.lower()}"
        )
    elif arguments.build_directory.is_absolute():
        build_directory = arguments.build_directory
    else:
        build_directory = project_root / arguments.build_directory
    build_directory = build_directory.resolve()

    firmware_base_name = f"rtthread-{arguments.target_bsp}"
    expected_artifacts = (
        build_directory / f"{firmware_base_name}.elf",
        build_directory / f"{firmware_base_name}.hex",
        build_directory / f"{firmware_base_name}.bin",
        build_directory / f"{firmware_base_name}.map",
    )

    environment = os.environ.copy()
    if toolchain_bin is not None:
        environment["PATH"] = str(toolchain_bin) + os.pathsep + environment.get("PATH", "")

    print(f"Project:       {project_root}")
    print(f"BSP:           {arguments.target_bsp}")
    print(f"Build type:    {arguments.build_type}")
    print(f"Build dir:     {build_directory}")
    print(f"CMake:         {cmake}")
    print(f"Ninja:         {ninja}")
    print(f"ARM GCC:       {compiler}")

    configure_arguments = [
        str(cmake),
        "-S",
        str(project_root),
        "-B",
        str(build_directory),
        "-G",
        "Ninja",
        "-DCMAKE_SYSTEM_NAME=Generic",
        "-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY",
        f"-DCMAKE_C_COMPILER={compiler}",
        f"-DCMAKE_ASM_COMPILER={compiler}",
        f"-DCMAKE_MAKE_PROGRAM={ninja}",
        f"-DTARGET_BSP={arguments.target_bsp}",
        f"-DCMAKE_BUILD_TYPE={arguments.build_type}",
    ]
    if arguments.fresh:
        configure_arguments.insert(1, "--fresh")

    checked_run(configure_arguments, environment)
    checked_run([str(cmake), "--build", str(build_directory), "--parallel"], environment)

    missing_artifacts = [path for path in expected_artifacts if not path.is_file()]
    if missing_artifacts:
        missing = "\n".join(f"  {path}" for path in missing_artifacts)
        raise FileNotFoundError(f"Build completed but expected artifact(s) are missing:\n{missing}")

    print("\nBuild completed:")
    for label, artifact in zip(("ELF", "HEX", "BIN", "MAP"), expected_artifacts):
        print(f"{label}: {artifact}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (FileNotFoundError, RuntimeError) as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(1)
