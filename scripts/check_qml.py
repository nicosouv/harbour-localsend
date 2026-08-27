#!/usr/bin/env python3
"""Static checks on the QML sources.

qmllint covers syntax. These are the traps it has nothing to say about: code
that parses, loads, and then quietly does the wrong thing at runtime. Every
rule here comes from a mistake made in this repository, so each one carries
the story of what it prevents rather than a bare rule number.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
QML_DIR = ROOT / "qml"

BINDING_START = re.compile(r"^\s*(readonly\s+)?property\s+\w+(<[^>]+>)?\s+\w+\s*:")
FUNCTION_START = re.compile(r"^\s*function\s+\w+\s*\(")


def qml_files():
    yield from sorted(QML_DIR.rglob("*.qml"))


def strip_comments(line):
    return re.sub(r"//.*$", "", line)


# Names main() puts into the QML root context. An id or a property that reuses
# one of these shadows it for the whole component.
CONTEXT_PROPERTIES = {
    "appSettings", "deviceModel", "transfer", "historyModel",
    "selection", "discovery", "receiver", "sender",
}

# Not anchored to the line start: `Column { id: sender }` on one line is just
# as much a shadow as the same thing spread over three.
ID_DECL = re.compile(r"(?<![\w.])id\s*:\s*(\w+)")


def check_shadowed_context_property(path, lines):
    """An id named after one of the app's context properties.

    The context property is still there, but the id wins inside the component
    that declares it, and nothing warns. ReceiveRequestPage gave its sender
    column `id: sender`, which shadowed the SendService for the whole page -
    harmless only because that page happened never to need it, and a bug
    waiting for the first line that did.
    """
    findings = []
    for number, raw in enumerate(lines, start=1):
        match = ID_DECL.search(strip_comments(raw))
        if match and match.group(1) in CONTEXT_PROPERTIES:
            findings.append((number, raw.strip()))
    return findings


ANCHOR_FILL = re.compile(r"^\s*anchors\s*\.\s*(fill|centerIn)\s*:")
ANCHOR_BLOCK = re.compile(r"^\s*anchors\s*\{")
GEOMETRY = re.compile(r"^\s*(x|y)\s*:")


def check_anchor_conflicts(path, lines):
    """An explicit x or y on an item that is also anchored into place.

    anchors.centerIn and anchors.fill set both coordinates. Setting one of
    them again does not offset the item, it fights the anchor: Qt keeps the
    anchor, drops the assignment, and logs nothing at all. Two chevrons drawn
    this way stacked on top of each other instead of forming an arrow.
    """
    findings = []
    depth = 0
    anchored_at = {}      # brace depth -> line number of the anchor

    for number, raw in enumerate(lines, start=1):
        line = strip_comments(raw)

        if ANCHOR_FILL.search(line):
            anchored_at[depth] = number
        elif GEOMETRY.search(line) and depth in anchored_at:
            findings.append((number, raw.strip()))

        opens = line.count("{")
        closes = line.count("}")
        # An anchor block spans lines; only track object scopes, and forget an
        # item's anchor once its own scope closes.
        if opens and not ANCHOR_BLOCK.search(line):
            depth += opens
        else:
            depth += opens
        depth -= closes
        for level in list(anchored_at):
            if level > depth:
                del anchored_at[level]

    return findings


ITEM_PROPERTIES = {
    "clip", "state", "states", "opacity", "visible", "enabled", "focus",
    "rotation", "scale", "smooth", "antialiasing", "parent", "children",
    "data", "transform", "x", "y", "z", "width", "height",
    "implicitWidth", "implicitHeight", "baselineOffset", "activeFocus",
}

PROPERTY_DECL = re.compile(
    r"^\s*(?:readonly\s+)?property\s+"
    r"(?:var|int|real|double|bool|string|url|color|list<[^>]+>|[A-Z]\w*)\s+(\w+)\s*[:;]?")


def check_shadowed_item_property(path, lines):
    """A property named like one Item already has.

    The declaration shadows the built-in, but only in the component's own root
    scope. Inside any nested Item a bare name resolves to that Item's own
    inherited property instead - silently, and with a plausible type.
    """
    findings = []
    for number, raw in enumerate(lines, start=1):
        match = PROPERTY_DECL.search(strip_comments(raw))
        if match and match.group(1) in ITEM_PROPERTIES:
            findings.append((number, raw.strip()))
    return findings


PLATFORM_TYPES = {
    "SectionHeader", "PageHeader", "Page", "Button", "Label", "TextField",
    "TextArea", "Slider", "Switch", "TextSwitch", "ComboBox", "ContextMenu",
    "MenuItem", "Dialog", "DialogHeader", "ViewPlaceholder", "SearchField",
    "IconButton", "BackgroundItem", "ListItem", "RemorseItem", "Separator",
    "SilicaListView", "SilicaGridView", "SilicaFlickable", "PullDownMenu",
    "PushUpMenu", "BusyIndicator", "GlassItem", "DockedPanel", "ProgressBar",
    "ValueButton", "Icon", "Theme", "Formatter", "Format", "DetailItem",
    "CoverBackground", "CoverAction", "CoverActionList", "Notification",
    "Item", "Rectangle", "Image", "Text", "Row", "Column", "Grid", "Flow",
    "Repeater", "Loader", "Timer", "Connections", "Component", "MouseArea",
    "Flickable", "ListView", "GridView", "ListModel", "Animation", "Gradient",
    "Binding", "Canvas", "QtObject",
}


def check_shadowed_platform_type(path, lines):
    """A component named after a type Silica or QtQuick already defines.

    The local one wins in every file that imports its directory, silently and
    at load time. A page importing "../components" for one delegate would get
    the local Label, ListItem or Button everywhere else too.
    """
    if path.parent.name != "components" or path.stem not in PLATFORM_TYPES:
        return []
    return [(1, path.stem + " is already a Silica or QtQuick type")]


def check_get_in_binding(path, lines):
    """A model's get() inside a property binding.

    The object get() returns is owned by the model, but a binding that reads
    it keeps a dependency guard on it. When the component is destroyed the
    binding's destructor can touch that wrapper after the model has released
    it, which aborts the process with "pure virtual method called". Call it
    from a function or a signal handler instead.
    """
    findings = []
    in_binding = False
    depth = 0
    binding_line = 0

    for number, raw in enumerate(lines, start=1):
        line = strip_comments(raw)

        if not in_binding:
            if BINDING_START.search(line) and not FUNCTION_START.search(line):
                in_binding = True
                binding_line = number
                depth = line.count("{") - line.count("}")
                if depth <= 0 and ".get(" not in line:
                    in_binding = False
                elif ".get(" in line:
                    findings.append((binding_line, raw.strip()))
                    in_binding = depth > 0
            continue

        if ".get(" in line:
            findings.append((binding_line, line.strip()))
        depth += line.count("{") - line.count("}")
        if depth <= 0:
            in_binding = False

    return findings


def check_qstr_in_library(path, lines):
    """qsTr() in a .pragma library script.

    A pragma library has no QML context and therefore no translation context.
    The call resolves to the untranslated source string in every locale, and
    lupdate files it under a context no .ts ever carries. Assemble translated
    sentences in QML and pass the pieces in.
    """
    text = "\n".join(lines)
    if ".pragma library" not in text:
        return []
    return [(number, raw.strip())
            for number, raw in enumerate(lines, start=1)
            if "qsTr(" in strip_comments(raw)]


QML_CHECKS = [
    ("id shadows a context property", check_shadowed_context_property),
    ("explicit x/y fights an anchor", check_anchor_conflicts),
    ("property shadows one Item already has", check_shadowed_item_property),
    ("component name shadows a platform type", check_shadowed_platform_type),
    ("model.get() inside a property binding", check_get_in_binding),
]

JS_CHECKS = [
    ("qsTr() in a pragma library", check_qstr_in_library),
]


def main():
    failures = 0
    checked = 0

    for path in qml_files():
        checked += 1
        lines = path.read_text(encoding="utf-8").splitlines()
        for title, check in QML_CHECKS:
            for number, snippet in check(path, lines):
                print(f"{path.relative_to(ROOT)}:{number}: {title}")
                print(f"    {snippet}")
                print(f"    {check.__doc__.strip().splitlines()[0]}")
                failures += 1

    for path in sorted(QML_DIR.rglob("*.js")):
        checked += 1
        lines = path.read_text(encoding="utf-8").splitlines()
        for title, check in JS_CHECKS:
            for number, snippet in check(path, lines):
                print(f"{path.relative_to(ROOT)}:{number}: {title}")
                print(f"    {snippet}")
                print(f"    {check.__doc__.strip().splitlines()[0]}")
                failures += 1

    print(f"\nchecked {checked} QML files, {failures} problem(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
