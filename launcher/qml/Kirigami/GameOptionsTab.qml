// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * The instance's options.txt. The options a player recognises come first,
 * grouped the way the game's own Options screen groups them, in the game's
 * terms ("Render distance: 12 chunks", not "renderDistance 12") and with
 * the same kind of control (GameOptionDelegate). A change is written to
 * options.txt at once and applies the next time the game starts; while it
 * runs everything is read-only, since the game rewrites the file as it
 * quits. Every raw key is still there, searchable, under "All Options".
 */
FormTab {
    id: tab

    required property var app
    property var details: null
    // Whether this instance's game is running (InstancePage.running).
    property bool running: false

    // KeyValueFilterModel over the instance's GameOptions.
    readonly property var model: tab.details ? tab.details.gameOptions : null
    readonly property var store: tab.model ? tab.model.sourceModel : null
    readonly property bool editable: !tab.running && collector.count > 0
    property bool showAll: false

    // key -> raw value, read from the unfiltered GameOptions underneath the
    // search proxy, so the groups never change while searching.
    property var values: ({})
    // The keys there are, one per line. A string only notifies when it
    // really changes, so the rows are rebuilt when a key appears or goes,
    // never while a value is being edited.
    property string keyList: ""
    function collect() {
        const map = {}
        for (let i = 0; i < collector.count; ++i) {
            const o = collector.objectAt(i)
            if (o)
                map[o.key] = o.value
        }
        tab.values = map
        tab.keyList = Object.keys(map).join("\n")
    }
    Instantiator {
        id: collector
        model: tab.store
        delegate: QtObject {
            required property string key
            required property string value
            onValueChanged: Qt.callLater(tab.collect)
        }
        onObjectAdded: Qt.callLater(tab.collect)
        onObjectRemoved: Qt.callLater(tab.collect)
    }

    function has(key) {
        return ("\n" + tab.keyList + "\n").indexOf("\n" + key + "\n") >= 0
    }
    // options.txt writes its decimals the way the game does: "1.0", not "1".
    function decimal(v) {
        return Number.isInteger(v) ? v.toFixed(1) : String(v)
    }
    // "en_us" -> "English (United States)", in that language's own words.
    function language() {
        const parts = String(tab.values.lang).split("_")
        const locale = Qt.locale(parts[0] + (parts.length > 1 ? "_" + parts[1].toUpperCase() : ""))
        if (locale.name === "C")
            return tab.values.lang
        return parts.length > 1 ? i18n("%1 (%2)", locale.nativeLanguageName, locale.nativeTerritoryName)
                                : locale.nativeLanguageName
    }
    // ["vanilla","fabric","file/Fresh Animations.zip"] -> "Fresh Animations"
    function resourcePacks() {
        let list = []
        try {
            list = JSON.parse(tab.values.resourcePacks)
        } catch (e) {
            return tab.values.resourcePacks
        }
        const named = list.filter(p => p !== "vanilla" && p !== "fabric")
                          .map(p => String(p).replace(/^file\//, "").replace(/\.zip$/i, ""))
        return named.length > 0 ? named.join(", ") : i18n("None")
    }

    // Row specs for GameOptionDelegate (its header lists the fields).
    function chunks(label, key, from) {
        return { label: label, key: key, kind: "slider", from: from, to: 32, step: 1,
                 toSlider: raw => Number(raw), toRaw: v => String(v),
                 text: v => i18np("%1 chunk", "%1 chunks", v) }
    }
    // A 0..1 value shown as 0..scale.
    function fraction(label, key, scale, text) {
        return { label: label, key: key, kind: "slider", from: 0, to: scale, step: 1,
                 toSlider: raw => Math.round(Number(raw) * scale),
                 toRaw: v => tab.decimal(v / scale), text: text }
    }
    function volume(label, key) {
        return tab.fraction(label, key, 100, v => v === 0 ? i18n("Off") : i18nc("@item:valuesuffix percent", "%1%", v))
    }
    function toggle(label, key) {
        return { label: label, key: key, kind: "switch" }
    }
    function info(label, key, text) {
        return { label: label, key: key, kind: "text", text: text }
    }

    // The game's own stops: options.txt stores the field of view as
    // (fov - 70) / 40, and 260 is the framerate slider's "Unlimited".
    readonly property int normalFov: 70
    readonly property int fovSpan: 40
    readonly property int unlimitedFps: 260
    readonly property int maxGuiScale: 6

    // { title, rows: [spec] } -- a row only shows when its key is in this
    // options.txt (fancyGraphics is what versions before 1.16 wrote instead
    // of graphicsMode).
    readonly property var optionGroups: [
        { title: i18nc("@title:group", "Video"), rows: [
            tab.chunks(i18n("Render distance"), "renderDistance", 2),
            tab.chunks(i18n("Simulation distance"), "simulationDistance", 5),
            { label: i18n("Field of view"), key: "fov", kind: "slider",
              from: tab.normalFov - tab.fovSpan, to: tab.normalFov + tab.fovSpan, step: 1,
              toSlider: raw => Math.round(tab.normalFov + Number(raw) * tab.fovSpan),
              toRaw: deg => tab.decimal((deg - tab.normalFov) / tab.fovSpan),
              text: deg => deg === tab.normalFov ? i18n("Normal")
                           : deg === tab.normalFov + tab.fovSpan ? i18n("Quake Pro") : String(deg) },
            { label: i18n("Max framerate"), key: "maxFps", kind: "slider", from: 10, to: tab.unlimitedFps, step: 10,
              toSlider: raw => Number(raw), toRaw: v => String(v),
              text: v => v >= tab.unlimitedFps ? i18n("Unlimited") : i18n("%1 fps", v) },
            { label: i18n("Graphics"), key: "graphicsMode", kind: "choice", choices: [
                { value: "0", label: i18n("Fast") }, { value: "1", label: i18n("Fancy") },
                { value: "2", label: i18n("Fabulous!") }] },
            { label: i18n("Graphics"), key: "fancyGraphics", kind: "choice", choices: [
                { value: "false", label: i18n("Fast") }, { value: "true", label: i18n("Fancy") }] },
            tab.fraction(i18n("Brightness"), "gamma", 100,
                         v => v === 0 ? i18n("Moody") : v === 100 ? i18n("Bright") : i18nc("@item:valuesuffix percent", "%1%", v)),
            { label: i18n("GUI scale"), key: "guiScale", kind: "choice",
              choices: [{ value: "0", label: i18n("Auto") }].concat(
                  Array.from({ length: tab.maxGuiScale }, (_, i) => ({ value: String(i + 1), label: String(i + 1) }))) },
            tab.toggle(i18n("VSync"), "enableVsync"),
            tab.toggle(i18n("Fullscreen"), "fullscreen")
        ] },
        { title: i18nc("@title:group", "Music and Sounds"), rows: [
            tab.volume(i18n("Master volume"), "soundCategory_master"),
            tab.volume(i18n("Music"), "soundCategory_music"),
            tab.volume(i18n("Jukebox and note blocks"), "soundCategory_record"),
            tab.volume(i18n("Weather"), "soundCategory_weather"),
            tab.volume(i18n("Blocks"), "soundCategory_block"),
            tab.volume(i18n("Hostile creatures"), "soundCategory_hostile"),
            tab.volume(i18n("Friendly creatures"), "soundCategory_neutral"),
            tab.volume(i18n("Players"), "soundCategory_player"),
            tab.volume(i18n("Ambient"), "soundCategory_ambient"),
            tab.volume(i18n("Voice"), "soundCategory_voice")
        ] },
        { title: i18nc("@title:group", "Controls"), rows: [
            // 0.5 is the game's 100%.
            tab.fraction(i18n("Mouse sensitivity"), "mouseSensitivity", 200, v => i18nc("@item:valuesuffix percent", "%1%", v)),
            tab.toggle(i18n("Auto-jump"), "autoJump")
        ] },
        // Read-only: the language list is the game's own, and packs are
        // turned on and off on the Content tab.
        { title: i18nc("@title:group", "Language and Packs"), rows: [
            tab.info(i18n("Language"), "lang", tab.language),
            tab.info(i18n("Resource packs"), "resourcePacks", tab.resourcePacks)
        ] }
    ]

    Kirigami.InlineMessage {
        Layout.fillWidth: true
        Layout.maximumWidth: Kirigami.Units.gridUnit * 30
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: Kirigami.Units.largeSpacing
        visible: collector.count === 0 || tab.running
        type: Kirigami.MessageType.Information
        text: collector.count === 0
              ? i18n("This instance has no options yet. Play it once and the game creates them.")
              : i18n("The game is running. These can be changed again once it closes.")
    }

    Repeater {
        model: tab.optionGroups
        delegate: ColumnLayout {
            id: group
            required property var modelData
            readonly property var shownRows: modelData.rows.filter(r => tab.has(r.key))

            Layout.fillWidth: true
            spacing: 0
            visible: group.shownRows.length > 0

            FormCard.FormHeader {
                title: group.modelData.title
            }
            FormCard.FormCard {
                Repeater {
                    model: group.shownRows
                    delegate: GameOptionDelegate {
                        required property var modelData
                        spec: modelData
                        raw: tab.values[modelData.key] !== undefined ? tab.values[modelData.key] : ""
                        store: tab.store
                        editable: tab.editable
                    }
                }
            }
        }
    }

    // Every key, raw, for anything the groups above leave out.
    FormCard.FormHeader {
        visible: collector.count > 0
        title: i18nc("@title:group", "All Options")
    }
    FormCard.FormCard {
        Layout.bottomMargin: Kirigami.Units.gridUnit
        visible: collector.count > 0

        FormCard.FormSwitchDelegate {
            text: i18n("Show all %1 options", collector.count)
            description: i18n("Every key of options.txt, as the game wrote it.")
            checked: tab.showAll
            onToggled: tab.showAll = checked
        }
        Kirigami.SearchField {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            visible: tab.showAll
            placeholderText: i18nc("@info:placeholder", "Search options…")
            onTextChanged: if (tab.model) tab.model.filterText = text
        }
        Repeater {
            id: rawRows
            model: tab.showAll ? tab.model : null
            delegate: FormCard.AbstractFormDelegate {
                id: rawRow
                required property string key
                required property string value
                background: null
                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    QQC2.Label {
                        Layout.preferredWidth: 1
                        Layout.fillWidth: true
                        text: rawRow.key
                        elide: Text.ElideRight
                        font: Kirigami.Theme.fixedWidthFont
                    }
                    QQC2.Label {
                        Layout.preferredWidth: 1
                        Layout.fillWidth: true
                        text: rawRow.value
                        elide: Text.ElideRight
                        color: Kirigami.Theme.disabledTextColor
                        font: Kirigami.Theme.fixedWidthFont
                    }
                }
            }
        }
        FormCard.FormTextDelegate {
            visible: tab.showAll && rawRows.count === 0
            text: tab.model ? i18n("Nothing matches “%1”.", tab.model.filterText) : ""
        }
    }
}
