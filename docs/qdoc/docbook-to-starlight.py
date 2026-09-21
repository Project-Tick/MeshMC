#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Project Tick
# SPDX-FileContributor: Project Tick
# SPDX-License-Identifier: Apache-2.0
#
# Turn QDoc's DocBook output into Markdown for the Astro/Starlight site.
#
#   ./docs/qdoc/docbook-to-starlight.py <docbook-dir> <output-dir>
#
# Why DocBook and not QDoc's HTML: the HTML carries QDoc's page furniture --
# navigation bars, breadcrumb markup, a stylesheet reference -- and turning it
# back into Markdown means parsing a layout to recover the meaning that was
# already there before it was laid out. The DocBook generator emits the
# meaning itself: db:abstract for the brief, db:section for a section,
# db:programlisting with a language attribute for code, db:link with the
# target already resolved (Qt classes come out pointing at doc.qt.io). The
# mapping below is therefore mostly one line per element.
#
# Why Python and not Node, next to a site that is Node: xml.etree is in the
# standard library, so this runs with nothing installed. The site has no XML
# parser among its dependencies, so a Node port would mean adding one -- easy
# to do later if this needs to move into the site's own build.

import os
import re
import sys
import xml.etree.ElementTree as ET

DB = "{http://docbook.org/ns/docbook}"
XLINK = "{http://www.w3.org/1999/xlink}"

# Tags met but not given a rule, collected so that a new QDoc release adding
# something is a line in the report and not a silently dropped paragraph.
unhandled = {}


def tag(element):
    return element.tag.replace(DB, "")


def text_of(element):
    """The element's text with all markup flattened away."""
    return "".join(element.itertext())


def escape(text):
    """Escape what Markdown would otherwise read as markup.

    Deliberately narrow: escaping everything that could theoretically be a
    Markdown token turns ordinary prose into a hedge of backslashes. Angle
    brackets are the ones that actually occur here -- '<' and '>' come up in
    C++ types such as QList<int> and would otherwise be read as HTML -- and a
    brace is escaped because Starlight compiles Markdown through MDX-aware
    tooling, where a bare '{' starts an expression.

    Backslashes are left as they are, on purpose. CommonMark treats a
    backslash before anything that is not punctuation as a literal
    backslash, so the QDoc command names that turn up in prose come out as
    written; doubling them would only make the source unreadable to arrive at
    the same rendering.
    """
    return (
        text.replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace("{", "&#123;")
    )


def inline(element, in_code=False):
    """Render an element's children as inline Markdown."""
    out = []

    if element.text:
        out.append(element.text if in_code else escape(element.text))

    for child in element:
        name = tag(child)

        if name in ("code", "literal"):
            # Inline code: no escaping inside, backticks are the fence.
            out.append("`" + text_of(child) + "`")
        elif name == "emphasis":
            body = inline(child)
            out.append(f"**{body}**" if child.get("role") == "bold" else f"*{body}*")
        elif name == "link":
            href = child.get(XLINK + "href", "")
            out.append(f"[{inline(child)}]({href})" if href else inline(child))
        elif name == "phrase":
            # QDoc uses db:phrase for text it has no better element for, such
            # as the name of a QDoc command written in prose. Plain text.
            out.append(escape(text_of(child)))
        elif name in ("superscript", "subscript"):
            out.append(f"<{name}>{escape(text_of(child))}</{name}>")
        else:
            unhandled[name] = unhandled.get(name, 0) + 1
            out.append(inline(child))

        if child.tail:
            out.append(child.tail if in_code else escape(child.tail))

    return "".join(out)


def block(element, depth=2):
    """Render an element as one or more Markdown blocks."""
    name = tag(element)
    out = []

    if name == "para":
        out.append(inline(element).strip())

    elif name == "programlisting":
        language = element.get("language", "")
        body = text_of(element).rstrip("\n")
        out.append(f"```{language}\n{body}\n```")

    elif name in ("note", "warning", "tip", "important", "caution"):
        # Starlight's asides. "warning" and "important" have no aside of their
        # own; caution is the closest in tone and is what the site's own wiki
        # pages already use.
        kind = {"note": "note", "tip": "tip"}.get(name, "caution")
        inner = "\n\n".join(b for b in (block(c, depth) for c in element) if b)
        out.append(f":::{kind}\n{inner}\n:::")

    elif name == "section":
        title = element.find(DB + "title")
        if title is not None:
            out.append("#" * min(depth, 6) + " " + inline(title).strip())
        for child in element:
            if child is title:
                continue
            rendered = block(child, depth + 1)
            if rendered:
                out.append(rendered)

    elif name in ("itemizedlist", "orderedlist"):
        for index, item in enumerate(element.findall(DB + "listitem"), start=1):
            marker = f"{index}." if name == "orderedlist" else "-"
            inner = "\n\n".join(b for b in (block(c, depth) for c in item) if b)
            # Continuation lines are indented so the list item keeps them.
            indented = inner.replace("\n", "\n" + " " * (len(marker) + 1))
            out.append(f"{marker} {indented}")

    elif name == "variablelist":
        # QDoc puts a class's Header/Inherits/Since facts in one of these. A
        # two-column table reads better on a web page than a definition list
        # rendered as prose.
        rows = []
        for entry in element.findall(DB + "varlistentry"):
            term = entry.find(DB + "term")
            item = entry.find(DB + "listitem")
            key = inline(term).strip() if term is not None else ""
            value = " ".join(
                inline(p).strip() for p in (item.findall(DB + "para") if item is not None else [])
            )
            rows.append((key, value.replace("|", "\\|")))
        if rows:
            out.append(
                "| | |\n|---|---|\n"
                + "\n".join(f"| {k} | {v} |" for k, v in rows)
            )

    elif name in ("methodsynopsis", "fieldsynopsis", "constructorsynopsis",
                  "destructorsynopsis", "classsynopsis"):
        # A signature. Kept as code rather than prose so it stays readable
        # and copyable.
        out.append("```cpp\n" + " ".join(text_of(element).split()) + "\n```")

    elif name in ("bridgehead", "title"):
        out.append("#" * min(depth, 6) + " " + inline(element).strip())

    elif name in ("info", "index", "anchor"):
        pass  # handled by the caller, or nothing to show

    else:
        unhandled[name] = unhandled.get(name, 0) + 1
        for child in element:
            rendered = block(child, depth)
            if rendered:
                out.append(rendered)

    return "\n\n".join(b for b in out if b)


def frontmatter_value(text):
    """A YAML double-quoted scalar, which is the only quoting rule needed."""
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def convert(path):
    root = ET.parse(path).getroot()
    info = root.find(DB + "info")

    title = ""
    description = ""
    if info is not None:
        node = info.find(DB + "title")
        if node is not None:
            title = text_of(node).strip()
        abstract = info.find(DB + "abstract")
        if abstract is not None:
            description = " ".join(text_of(abstract).split())

    body = []
    for child in root:
        if tag(child) == "info":
            continue
        rendered = block(child)
        if rendered:
            body.append(rendered)

    lines = ["---", f"title: {frontmatter_value(title)}"]
    if description:
        lines.append(f"description: {frontmatter_value(description)}")
    # An HTML comment and not MDX's {/* ... */}: these are .md files, where
    # a brace expression is not compiled away but printed to the reader.
    lines += [
        "---",
        "",
        "<!-- Generated from MeshMC's own sources by QDoc; see docs/qdoc/.",
        "     Edits made here are lost on the next run. -->",
        "",
        "",
    ]
    return "\n".join(lines) + "\n\n".join(body) + "\n"


def main():
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} <docbook-dir> <output-dir>")

    source, destination = sys.argv[1], sys.argv[2]
    if not os.path.isdir(source):
        sys.exit(f"{source} is not a directory; run build-docs.sh with "
                 f"--outputformat DocBook first")
    os.makedirs(destination, exist_ok=True)

    written = 0
    for name in sorted(os.listdir(source)):
        if not name.endswith(".xml"):
            continue
        target = os.path.join(destination, name[:-4] + ".md")
        with open(target, "w", encoding="utf-8") as handle:
            handle.write(convert(os.path.join(source, name)))
        print(f"  {name} -> {os.path.relpath(target)}")
        written += 1

    print(f"\n{written} page(s) written to {destination}")
    if unhandled:
        print("\nDocBook elements with no rule of their own (their children were "
              "still rendered):")
        for name, count in sorted(unhandled.items(), key=lambda kv: -kv[1]):
            print(f"  {name}: {count}")


if __name__ == "__main__":
    main()
