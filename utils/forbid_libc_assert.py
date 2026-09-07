#!/usr/bin/env python3
"""Forbid libc assert() in first-party code.

Offending assert() calls are rewritten in place to debug_assert() from
<bsod/bsod.h> (adding the include when missing) and the hook fails so the
changes can be reviewed and staged. Scope: src/ and the first-party
libraries (Marlin, WUI, Prusa-Firmware-MMU). A line can opt out with a
`// libc-assert-allowed` marker.
"""
from __future__ import annotations

import re
import subprocess
import sys
from collections import defaultdict

ASSERT_PATTERN = re.compile(r"\bassert\s*\(")


def is_code_use(line: str) -> bool:
    if "libc-assert-allowed" in line:
        return False
    # continuation line of a multi-line /* */ block comment
    if line.lstrip().startswith("*"):
        return False
    # strip // comments and (possibly unclosed) /* */ comments
    code = re.sub(r"//.*|/\*.*?(\*/|$)", "", line)
    return ASSERT_PATTERN.search(code) is not None


def find_offenders() -> dict[str, list[int]]:
    grep = subprocess.run(
        [
            "git", "grep", "-nE", ASSERT_PATTERN.pattern, "--", "src",
            "lib/Marlin", "lib/WUI", "lib/Prusa-Firmware-MMU",
            ":(exclude)lib/Prusa-Firmware-MMU/lib"
        ],
        capture_output=True,
        text=True,
    )
    offenders = defaultdict(list)
    for hit in grep.stdout.splitlines():
        path, line_number, line = hit.split(":", 2)
        if path.endswith((".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp",
                          ".hxx", ".ipp", ".tpp")) and is_code_use(line):
            offenders[path].append(int(line_number))
    return offenders


def add_bsod_include(lines: list[str]) -> bool:
    """Insert the bsod include after the last #include that is not inside an #if block."""
    if any(re.search(r'#\s*include\s*[<"][^">]*bsod', line) for line in lines):
        return True

    depth = 0
    last_include = None
    for i, line in enumerate(lines):
        if re.match(r"\s*#\s*if", line):  # #if, #ifdef, #ifndef
            depth += 1
        elif re.match(r"\s*#\s*endif", line):
            depth -= 1
        elif depth == 0 and re.match(r"\s*#\s*include\b", line):
            last_include = i

    if last_include is None:
        return False
    lines.insert(last_include + 1, "#include <bsod/bsod.h>\n")
    return True


def rewrite(path: str, line_numbers: list[int]) -> bool:
    with open(path, encoding="utf-8") as f:
        lines = f.readlines()
    for line_number in line_numbers:
        lines[line_number - 1] = ASSERT_PATTERN.sub("debug_assert(",
                                                    lines[line_number - 1])
    included = add_bsod_include(lines)
    with open(path, "w", encoding="utf-8") as f:
        f.writelines(lines)
    return included


def main() -> int:
    offenders = find_offenders()
    if not offenders:
        return 0

    print("libc assert() is forbidden in first-party code;")
    print(
        "offending calls were rewritten to debug_assert() from <bsod/bsod.h>.")
    print("Review the changes and stage them:")
    for path, line_numbers in offenders.items():
        include_added = rewrite(path, line_numbers)
        lines = ",".join(map(str, line_numbers))
        note = "" if include_added else " (add #include <bsod/bsod.h> manually)"
        print(f"  {path}:{lines}{note}")
    return 1


if __name__ == "__main__":
    sys.exit(main())
