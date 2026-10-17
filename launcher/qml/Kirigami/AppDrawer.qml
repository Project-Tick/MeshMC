// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

/*
 * The sidebar: the app's sections, then the instances played most recently.
 *
 * Beside the pages on a wide window, where its own button at the bottom
 * folds it to icons (remembered in the UiSidebarCollapsed setting); a modal
 * drawer behind the toolbar's menu button on a narrow one, closing once a
 * section is picked.
 */
Kirigami.GlobalDrawer {
    id: drawer

    required property var app

    // How many recently played instances the sidebar lists.
    readonly property int recentLimit: 5

    readonly property var settings: drawer.app.shell && drawer.app.shell.settings ? drawer.app.shell.settings : null

    modal: !drawer.app.wideLayout
    collapsible: !drawer.modal
    onModalChanged: drawerOpen = !modal

    // The folded state, as it was last left. Read once the window exists,
    // since the drawer's own button changes `collapsed` without a binding.
    function restoreState() {
        drawer.drawerOpen = !drawer.modal
        if (!drawer.settings)
            return
        // The settings file hands a bool back as text.
        var stored = drawer.settings.value("UiSidebarCollapsed")
        drawer.collapsed = stored === true || stored === "true"
    }
    onCollapsedChanged: {
        if (drawer.settings && drawer.collapsible)
            drawer.settings.setValue("UiSidebarCollapsed", drawer.collapsed)
    }

    function go(section) {
        drawer.app.showSection(section)
        if (drawer.modal)
            drawer.close()
    }

    component SectionAction: Kirigami.Action {
        required property string section
        checkable: true
        checked: drawer.app.section === section
        onTriggered: {
            // A checkable action unchecks itself on a second click.
            checked = Qt.binding(() => drawer.app.section === section)
            drawer.go(section)
        }
    }

    // The app's identity above the sections, as in a stock KDE application:
    // its logo and name, with the version dimmed under it. Kirigami hides the
    // header of a folded drawer unless told otherwise, and then drops the
    // name, which leaves the logo alone. It is centred on the same axis as
    // the sections' icons below, expanded or folded, so the margin is what
    // a folded drawer leaves around a logo-sized icon.
    showHeaderWhenCollapsed: true
    readonly property int logoSize: Kirigami.Units.iconSizes.medium
    header: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        Image {
            Layout.leftMargin: Math.max(0, (drawer.collapsedSize - drawer.logoSize) / 2)
            Layout.topMargin: Kirigami.Units.largeSpacing
            Layout.bottomMargin: Kirigami.Units.largeSpacing
            Layout.preferredWidth: drawer.logoSize
            Layout.preferredHeight: drawer.logoSize
            source: "qrc:/org.projecttick.MeshMC.svg"
            sourceSize: Qt.size(drawer.logoSize * 2, drawer.logoSize * 2)
            fillMode: Image.PreserveAspectFit
            smooth: true
            Accessible.ignored: true
        }
        ColumnLayout {
            visible: !drawer.collapsed
            spacing: 0
            Layout.fillWidth: true
            Layout.minimumWidth: 0

            Kirigami.Heading {
                Layout.fillWidth: true
                level: 3
                text: i18nc("@title", "MeshMC")
                elide: Text.ElideRight
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: text.length > 0
                text: Qt.application.version
                elide: Text.ElideRight
                font: Kirigami.Theme.smallFont
                opacity: 0.7
            }
        }
    }

    // The recently played instances, as drawer actions under "Recent".
    Instantiator {
        id: recentInstances
        model: drawer.app.shell && drawer.app.shell.recentModel ? drawer.app.shell.recentModel : null
        delegate: Kirigami.Action {
            required property int index
            required property string instanceId
            required property string name
            required property string iconKey
            text: name
            icon.source: "image://instanceicon/" + (iconKey.length > 0 ? iconKey : "default")
            checkable: true
            checked: drawer.app.openedInstanceId === instanceId
            onTriggered: {
                checked = Qt.binding(() => drawer.app.openedInstanceId === instanceId)
                drawer.app.openInstance(instanceId)
                if (drawer.modal)
                    drawer.close()
            }
        }
        onObjectAdded: recentAction.refresh()
        onObjectRemoved: recentAction.refresh()
    }

    // Breeze's monochrome -symbolic icons, as stock KDE applications use in
    // their sidebars: several of the plain names have a colour version at
    // 32px and up (applications-games, help-about), which a HiDPI sidebar
    // would get.
    actions: [
        SectionAction {
            section: "home"
            text: i18nc("@action:inmenu", "Home")
            icon.name: "go-home-symbolic"
        },
        SectionAction {
            section: "library"
            text: i18nc("@action:inmenu", "Library")
            icon.name: "applications-games-symbolic"
        },
        SectionAction {
            section: "discover"
            text: i18nc("@action:inmenu", "Discover")
            icon.name: "compass-symbolic"
        },
        SectionAction {
            section: "accounts"
            text: i18nc("@action:inmenu", "Accounts")
            icon.name: "user-identity-symbolic"
        },
        // A window of its own (SettingsView.qml), not a section.
        Kirigami.Action {
            text: i18nc("@action:inmenu", "Settings")
            icon.name: "configure-symbolic"
            onTriggered: {
                drawer.app.openSettings("")
                if (drawer.modal)
                    drawer.close()
            }
        },
        SectionAction {
            section: "about"
            text: i18nc("@action:inmenu", "About MeshMC")
            icon.name: "help-about-symbolic"
        },
        Kirigami.Action {
            id: recentAction
            text: i18nc("@title:group", "Recently Played")
            icon.name: "document-open-recent-symbolic"
            expandible: true
            visible: children.length > 0
            function refresh() {
                var actions = []
                for (var i = 0; i < Math.min(recentInstances.count, drawer.recentLimit); ++i) {
                    var action = recentInstances.objectAt(i)
                    if (action)
                        actions.push(action)
                }
                recentAction.children = actions
            }
        }
    ]
}
