#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: Apache-2.0
"""
Turns the translatable strings of Qt Designer .ui files into i18n() calls that
xgettext can read.

KDE ships `extractrc` for this, but it is not part of KI18n and is not
installed on most machines that build MeshMC, so update.sh uses this small
stand-in instead. It is intentionally narrow: it only has to agree with what
`uic -tr tr2i18n` compiles into the generated ui_*.h files, because those
calls are what look the strings up at runtime. A string that is extracted here
but not looked up (or the other way round) is a dead or an untranslated string.

The rules, mirroring uic:
  * every <string> is translatable unless it carries notr="true";
  * empty strings are skipped (tr2i18n("") returns QString());
  * comment="..."      is the disambiguation, i.e. the context -> i18nc();
  * extracomment="..." is a hint for translators -> "i18n:" comment;
  * strings of a few non-text properties (see SKIP_PROPERTIES) are skipped.

Output goes to stdout. `#line` directives keep the original .ui path and line
number so the generated .pot points at the .ui file and not at this stream.

Usage: extract-ui-strings.py <repo-root> <file.ui>...
"""

import os
import sys
import xml.parsers.expat

# Properties uic never routes through a translation call.
SKIP_PROPERTIES = {"objectName", "styleSheet", "buddy"}


def c_string(text):
    out = []
    for ch in text:
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


class UiParser:
    def __init__(self, label):
        self.label = label
        self.parser = xml.parsers.expat.ParserCreate()
        self.parser.buffer_text = True
        self.parser.StartElementHandler = self.start
        self.parser.EndElementHandler = self.end
        self.parser.CharacterDataHandler = self.chars
        self.property_stack = []
        self.current = None
        self.entries = []

    def start(self, name, attrs):
        if name == "property" or name == "attribute":
            self.property_stack.append(attrs.get("name", ""))
        elif name == "string":
            self.current = {
                "line": self.parser.CurrentLineNumber,
                "attrs": attrs,
                "text": [],
                "property": self.property_stack[-1] if self.property_stack else "",
            }

    def chars(self, data):
        if self.current is not None:
            self.current["text"].append(data)

    def end(self, name):
        if name == "property" or name == "attribute":
            self.property_stack.pop()
        elif name == "string" and self.current is not None:
            cur, self.current = self.current, None
            attrs = cur["attrs"]
            text = "".join(cur["text"])
            if attrs.get("notr", "").lower() == "true":
                return
            if not text or cur["property"] in SKIP_PROPERTIES:
                return
            self.entries.append(
                (cur["line"], attrs.get("comment", ""), attrs.get("extracomment", ""), text)
            )

    def parse(self, data):
        self.parser.Parse(data, True)
        return self.entries


def main(argv):
    if len(argv) < 3:
        sys.stderr.write(__doc__)
        return 2

    root = os.path.abspath(argv[1])
    out = sys.stdout
    for path in argv[2:]:
        label = os.path.relpath(os.path.abspath(path), root).replace(os.sep, "/")
        with open(path, "rb") as handle:
            entries = UiParser(label).parse(handle.read())
        for line, comment, extracomment, text in entries:
            out.write('#line %d "%s"\n' % (line, label))
            if extracomment:
                # One line, so xgettext's --add-comments=i18n picks it up.
                out.write("// i18n: %s\n" % " ".join(extracomment.split()))
            if comment:
                out.write("i18nc(%s, %s);\n" % (c_string(comment), c_string(text)))
            else:
                out.write("i18n(%s);\n" % c_string(text))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
