// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * First run's Java step: automatic Java downloads, the most memory a game
 * gets, and the Java installs found on this computer, one of which can be
 * picked. The shell's detection (detectJava()) runs when the flow opens;
 * picking one writes JavaPath as the classic setup wizard did (useJava()).
 */
ColumnLayout {
    id: step

    required property var shell
    readonly property var installs: step.shell ? step.shell.javaInstalls : null
    readonly property bool detecting: !!step.shell && !!step.shell.javaDetecting

    spacing: 0

    SettingsSource {
        id: store
        adapter: step.shell && step.shell.settings ? step.shell.settings : null
    }

    FormCard.FormCard {
        SettingSwitch {
            id: autoDownload
            source: store
            key: "JavaAutoDownload"
            text: i18nc("@option:check", "Download Java automatically")
            description: i18n("Recommended. Each Minecraft version gets the Java it needs.")
        }
        FormCard.FormDelegateSeparator {}
        MaxMemoryDelegate {
            source: store
            systemMiB: step.shell && step.shell.systemMemoryMiB ? step.shell.systemMemoryMiB : 8192
        }
    }

    FormCard.FormHeader {
        title: i18nc("@title:group", "Java on This Computer")
        actions: [
            Kirigami.Action {
                text: i18nc("@action:button", "Detect Again")
                icon.name: "view-refresh"
                enabled: !!step.shell && !step.detecting
                onTriggered: step.shell.detectJava()
            }
        ]
    }
    FormCard.FormCard {
        Repeater {
            id: found
            model: step.installs
            delegate: FormCard.FormRadioDelegate {
                id: javaRow
                required property string path
                required property var version
                required property var architecture
                required property bool recommended
                text: recommended ? i18nc("@option:radio %1 Java version", "Java %1 (recommended)", String(version))
                                  : i18nc("@option:radio %1 Java version", "Java %1", String(version))
                description: architecture ? i18nc("@info %1 path, %2 architecture", "%1 · %2", path, String(architecture)) : path
                checked: store.string("JavaPath") === path
                onToggled: {
                    if (!checked)
                        return
                    step.shell.useJava(path)
                    checked = Qt.binding(() => store.string("JavaPath") === javaRow.path)
                }
            }
        }
        FormCard.AbstractFormDelegate {
            visible: step.detecting
            background: null
            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                QQC2.BusyIndicator {
                    running: step.detecting
                    implicitWidth: Kirigami.Units.iconSizes.medium
                    implicitHeight: Kirigami.Units.iconSizes.medium
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: i18n("Looking for Java…")
                }
            }
        }
        FormCard.FormTextDelegate {
            visible: !step.detecting && found.count === 0
            text: i18n("No Java found")
            description: autoDownload.checked
                         ? i18n("That's fine: MeshMC downloads the right one the first time you play.")
                         : i18n("Turn on automatic downloads above, or install Java and detect again.")
        }
    }
}
