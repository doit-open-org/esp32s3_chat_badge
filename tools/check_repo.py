#!/usr/bin/env python3
"""Static checks that do not require the ESP-IDF toolchain."""

from __future__ import annotations

import csv
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BOARD = ROOT / "main/boards/esp32s3-chat-badge"
FLASH_SIZE = 16 * 1024 * 1024

REQUIRED = [
    ROOT / "CMakeLists.txt",
    ROOT / "sdkconfig.defaults",
    ROOT / "sdkconfig.defaults.esp32s3",
    ROOT / "partitions/esp32s3_chat_badge_16m.csv",
    BOARD / "CMakeLists.txt",
    BOARD / "sdkconfig.defaults",
    BOARD / "config.h",
    BOARD / "esp32s3_chat_badge.cc",
    BOARD / "zw_sdcard.c",
    ROOT / "main/boards/common/wifi_board_c_api.h",
    ROOT / "main/display/page_manager/LICENSE",
    ROOT / "THIRD_PARTY_NOTICES.md",
]

TEXT_SUFFIXES = {
    ".c", ".cc", ".cpp", ".h", ".hpp", ".cmake", ".txt", ".md", ".yml",
    ".yaml", ".json", ".py", ".sh", ".ps1", ".csv", ".cfg", ".html",
}
FORBIDDEN_TEXT = {
    "git.doit": "internal Git host",
    "/home/zgf/": "developer-local absolute path",
    "firmware/zuowei_v2": "legacy internal board path",
    "firmware/_common_v2": "legacy internal assets path",
    "-----BEGIN PRIVATE KEY-----": "private key material",
    "-----BEGIN OPENSSH PRIVATE KEY-----": "SSH private key material",
}
TOKEN_PATTERNS = {
    "GitHub token": re.compile(r"\bgh[pousr]_[A-Za-z0-9_]{30,}\b"),
    "AWS access key": re.compile(r"\bAKIA[0-9A-Z]{16}\b"),
}
LOCAL_PATH_PATTERNS = {
    "Linux home-directory path": re.compile(r"(?<![A-Za-z0-9_])/home/[^/\s]+/"),
    "macOS home-directory path": re.compile(r"(?<![A-Za-z0-9_])/Users/[^/\s]+/"),
    "Windows user-profile path": re.compile(r"\b[A-Za-z]:[\\/]Users[\\/][^\\/\s]+[\\/]"),
}

# Reject formatted log statements that pass a password buffer to a %s slot.
# Password lengths and generic status messages are permitted.
PASSWORD_LOG_PATTERN = re.compile(
    r"(?:ESP_LOG[A-Z]|BLUFI_[A-Z]+|printf|fprintf)\s*\([^;\n]*%s[^;\n]*"
    r"(?:\.|->)(?:password|passwd)\b",
    re.IGNORECASE,
)


def parse_size(value: str) -> int:
    value = value.strip()
    if not value:
        raise ValueError("empty numeric value")
    suffix = value[-1].upper()
    multiplier = 1
    if suffix == "K":
        multiplier = 1024
        value = value[:-1]
    elif suffix == "M":
        multiplier = 1024 * 1024
        value = value[:-1]
    return int(value, 0) * multiplier


def check_required(errors: list[str]) -> None:
    for path in REQUIRED:
        if not path.is_file():
            errors.append(f"missing required file: {path.relative_to(ROOT)}")


def check_board_sources(errors: list[str]) -> None:
    cmake = (BOARD / "CMakeLists.txt").read_text(encoding="utf-8")
    names = re.findall(r'"\$\{BOARD_PATH\}/([^"\n]+)"', cmake)
    if not names:
        errors.append("board CMakeLists.txt does not declare board sources")
    for name in names:
        if not (BOARD / name).is_file():
            errors.append(f"board CMake source does not exist: {name}")


def check_partition_table(errors: list[str]) -> None:
    path = ROOT / "partitions/esp32s3_chat_badge_16m.csv"
    rows: list[tuple[str, int, int]] = []
    with path.open(newline="", encoding="utf-8") as handle:
        for raw in csv.reader(handle):
            if not raw or raw[0].lstrip().startswith("#"):
                continue
            if len(raw) < 5:
                errors.append(f"invalid partition row: {raw}")
                continue
            name, offset_raw, size_raw = raw[0].strip(), raw[3].strip(), raw[4].strip()
            try:
                offset = parse_size(offset_raw)
                size = parse_size(size_raw)
            except ValueError as exc:
                errors.append(f"invalid partition value for {name}: {exc}")
                continue
            rows.append((name, offset, size))

    for name, offset, size in rows:
        if offset + size > FLASH_SIZE:
            errors.append(f"partition {name} exceeds 16 MB flash")
        if offset % 0x1000:
            errors.append(f"partition {name} offset is not 4 KB aligned")

    ordered = sorted(rows, key=lambda item: item[1])
    for previous, current in zip(ordered, ordered[1:]):
        prev_end = previous[1] + previous[2]
        if current[1] < prev_end:
            errors.append(f"partition overlap: {previous[0]} and {current[0]}")


def iter_text_files():
    ignored_parts = {".git", "build", "managed_components", "__pycache__"}
    scanner_path = Path(__file__).resolve()
    for path in ROOT.rglob("*"):
        if path.resolve() == scanner_path:
            continue
        if not path.is_file() or any(part in ignored_parts for part in path.parts):
            continue
        if path.suffix.lower() in TEXT_SUFFIXES or path.name in {"CMakeLists.txt", "Kconfig", "Kconfig.projbuild", "LICENSE"}:
            yield path


def check_text(errors: list[str]) -> None:
    for path in iter_text_files():
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        rel = path.relative_to(ROOT)
        for needle, description in FORBIDDEN_TEXT.items():
            if needle in text:
                errors.append(f"{rel}: contains {description}: {needle!r}")
        for description, pattern in TOKEN_PATTERNS.items():
            if pattern.search(text):
                errors.append(f"{rel}: contains a possible {description}")
        for description, pattern in LOCAL_PATH_PATTERNS.items():
            if pattern.search(text):
                errors.append(f"{rel}: contains a possible {description}")
        for line_number, line in enumerate(text.splitlines(), start=1):
            if PASSWORD_LOG_PATTERN.search(line):
                errors.append(
                    f"{rel}:{line_number}: appears to log a plaintext password buffer"
                )


def check_nested_git(errors: list[str]) -> None:
    for path in ROOT.rglob(".git"):
        if path == ROOT / ".git":
            continue
        errors.append(f"nested Git metadata found: {path.relative_to(ROOT)}")


def check_obsolete_components(errors: list[str]) -> None:
    obsolete_paths = {
        ROOT / "components/doit_blufi":
            "duplicates symbols from blufi_wificfg",
        ROOT / "components/78__esp-ml307":
            "is a cellular-only component not used by this Wi-Fi board",
        ROOT / "components/doit_min_program_upload/assets":
            "contains the unused legacy LittleFS material bundle",
        ROOT / "components/espressif__esp_codec_dev":
            "is an unused generic codec component for this VB6824-only board",
        ROOT / "main/display/oled_display.cc":
            "is an unused display implementation for this LCD-only board",
        ROOT / "main/led/single_led.cc":
            "is an unused addressable-LED implementation for this board",
        ROOT / "main/audio/codecs/es8311_audio_codec.cc":
            "is an unused audio codec implementation for this VB6824-only board",
    }
    for path, reason in obsolete_paths.items():
        if path.exists():
            errors.append(f"obsolete {path.relative_to(ROOT)} {reason}")


def check_single_board_sources(errors: list[str]) -> None:
    cmake = (ROOT / "main/CMakeLists.txt").read_text(encoding="utf-8")
    forbidden = (
        "boards/common/ml307_board.cc",
        "boards/common/dual_network_board.cc",
        "file(GLOB BOARD_COMMON_SOURCES",
        '"audio/codecs/es8311_audio_codec.cc"',
        '"audio/codecs/jy6311_audio_codec.cc"',
        '"led/single_led.cc"',
        '"display/oled_display.cc"',
    )
    for needle in forbidden:
        if needle in cmake:
            errors.append(f"single-board CMake still includes legacy source selector: {needle}")


def main() -> int:
    errors: list[str] = []
    check_required(errors)
    if not errors:
        check_board_sources(errors)
        check_partition_table(errors)
    check_text(errors)
    check_nested_git(errors)
    check_obsolete_components(errors)
    check_single_board_sources(errors)

    if errors:
        print("Repository checks failed:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1

    print("Repository checks passed:")
    print("  - required board and documentation files are present")
    print("  - board CMake source references resolve")
    print("  - 16 MB partition table is aligned and non-overlapping")
    print("  - no internal/developer paths, private keys, common token formats, or plaintext password logs found")
    print("  - obsolete duplicate BLUFI, cellular-only, and legacy LittleFS paths are absent")
    print("  - common source selection is limited to the Wi-Fi badge board")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
