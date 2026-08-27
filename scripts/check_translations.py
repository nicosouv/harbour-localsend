#!/usr/bin/env python3
"""Keeps the .ts catalogues honest against the QML sources.

lupdate is not run in CI - the catalogues are generated from
scripts/make_translations.py and committed - so nothing otherwise notices
when the two drift apart. And the drift is silent: a string with no entry, or
an entry under a context no file uses any more, builds and ships perfectly
and simply comes out in English.

Renaming a page is the sharp edge. Qt keys translations by context, and for
QML the context is the file's base name, so MainPage.qml becoming
DevicesPage.qml would orphan every one of its messages in every locale at
once, with nothing at all to show for it until somebody switched language.
"""

import re
import sys
from pathlib import Path
from xml.etree import ElementTree

ROOT = Path(__file__).resolve().parent.parent
QML_DIR = ROOT / "qml"
TS_DIR = ROOT / "translations"

# English is both the source language and the reference catalogue: every
# other locale is checked against it rather than against the QML, so a string
# only has to be found once.
TEMPLATE = "harbour-localsend-en.ts"

QSTR = re.compile(r'qsTr\(\s*"((?:[^"\\]|\\.)*)"')
BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
LINE_COMMENT = re.compile(r"//[^\n]*")


def strip_comments(text):
    """A qsTr() shown in a comment is documentation, not a string."""
    return LINE_COMMENT.sub("", BLOCK_COMMENT.sub("", text))


def unescape_qml(source):
    """QML escape sequences, resolved the way lupdate would resolve them."""
    return (source.replace("\\n", "\n").replace("\\t", "\t")
                  .replace("\\r", "\r").replace('\\"', '"')
                  .replace("\\\\", "\\"))


def qml_strings():
    """(context, source) pairs the QML actually asks to translate."""
    found = set()
    for path in sorted(QML_DIR.rglob("*.qml")):
        context = path.stem
        text = strip_comments(path.read_text(encoding="utf-8"))
        for source in QSTR.findall(text):
            found.add((context, unescape_qml(source)))
    return found


def ts_strings(path):
    root = ElementTree.parse(path).getroot()
    found = set()
    for context in root.findall("context"):
        name = context.find("name").text
        for message in context.findall("message"):
            source = message.find("source")
            if source is not None and source.text is not None:
                found.add((name, source.text))
    return found


def report(title, pairs, limit=12):
    print(f"\n{title} ({len(pairs)}):")
    for context, source in sorted(pairs)[:limit]:
        shown = source if len(source) <= 60 else source[:57] + "..."
        print(f"    {context}: {shown}")
    if len(pairs) > limit:
        print(f"    ... and {len(pairs) - limit} more")


def main():
    wanted = qml_strings()
    template_path = TS_DIR / TEMPLATE
    if not template_path.exists():
        print(f"{TEMPLATE} is missing")
        return 1

    template = ts_strings(template_path)
    problems = 0

    missing = wanted - template
    if missing:
        report(f"{TEMPLATE}: strings the QML uses with no entry", missing)
        problems += len(missing)

    orphaned = template - wanted
    if orphaned:
        report(f"{TEMPLATE}: entries no QML file asks for any more", orphaned)
        problems += len(orphaned)

    for path in sorted(TS_DIR.glob("*.ts")):
        if path.name == TEMPLATE:
            continue
        locale = ts_strings(path)

        absent = template - locale
        if absent:
            report(f"{path.name}: missing next to {TEMPLATE}", absent)
            problems += len(absent)

        extra = locale - template
        if extra:
            report(f"{path.name}: not in {TEMPLATE} any more", extra)
            problems += len(extra)

    catalogues = len(list(TS_DIR.glob("*.ts")))
    print(f"\nchecked {len(wanted)} strings across {catalogues} catalogues, "
          f"{problems} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
