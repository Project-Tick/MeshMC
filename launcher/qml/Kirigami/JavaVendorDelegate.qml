// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * Which vendor automatic Java downloads come from (JavaAutoDownloadVendor).
 * Hidden in a build without the Java downloader.
 */
SettingChoice {
    id: delegate

    // JavaController: vendors ([{value, label}]), downloaderAvailable.
    property var java: null

    key: "JavaAutoDownloadVendor"
    text: i18nc("@label:listbox", "Download from")
    visible: !!delegate.java && delegate.java.downloaderAvailable
    options: delegate.java ? delegate.java.vendors : []
}
