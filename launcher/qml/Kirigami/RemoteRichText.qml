// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * Markdown or HTML from the network -- a pack's description, a changelog --
 * shown as a read-only TextEdit that reads like a Label.
 *
 * Why not a Label (Text): such content links images from wherever its
 * author put them, and Text (Qt 6.11) crashes the launcher when one of them
 * fails (a 404, a refused connection, a YouTube page in an <img>) while
 * another is still loading. TextEdit loads them its own way and survives
 * that. The custom interface's RemoteRichText found this; this is the same
 * component in the KDE colours.
 *
 * Set `source`, not `text`: TextEdit's `text` reads back as the document
 * re-serialised (never empty for HTML), so test `source` for emptiness too.
 *
 * `article`: `source` is a whole page written by RichBody (C++,
 * models/RichBody.h) -- a pack's description -- and reads like one: headings
 * on Kirigami.Heading's scale, spaced paragraphs and lists, pictures as their
 * own blocks no wider than the text, quiet rules, links in the link colour.
 * The stylesheet below is built from Kirigami.Theme, so it follows a colour
 * scheme change.
 */
TextEdit {
    id: root

    property string source: ""
    // "markdown" or "html"
    property string format: "markdown"
    property bool article: false
    property bool loaded: false

    // Kirigami.Heading's own steps over the body size: level 1, 2 and 3,
    // and the minor headings at the body size, bold.
    readonly property real bodyPoints: Kirigami.Theme.defaultFont.pointSize > 0
                                       ? Kirigami.Theme.defaultFont.pointSize : 10
    readonly property real h1Points: Math.round(root.bodyPoints * 1.35 * 10) / 10
    readonly property real h2Points: Math.round(root.bodyPoints * 1.20 * 10) / 10
    readonly property real h3Points: Math.round(root.bodyPoints * 1.15 * 10) / 10

    // An opaque "#rrggbb" for the stylesheet (rich text reads no alpha):
    // translucent colours are laid over the background first.
    function cssColor(c) {
        return Qt.tint(Kirigami.Theme.backgroundColor, c).toString()
    }

    readonly property string articleStyle: {
        const px = (n) => Math.round(n) + "px"
        const small = Kirigami.Units.smallSpacing
        const large = Kirigami.Units.largeSpacing
        const heading = (tag, points, top) => tag + " { font-size: " + points
                + "pt; font-weight: 600; color: " + root.cssColor(Kirigami.Theme.textColor)
                + "; margin-top: " + px(top) + "; margin-bottom: " + px(small * 2) + "; }"
        return "<style>"
            + "p { margin-top: 0px; margin-bottom: " + px(large) + "; }"
            + "ul, ol { margin-top: 0px; margin-bottom: " + px(large) + "; }"
            + "li { margin-bottom: " + px(small) + "; }"
            + heading("h1", root.h1Points, Kirigami.Units.gridUnit)
            + heading("h2", root.h2Points, Kirigami.Units.gridUnit)
            + heading("h3", root.h3Points, large * 2)
            + heading("h4, h5, h6", root.bodyPoints, large * 2)
            + "a { color: " + root.cssColor(Kirigami.Theme.linkColor) + "; text-decoration: none; }"
            + "code, pre { font-family: '" + Kirigami.Theme.fixedWidthFont.family + "'; background-color: "
            + root.cssColor(Kirigami.Theme.alternateBackgroundColor) + "; }"
            + "pre { margin-top: 0px; margin-bottom: " + px(large) + "; }"
            + "blockquote { margin-left: " + px(large) + "; color: "
            + root.cssColor(Kirigami.Theme.disabledTextColor) + "; }"
            + "table.figure { margin-top: " + px(small * 2) + "; margin-bottom: " + px(large * 2) + "; }"
            // Less below a rule than above it: a heading after it brings
            // its own top margin.
            + "table.rule { margin-top: " + px(large) + "; margin-bottom: " + px(small * 2) + "; }"
            + "td.rule { font-size: 1px; background-color: "
            + root.cssColor(Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g,
                                    Kirigami.Theme.textColor.b, Kirigami.Theme.frameContrast)) + "; }"
            + "table.grid { margin-bottom: " + px(large) + "; border-color: "
            + root.cssColor(Qt.rgba(Kirigami.Theme.textColor.r, Kirigami.Theme.textColor.g,
                                    Kirigami.Theme.textColor.b, Kirigami.Theme.frameContrast)) + "; }"
            + "</style>"
    }

    readOnly: true
    selectByMouse: true
    activeFocusOnPress: false
    // Set by load(), with the text, so the two always change together.
    textFormat: TextEdit.MarkdownText
    wrapMode: TextEdit.Wrap
    color: Kirigami.Theme.textColor
    selectedTextColor: Kirigami.Theme.highlightedTextColor
    selectionColor: Kirigami.Theme.highlightColor
    font: Kirigami.Theme.defaultFont

    function load() {
        root.text = ""
        root.textFormat = root.format === "html" ? TextEdit.RichText : TextEdit.MarkdownText
        // linkColor does not reach RichText; a style rule does.
        const style = root.article ? root.articleStyle
                    : "<style>a { color: " + root.cssColor(Kirigami.Theme.linkColor) + "; text-decoration: none; }</style>"
        root.text = root.format === "html" && root.source.length > 0
                    ? style + root.source
                    : root.source
    }

    onSourceChanged: if (root.loaded) root.load()
    onFormatChanged: if (root.loaded) root.load()
    onArticleChanged: if (root.loaded) root.load()
    // A colour scheme change: the stylesheet's colours are baked into the text.
    onArticleStyleChanged: if (root.loaded && root.article) root.load()
    Component.onCompleted: {
        root.loaded = true
        root.load()
    }

    onLinkActivated: (link) => Qt.openUrlExternally(link)

    // A link is something to click: say so with the cursor.
    HoverHandler {
        cursorShape: root.hoveredLink.length > 0 ? Qt.PointingHandCursor : Qt.IBeamCursor
    }
}
