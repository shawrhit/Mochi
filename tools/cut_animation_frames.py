#!/usr/bin/env python3
from pathlib import Path
import re
import sys


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = REPO_ROOT / "src" / "custom_animations_master.h"
DEFAULT_OUTPUT = REPO_ROOT / "src" / "custom_animations.h"


def rebuild_frames(source_path: Path, output_path: Path, keep_every: int) -> None:
    if keep_every < 1:
        raise ValueError("keep_every must be >= 1")

    text = source_path.read_text()
    lines = text.splitlines(keepends=True)

    frame_def_re = re.compile(
        r"^const unsigned char\s+(\w+)_frame_(\d+)\s*\[\]\s+PROGMEM\s*=\s*\{\s*$"
    )

    kept_frames = {}
    new_lines = []

    i = 0
    while i < len(lines):
        line = lines[i]
        m = frame_def_re.match(line)
        if m:
            prefix = m.group(1)
            idx = int(m.group(2))
            block = [line]
            i += 1
            while i < len(lines):
                block.append(lines[i])
                if lines[i].strip() == "};":
                    i += 1
                    break
                i += 1

            if idx % keep_every == 0:
                new_lines.extend(block)
                kept_frames.setdefault(prefix, []).append(f"{prefix}_frame_{idx:04d}")
            continue

        new_lines.append(line)
        i += 1

    text2 = "".join(new_lines)

    for prefix, frames in kept_frames.items():
        count_pattern = rf"(const int {re.escape(prefix)}_frames_count = )\d+;"
        text2 = re.sub(count_pattern, lambda m: f"{m.group(1)}{len(frames)};", text2)

        array_lines = []
        for idx, name in enumerate(frames):
            if idx % 8 == 0:
                array_lines.append("\t" + name)
            else:
                array_lines[-1] += ", " + name

        for j in range(len(array_lines) - 1):
            array_lines[j] += ","

        block = "\n".join(array_lines)
        array_pattern = rf"(const unsigned char\* const {re.escape(prefix)}_allArray\[\] PROGMEM = \{{\n)(.*?)(\n\}};)"
        text2 = re.sub(
            array_pattern,
            lambda m: f"{m.group(1)}{block}{m.group(3)}",
            text2,
            flags=re.DOTALL,
        )

    output_path.write_text(text2)


def main() -> int:
    keep_every = int(sys.argv[1]) if len(sys.argv) >= 2 else 2
    source_path = Path(sys.argv[2]).expanduser().resolve() if len(sys.argv) >= 3 else DEFAULT_SOURCE
    output_path = Path(sys.argv[3]).expanduser().resolve() if len(sys.argv) >= 4 else DEFAULT_OUTPUT

    if not source_path.exists():
        print(f"Source file not found: {source_path}", file=sys.stderr)
        return 1

    if source_path == output_path:
        print("Source and output must be different files.", file=sys.stderr)
        return 1

    rebuild_frames(source_path, output_path, keep_every)
    print(
        f"Updated {output_path} from {source_path} (keep_every={keep_every})."
    )
    print(
        "Usage: cut_animation_frames.py [keep_every] [source_header] [output_header]"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
