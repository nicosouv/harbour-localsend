#!/usr/bin/env python3
"""Checks the .desktop file the way Harbour's validator will.

The RPM validator only runs in CI, inside the Sailfish SDK, after a full
cross-build - so a one-character mistake in this file costs a round trip of
several minutes to find. Everything here is a rule that validator enforces,
reimplemented against the same data files it uses.

The parsing rule is the sharp edge, and it is worth spelling out. The
validator pulls the section out with

    sed '1,/^\\[X-Sailjail\\]/d;/\\[/,$d'

which stops at the first line containing an opening bracket - *including a
line that is only a comment*. So a comment inside [X-Sailjail] that mentions,
say, "[X-Share Method files]" silently truncates the section, every key below
it disappears, and the error you get back is the baffling "Empty X-Sailjail
section not allowed". That cost one CI round trip; it should not cost another.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DESKTOP = ROOT / "harbour-localsend.desktop"

# sailfishos/sdk-harbour-rpmvalidator, allowed_sailjailkeys.conf
ALLOWED_KEYS = {
    "Permissions",
    "OrganizationName",
    "ApplicationName",
    "ExecDBus",
}

# sailfishos/sdk-harbour-rpmvalidator, allowed_permissions.conf. Sharing is
# deliberately not in this list, and neither is Privileged: the validator
# rejects both, whatever sailjail-permissions itself defines.
ALLOWED_PERMISSIONS = {
    "Audio", "Bluetooth", "Camera", "Internet", "Location", "MediaIndexing",
    "Microphone", "NFC", "RemovableMedia", "UserDirs", "WebView", "Documents",
    "Downloads", "Music", "Pictures", "PublicDir", "Videos", "Compatibility",
    "Secrets", "Contacts", "Accounts",
}


def sailjail_section(lines):
    """The X-Sailjail section exactly as the validator's sed would see it."""
    section = []
    started = False
    for line in lines:
        if not started:
            if line.strip() == "[X-Sailjail]":
                started = True
            continue
        # The validator's second sed expression, which does not care whether
        # the bracket opens a section or merely sits in a comment.
        if "[" in line:
            break
        section.append(line)
    return started, section


def check():
    problems = []

    if not DESKTOP.exists():
        return [f"{DESKTOP.name} is missing"]

    lines = DESKTOP.read_text(encoding="utf-8").splitlines()
    started, section = sailjail_section(lines)

    if not started:
        return ["no [X-Sailjail] section"]

    keys = [line for line in section if line.strip() and not line.startswith("#")]
    if not keys:
        problems.append(
            "the [X-Sailjail] section reads as empty. A '[' on a line inside "
            "it - a comment counts - truncates everything below it.")
        return problems

    for line in keys:
        if "=" not in line:
            problems.append(f"[X-Sailjail]: not a key=value line: {line}")
            continue

        key, value = line.split("=", 1)
        key = key.strip()

        if key not in ALLOWED_KEYS:
            problems.append(f"[X-Sailjail]: key is not allowed by Harbour: {key}")
            continue

        if key == "Permissions":
            for name in value.split(";"):
                name = name.strip()
                if name and name not in ALLOWED_PERMISSIONS:
                    problems.append(
                        f"[X-Sailjail]: permission is not allowed by "
                        f"Harbour: {name}")

        if key == "ApplicationName":
            if not re.match(r"^[A-Za-z_-][A-Z0-9a-z_-]*$", value.strip()):
                problems.append(
                    f"[X-Sailjail]: ApplicationName has illegal "
                    f"characters: {value.strip()}")

    return problems


def main():
    problems = check()
    for problem in problems:
        print(f"{DESKTOP.relative_to(ROOT)}: {problem}")

    print(f"\nchecked {DESKTOP.name}, {len(problems)} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
