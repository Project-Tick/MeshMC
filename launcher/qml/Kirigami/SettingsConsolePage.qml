// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * Console: what the game's log window does while you play, the font the
 * log is drawn in (ConsoleFont, ConsoleFontSize: the instance page's Log
 * tab reads them too), and how much of the log is kept.
 */
SettingsPage {
    id: page

    // Installed families, the stored one first even when it is no longer
    // installed, so a setting from another computer is not lost.
    readonly property string fontFamily: page.store.string("ConsoleFont")
    readonly property var families: {
        var list = Qt.fontFamilies()
        if (page.fontFamily.length > 0 && list.indexOf(page.fontFamily) < 0)
            list = [page.fontFamily].concat(list)
        return list
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Game Console")
    }
    ConsoleCard {
        source: page.store
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Font")
    }
    FormCard.FormCard {
        FormCard.FormComboBoxDelegate {
            id: familyBox
            text: i18nc("@label:listbox", "Font")
            description: i18n("Used for the game's log. A monospaced font lines columns up best.")
            model: page.families
            function sync() { currentIndex = page.families.indexOf(page.fontFamily) }
            Connections {
                target: page
                function onFontFamilyChanged() { familyBox.sync() }
                function onFamiliesChanged() { familyBox.sync() }
            }
            Component.onCompleted: sync()
            onActivated: {
                page.store.setValue("ConsoleFont", currentText)
                sync()
            }
        }
        FormCard.FormDelegateSeparator {}
        SettingNumber {
            source: page.store
            key: "ConsoleFontSize"
            label: i18nc("@label:spinbox", "Font size")
            from: 6
            to: 48
            suffix: i18nc("@item:spinbox unit", "pt")
        }
        FormCard.FormDelegateSeparator {}
        FormCard.AbstractFormDelegate {
            background: null
            focusPolicy: Qt.NoFocus
            // What the game itself writes, so not translated.
            contentItem: QQC2.Label {
                text: "[12:00:01] [Render thread/INFO]: Setting user: Steve\n[12:00:02] [Render thread/WARN]: Missing sound for event"
                textFormat: Text.PlainText
                elide: Text.ElideRight
                font.family: page.fontFamily.length > 0 ? page.fontFamily : Kirigami.Theme.fixedWidthFont.family
                font.pointSize: Math.max(6, page.store.number("ConsoleFontSize"))
                Accessible.name: i18n("Font preview")
            }
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Log History")
    }
    FormCard.FormCard {
        SettingNumber {
            source: page.store
            key: "ConsoleMaxLines"
            label: i18nc("@label:spinbox", "Lines to keep")
            from: 10000
            to: 1000000
            stepSize: 10000
        }
        FormCard.FormDelegateSeparator {}
        SettingSwitch {
            source: page.store
            key: "ConsoleOverflowStop"
            text: i18nc("@option:check", "Stop logging when the limit is reached")
            description: i18n("Otherwise the oldest lines are dropped to make room.")
        }
    }
}
