// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * The JVM arguments. Memory has settings of its own, so arguments that set
 * it by hand (-Xmx, -Xms, PermSize) are called out under the field while
 * they are typed.
 */
SettingText {
    id: delegate

    // JavaController, for argsWarning(); without it, no warning.
    property var java: null

    key: "JvmArgs"
    label: i18nc("@label:textbox", "JVM arguments")
    placeholderText: "-XX:+UseG1GC"
    status: Kirigami.MessageType.Warning
    statusMessage: delegate.java ? delegate.java.argsWarning(delegate.text) : ""
}
