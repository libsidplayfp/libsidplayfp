#!/usr/bin/env python3
"""Insert __EMSCRIPTEN__ thread guards into FilterModelConfig sources.

The upstream libsidplayfp sources spawn helper threads while building
filter lookup tables. Emscripten's single-threaded runtime cannot
satisfy those pthread calls, so we wrap the section with a
`#if defined(__EMSCRIPTEN__)` branch that executes the builder lambdas
sequentially.

This script is designed to be idempotent: running it multiple times
leaves the files unchanged after the first pass. It searches for the
`sidThread` declarations in each FilterModelConfig file and derives the
list of lambdas to invoke sequentially, so it will continue to work if
upstream reorders the declarations or adds new ones.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
from typing import Iterable, List

THREAD_MARKER = "#if defined(HAVE_CXX20) && defined(__cpp_lib_jthread)"
SEQUENTIAL_COMMENT = (
    "// Execute sequentially when pthreads are unavailable in the WASM build."
)

THREAD_CALL_REGEX = re.compile(r"sidThread\s+\w+\(\s*(\w+)\s*\)")
TAIL_REGEX = re.compile(r"(#endif\s*\n)(\s*})", re.MULTILINE)


def discover_thread_calls(contents: str) -> List[str]:
    """Return the unique lambda names used to spawn worker threads."""

    calls: List[str] = []
    for match in THREAD_CALL_REGEX.finditer(contents):
        name = match.group(1)
        if name not in calls:
            calls.append(name)
    return calls


def inject_guard(contents: str, calls: Iterable[str]) -> str:
    """Inject the __EMSCRIPTEN__ guard block before the thread marker."""

    if "#if defined(__EMSCRIPTEN__)" in contents:
        # Already patched.
        return contents

    try:
        marker_index = contents.index(THREAD_MARKER)
    except ValueError:
        return contents  # Marker missing; leave file untouched.

    line_start = contents.rfind("\n", 0, marker_index) + 1
    line_end = contents.find("\n", marker_index)
    if line_end == -1:
        line_end = len(contents)
    else:
        line_end += 1  # include newline

    original_line = contents[line_start:line_end]
    indent = original_line[: original_line.index("#")]

    sequential_lines = "\n".join(f"{indent}    {call}();" for call in calls)
    guard_block = (
        f"{indent}#if defined(__EMSCRIPTEN__)\n"
        f"{indent}    {SEQUENTIAL_COMMENT}\n"
        f"{sequential_lines}\n"
        f"{indent}#else"
    )

    with_guard = contents[:line_start] + guard_block + "\n" + original_line + contents[line_end:]

    # Ensure we close the new #if guard before the function ends.
    if not re.search(r"#endif\s*\n#endif\s*\n", with_guard):
        def _append_guard(match: re.Match[str]) -> str:
            return match.group(1) + "#endif\n" + match.group(2)

        with_guard, count = TAIL_REGEX.subn(_append_guard, with_guard, count=1)
        if count == 0:
            raise RuntimeError("Unable to append closing #endif for __EMSCRIPTEN__ guard")

    return with_guard


def process_file(path: pathlib.Path) -> bool:
    contents = path.read_text()
    calls = discover_thread_calls(contents)
    if not calls:
        return False

    updated = inject_guard(contents, calls)
    if updated == contents:
        return False

    path.write_text(updated)
    return True


def main(argv: List[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "root", type=pathlib.Path, help="Path to the cloned libsidplayfp repository"
    )
    args = parser.parse_args(argv)

    root = args.root.resolve()
    targets = sorted(root.glob("src/builders/**/FilterModelConfig*.cpp"))
    if not targets:
        print("No FilterModelConfig sources found", file=sys.stderr)
        return 1

    modified_any = False
    for path in targets:
        try:
            modified = process_file(path)
        except Exception as exc:  # pragma: no cover - propagated as build failure
            print(f"Failed to update {path.relative_to(root)}: {exc}", file=sys.stderr)
            return 1
        if modified:
            modified_any = True
            rel = path.relative_to(root)
            print(f"Applied __EMSCRIPTEN__ guard to {rel}")

    if not modified_any:
        print("Thread guards already present; no changes applied.")
    return 0


if __name__ == "__main__":  # pragma: no cover
    sys.exit(main(sys.argv[1:]))
