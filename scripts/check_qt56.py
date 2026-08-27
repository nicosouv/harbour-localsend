#!/usr/bin/env python3
"""Flags APIs the device does not have.

Sailfish OS runs Qt 5.6 and a V4 engine that only implements ES5.1, but every
lane that compiles this project off-device - the Docker image, a desktop Qt,
an IDE - runs something far newer. Those all accept code the handset rejects,
and the rejection is not a build error: it is a QML file that fails to load at
runtime, or a C++ file that CI cross-builds and only then refuses.

So the gap gets its own gate. Each entry names the Qt or ECMAScript version
that introduced the symbol, and what to write instead.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = ROOT / "src"
QML_DIR = ROOT / "qml"

# (pattern, since, replacement). Word-bounded so `errorOccurred` does not fire
# on a member of ours that merely ends in it.
CPP_RULES = [
    (r"\bQRandomGenerator\b", "Qt 5.10", "QUuid::createUuid() as an entropy source"),
    (r"\bq?Overload\s*<", "Qt 5.7", "static_cast<void (Class::*)(args)>(&Class::method)"),
    (r"\bQt::SkipEmptyParts\b", "Qt 5.14", "QString::SkipEmptyParts"),
    (r"\bQt::KeepEmptyParts\b", "Qt 5.14", "QString::KeepEmptyParts"),
    (r"\bQt::endl\b", "Qt 5.14", "'\\n'"),
    (r"\bQt::flush\b", "Qt 5.14", "stream.flush()"),
    (r"\berrorOccurred\b", "Qt 5.15", "the error() signal, cast to disambiguate"),
    (r"\bQNetworkDatagram\b", "Qt 5.8", "readDatagram(char*, qint64, QHostAddress*, quint16*)"),
    (r"\btoSecsSinceEpoch\b", "Qt 5.8", "toMSecsSinceEpoch() / 1000"),
    (r"\bfromSecsSinceEpoch\b", "Qt 5.8", "fromMSecsSinceEpoch(secs * 1000)"),
    (r"\bcurrentSecsSinceEpoch\b", "Qt 5.8", "currentMSecsSinceEpoch() / 1000"),
    (r"\bformattedDataSize\b", "Qt 5.10", "a hand-rolled formatter"),
    (r"\bQStringView\b", "Qt 5.10", "QStringRef or QString"),
    (r"\bswapItemsAt\b", "Qt 5.13", "swap()"),
    (r"\bQ_NAMESPACE\b", "Qt 5.8", "a plain namespace with free functions"),
    (r"\bQScopeGuard\b", "Qt 5.12", "an explicit cleanup path"),
    (r"\bsingletonInstance\b", "Qt 5.12", "a context property"),
    (r"\bbirthTime\b", "Qt 5.10", "QFileInfo::created()"),
    (r"\bQNetworkInterface::type\b", "Qt 5.7", "flags() and the interface name"),
    (r"\bstartCommand\b", "Qt 5.15", "QProcess::start(program, arguments)"),
    (r"\bmoveToTrash\b", "Qt 5.15", "QFile::remove()"),
    (r"\bqEnvironmentVariableIntValue\b", "Qt 5.5", "qgetenv().toInt()"),
    (r"\bQ_DECLARE_LOGGING_CATEGORY\b.*\bQ_LOGGING_CATEGORY\b", "n/a", "unused"),
]

# The V4 engine in 5.6 is ES5.1. Everything below parses on a modern engine
# and throws - or silently yields undefined - on the device.
JS_RULES = [
    (r"(?<![\w.])(?:let|const)\s+[A-Za-z_$]", "ES6", "var"),
    (r"=>", "ES6 arrow function", "function () { ... }"),
    (r"`", "ES6 template literal", "string concatenation with +"),
    (r"\.includes\s*\(", "ES2016", "indexOf(x) !== -1"),
    (r"\.startsWith\s*\(", "ES6", "indexOf(x) === 0"),
    (r"\.endsWith\s*\(", "ES6", "lastIndexOf(x) === length - x.length"),
    (r"\.padStart\s*\(|\.padEnd\s*\(", "ES2017", "a manual pad loop"),
    (r"\bObject\.assign\b", "ES6", "an explicit copy loop"),
    (r"\bObject\.entries\b|\bObject\.values\b", "ES2017", "Object.keys() and lookups"),
    (r"\bArray\.from\b", "ES6", "a push loop"),
    (r"\bPromise\b", "ES6", "signals and callbacks"),
    (r"\bSet\s*\(|\bMap\s*\(", "ES6", "a plain object used as a dictionary"),
    (r"\bfind\s*\(\s*function", "ES6 Array.find", "a for loop"),
    (r"\bQt\.callLater\b", "Qt 5.8", "a zero-interval Timer"),
    (r"\.\.\.", "ES6 spread", "an explicit argument list"),
]

# QML imports newer than the platform. QtQuick 2.6 ships with Qt 5.6.
IMPORT_RULE = re.compile(r"^\s*import\s+(QtQuick|QtQml)(?:\.\w+)*\s+(\d+)\.(\d+)")
MAX_QTQUICK_MINOR = 6

BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
LINE_COMMENT = re.compile(r"//[^\n]*")
STRING_LITERAL = re.compile(r'"(?:[^"\\]|\\.)*"' r"|'(?:[^'\\]|\\.)*'")


def blank_noise(text, keep_backticks=False):
    """Comments and string bodies, blanked out but line-preserving.

    A rule that fires on the word "includes" inside a user-facing sentence, or
    on an example in a comment, is a rule nobody keeps running.
    """
    def blank(match):
        return re.sub(r"[^\n]", " ", match.group(0))

    text = BLOCK_COMMENT.sub(blank, text)
    text = LINE_COMMENT.sub(blank, text)
    if not keep_backticks:
        text = STRING_LITERAL.sub(blank, text)
    return text


def scan(path, rules, keep_backticks=False):
    findings = []
    text = blank_noise(path.read_text(encoding="utf-8"), keep_backticks)
    for number, line in enumerate(text.splitlines(), start=1):
        for pattern, since, instead in rules:
            if re.search(pattern, line):
                findings.append((number, line.strip(), since, instead))
                break
    return findings


def check_imports(path):
    findings = []
    for number, line in enumerate(
            path.read_text(encoding="utf-8").splitlines(), start=1):
        match = IMPORT_RULE.match(line)
        if not match:
            continue
        major, minor = int(match.group(2)), int(match.group(3))
        if major > 2 or (major == 2 and minor > MAX_QTQUICK_MINOR):
            findings.append((number, line.strip(), "newer than Qt 5.6",
                             f"{match.group(1)} 2.{MAX_QTQUICK_MINOR} or lower"))
    return findings


def main():
    problems = 0
    checked = 0

    for path in sorted(SRC_DIR.rglob("*.cpp")) + sorted(SRC_DIR.rglob("*.h")):
        checked += 1
        for number, snippet, since, instead in scan(path, CPP_RULES):
            print(f"{path.relative_to(ROOT)}:{number}: added in {since}")
            print(f"    {snippet}")
            print(f"    use {instead}")
            problems += 1

    # QML carries JavaScript, so it gets both gates. Backticks are meaningful
    # here and never legitimate, so string bodies stay visible for that rule.
    for path in sorted(QML_DIR.rglob("*.qml")) + sorted(QML_DIR.rglob("*.js")):
        checked += 1
        for number, snippet, since, instead in (
                scan(path, JS_RULES) + check_imports(path)):
            print(f"{path.relative_to(ROOT)}:{number}: {since}")
            print(f"    {snippet}")
            print(f"    use {instead}")
            problems += 1

    print(f"\nchecked {checked} files for Qt 5.6 compatibility, "
          f"{problems} problem(s)")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
