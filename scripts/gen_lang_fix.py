#!/usr/bin/env python3
"""Generate the language header with product-specific string/audio overrides."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
from typing import Any


HEADER_TEMPLATE = """// Auto-generated language config
// Language: {lang_code} with en-US fallback
#pragma once

#include <string_view>

#ifndef {lang_macro}
    #define {lang_macro}
#endif

namespace Lang {{
    constexpr const char* CODE = "{lang_code}";

    namespace Strings {{
{strings}
    }}

    namespace Sounds {{
{sounds}
    }}
}}
"""


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        value = json.load(handle)
    if not isinstance(value, dict):
        raise ValueError(f"JSON root must be an object: {path}")
    return value


def get_strings(data: dict[str, Any], path: Path, *, require_language: bool) -> dict[str, str]:
    if require_language and "language" not in data:
        raise ValueError(f"Missing 'language' object: {path}")
    strings = data.get("strings")
    if not isinstance(strings, dict):
        raise ValueError(f"Missing or invalid 'strings' object: {path}")
    for key, value in strings.items():
        if not isinstance(key, str) or not isinstance(value, str):
            raise ValueError(f"Language entries must be string pairs: {path}")
    return strings


def get_sound_files(directory: Path) -> set[str]:
    if not directory.is_dir():
        return set()
    return {path.name for path in directory.iterdir() if path.is_file() and path.suffix == ".ogg"}


def sound_declaration(filename: str) -> str:
    base_name = Path(filename).stem
    upper_name = base_name.upper()
    return f'''        extern const char ogg_{base_name}_start[] asm("_binary_{base_name}_ogg_start");
        extern const char ogg_{base_name}_end[] asm("_binary_{base_name}_ogg_end");
        static const std::string_view OGG_{upper_name} {{
            static_cast<const char*>(ogg_{base_name}_start),
            static_cast<size_t>(ogg_{base_name}_end - ogg_{base_name}_start)
        }};'''


def generate_header(input_path: Path, input_fix_path: Path | None, output_path: Path) -> None:
    if not input_path.is_file():
        raise FileNotFoundError(f"Language file not found: {input_path}")

    lang_dir = input_path.parent
    lang_code = lang_dir.name
    assets_dir = lang_dir.parent.parent

    language_data = load_json(input_path)
    language_strings = get_strings(language_data, input_path, require_language=True)

    base_path = assets_dir / "locales" / "en-US" / "language.json"
    base_strings: dict[str, str] = {}
    if base_path.is_file():
        base_strings = get_strings(load_json(base_path), base_path, require_language=True)

    override_strings: dict[str, str] = {}
    override_language_dir: Path | None = None
    override_root: Path | None = None
    if input_fix_path is not None:
        override_language_dir = input_fix_path.parent
        override_root = override_language_dir.parent
        if input_fix_path.is_file():
            override_strings = get_strings(
                load_json(input_fix_path), input_fix_path, require_language=False
            )

    merged_strings = dict(base_strings)
    merged_strings.update(language_strings)
    merged_strings.update(override_strings)

    string_lines = []
    for key, value in sorted(merged_strings.items()):
        escaped_value = json.dumps(value, ensure_ascii=False)
        string_lines.append(
            f"        constexpr const char* {key.upper()} = {escaped_value};"
        )

    base_sounds = get_sound_files(assets_dir / "locales" / "en-US")
    language_sounds = get_sound_files(lang_dir)
    override_sounds = get_sound_files(override_language_dir) if override_language_dir else set()
    common_sounds = get_sound_files(assets_dir / "common")
    override_common_sounds = (
        get_sound_files(override_root / "common") if override_root else set()
    )

    language_sound_names = base_sounds | language_sounds | override_sounds
    common_sound_names = common_sounds | override_common_sounds
    duplicate_names = language_sound_names & common_sound_names
    if duplicate_names:
        names = ", ".join(sorted(duplicate_names))
        raise ValueError(f"Duplicate embedded sound basenames across language/common: {names}")

    sound_lines = [sound_declaration(name) for name in sorted(language_sound_names | common_sound_names)]

    fallback_count = len(set(base_strings) - set(language_strings) - set(override_strings))
    print(f"Processing language: {lang_code}")
    print(f"  base strings: {len(base_strings)}")
    print(f"  language strings: {len(language_strings)}")
    print(f"  override strings: {len(override_strings)}")
    print(f"  fallback strings: {fallback_count}")
    print(f"  embedded sounds: {len(sound_lines)}")

    content = HEADER_TEMPLATE.format(
        lang_code=lang_code,
        lang_macro=lang_code.replace("-", "_").lower(),
        strings="\n".join(string_lines),
        sounds="\n\n".join(sound_lines),
    )

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(content, encoding="utf-8", newline="\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate language config with en-US and product overrides"
    )
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--input_fix", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    try:
        generate_header(args.input, args.input_fix, args.output)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(f"error: {exc}")
        return 1

    print(f"Generated: {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
