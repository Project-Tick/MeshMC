// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MeshMC.Theme

/*
 * Every component, in every state it can be in, on one scrollable page.
 * This is the artifact design review happens against, so each section is
 * labelled and each state called out explicitly rather than left to be
 * inferred from a single "normal" example. All sample data is self
 * contained -- nothing outside this file needs to feed it a model.
 */
Item {
    id: root

    readonly property var iconTints: ({
        grass: "#6aa84f",
        gold: "#e0b000",
        diamond: "#3ec6d0",
        enderman: "#30203a",
        fabric: "#d9773f",
        creeper: "#4b8f3e",
        iron: "#b8b8b8",
        stone: "#8a8a8a",
        tnt: "#c0392b"
    })

    readonly property var iconNames: [
        "home", "library", "compass", "package", "settings", "search", "plus", "play",
        "stop", "folder", "more", "chevron-down", "chevron-right", "user", "users",
        "log-out", "refresh", "sort", "clock", "cube", "x", "check", "sun", "moon",
        "external-link", "trash", "copy", "edit", "download", "grid", "list", "bell",
        "info", "alert-triangle", "layers", "terminal", "image", "globe"
    ]

    readonly property var navItems: [
        { id: "library", icon: "library", label: i18n("Library") },
        { id: "modpacks", icon: "compass", label: i18n("Discover") }
    ]
    readonly property var navFooterItems: [
        { id: "settings", icon: "settings", label: i18n("Settings") }
    ]

    Rectangle {
        anchors.fill: parent
        color: Theme.palette.canvas
    }

    ListModel {
        id: recentModel
        ListElement { instanceId: "recent-1"; name: "Vanilla 1.21.4"; iconKey: "grass"; isRunning: false }
        ListElement { instanceId: "recent-2"; name: "All the Mods 10"; iconKey: "iron"; isRunning: true }
        ListElement { instanceId: "recent-3"; name: "Skyblock Extreme"; iconKey: "diamond"; isRunning: false }
    }

    ListModel {
        id: vanillaModel
        ListElement { instanceId: "vanilla-1"; name: "Vanilla 1.21.4"; iconKey: "grass"; isRunning: false; canLaunch: true; lastLaunch: 1726000000000; gameVersion: "1.21.4"; loader: ""; iconTint: "#6aa84f"; coverImage: ""; launchStatus: ""; launchProgress: -1 }
        ListElement { instanceId: "vanilla-2"; name: "Superflat Creative"; iconKey: "stone"; isRunning: false; canLaunch: true; lastLaunch: 0; gameVersion: "1.20.1"; loader: ""; iconTint: "#8a8a8a"; coverImage: ""; launchStatus: ""; launchProgress: -1 }
        ListElement { instanceId: "vanilla-3"; name: "Hardcore Survival"; iconKey: "tnt"; isRunning: false; canLaunch: true; lastLaunch: 1706000000000; gameVersion: "1.19.4"; loader: ""; iconTint: "#c0392b"; coverImage: ""; launchStatus: ""; launchProgress: -1 }
    }

    ListModel {
        id: moddedModel
        ListElement { instanceId: "modded-1"; name: "All the Mods 10"; iconKey: "iron"; isRunning: true; canLaunch: true; lastLaunch: 1726000000000; gameVersion: "1.20.1"; loader: "Fabric"; iconTint: "#b8b8b8"; coverImage: ""; launchStatus: ""; launchProgress: -1 }
        ListElement { instanceId: "modded-2"; name: "Create: Above and Beyond"; iconKey: "fabric"; isRunning: false; canLaunch: true; lastLaunch: 1706000000000; gameVersion: "1.18.2"; loader: "Fabric"; iconTint: "#d9773f"; coverImage: ""; launchStatus: ""; launchProgress: -1 }
        ListElement { instanceId: "modded-3"; name: "Enderman Challenge"; iconKey: "enderman"; isRunning: false; canLaunch: true; lastLaunch: 1706000000000; gameVersion: "1.21.1"; loader: "Forge"; iconTint: "#30203a"; coverImage: ""; launchStatus: ""; launchProgress: -1 }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: root.width
            spacing: Theme.space.xxl

            SectionHeader {
                Layout.topMargin: Theme.space.lg
                Layout.leftMargin: Theme.space.lg
                title: i18n("Top Bar")
                collapsible: false
            }

            TopBar {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.space.lg
                Layout.rightMargin: Theme.space.lg
                title: i18n("Instances")
                count: 12
                searchText: "vanilla"

                Button {
                    text: i18n("New instance")
                    highlighted: true
                }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Sidebar Nav")
                collapsible: false
            }

            RowLayout {
                Layout.leftMargin: Theme.space.lg

                SidebarNav {
                    Layout.preferredWidth: 244
                    Layout.preferredHeight: 420
                    items: root.navItems
                    footerItems: root.navFooterItems
                    currentId: "library"
                    recentModel: recentModel
                    accountName: "Steve"
                    accountKind: "Microsoft"
                }
            }

            // Nav Item states: unselected, selected.
            RowLayout {
                Layout.leftMargin: Theme.space.lg
                spacing: Theme.space.sm

                NavItem { iconName: "library"; label: i18n("Unselected") }
                NavItem { iconName: "library"; label: i18n("Selected"); selected: true }
            }

            // Account Chip states: signed in (Microsoft), signed in (Offline), signed out.
            RowLayout {
                Layout.leftMargin: Theme.space.lg
                spacing: Theme.space.lg

                AccountChip { name: "Steve"; kind: "Microsoft" }
                AccountChip { name: "Steve"; kind: "Offline" }
                AccountChip { name: ""; kind: "" }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Status Badge")
                collapsible: false
            }

            RowLayout {
                Layout.leftMargin: Theme.space.lg
                spacing: Theme.space.sm

                StatusBadge { text: i18n("Local"); tone: "neutral" }
                StatusBadge { text: i18n("Up to date"); tone: "success" }
                StatusBadge { text: i18n("Update available"); tone: "warning" }
                StatusBadge { text: i18n("Broken"); tone: "danger" }
                StatusBadge { text: i18n("Modded"); tone: "info" }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Buttons")
                collapsible: false
            }

            RowLayout {
                Layout.leftMargin: Theme.space.lg
                spacing: Theme.space.md

                PlayButton { }
                PlayButton { running: true }
                PlayButton { size: Theme.control.height }
                PlayButton { round: false }
                PlayButton { round: false; running: true }
                IconButton { iconName: "folder"; tip: i18n("Open folder") }
                IconButton { iconName: "more"; tip: i18n("More"); flat: false }
                IconButton { iconName: "refresh"; tip: i18n("Refresh"); size: Theme.control.heightLg }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Controls")
                collapsible: false
            }

            GridLayout {
                Layout.leftMargin: Theme.space.lg
                Layout.rightMargin: Theme.space.lg
                Layout.fillWidth: true
                columns: 3
                columnSpacing: Theme.space.xl
                rowSpacing: Theme.space.lg

                TextField {
                    Layout.preferredWidth: 220
                    placeholderText: i18n("Text field")
                }
                ComboBox {
                    Layout.preferredWidth: 220
                    model: [i18n("First option"), i18n("Second option"), i18n("Third option")]
                }
                SpinBox {
                    Layout.preferredWidth: 140
                    from: 0
                    to: 100
                    value: 42
                }

                RowLayout {
                    spacing: Theme.space.lg
                    CheckBox { text: i18n("Unchecked") }
                    CheckBox { text: i18n("Checked"); checked: true }
                }
                RowLayout {
                    spacing: Theme.space.lg
                    RadioButton { text: i18n("Off") }
                    RadioButton { text: i18n("On"); checked: true }
                }
                RowLayout {
                    spacing: Theme.space.lg
                    Switch { text: i18n("Off") }
                    Switch { text: i18n("On"); checked: true }
                }

                Slider {
                    Layout.preferredWidth: 220
                    value: 0.6
                }
                SegmentedControl {
                    options: [ { value: "a", label: i18n("Day") }, { value: "b", label: i18n("Week") }, { value: "c", label: i18n("Month") } ]
                    current: "b"
                }
                TabStrip {
                    tabs: [ { id: "one", label: i18n("Mods"), count: 6 }, { id: "two", label: i18n("Worlds") }, { id: "three", label: i18n("Log") } ]
                    current: "one"
                }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Dialog Header")
                collapsible: false
            }

            RowLayout {
                Layout.leftMargin: Theme.space.lg
                Layout.rightMargin: Theme.space.lg
                spacing: Theme.space.md

                Repeater {
                    model: [
                        { title: i18n("Delete world"), icon: "alert-triangle", tint: Theme.palette.danger },
                        { title: i18n("Update available"), icon: "download", tint: Theme.palette.accent },
                        { title: i18n("Choose an icon"), icon: "image", tint: Theme.palette.accent }
                    ]

                    delegate: Rectangle {
                        required property var modelData
                        Layout.preferredWidth: 280
                        Layout.preferredHeight: 64
                        radius: Theme.radius.xl
                        color: Theme.palette.surfaceOverlay
                        border.width: 1
                        border.color: Theme.palette.border

                        DialogHeader {
                            anchors.fill: parent
                            title: modelData.title
                            icon: modelData.icon
                            iconColor: modelData.tint
                        }
                    }
                }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Tag & Search Field")
                collapsible: false
            }

            RowLayout {
                Layout.leftMargin: Theme.space.lg
                spacing: Theme.space.sm

                Tag { text: "Fabric"; iconName: "layers" }
                Tag { text: "1.21.4"; iconName: "cube" }
                Tag { text: i18n("2 h ago"); iconName: "clock" }

                Rectangle {
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: Theme.control.heightLg
                    radius: Theme.radius.md
                    color: Theme.palette.textPrimary

                    Tag {
                        anchors.centerIn: parent
                        text: i18n("On media")
                        iconName: "check"
                        onMedia: true
                    }
                }

                SearchBox {
                    Layout.preferredWidth: 220
                    placeholderText: i18n("Search")
                }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Instance Card")
                collapsible: false
            }

            RowLayout {
                Layout.leftMargin: Theme.space.lg
                Layout.rightMargin: Theme.space.lg
                spacing: Theme.space.md

                // Every state InstanceCard can be in: rest, forceHovered,
                // selected, running, never played, launching.
                Repeater {
                    model: [
                        { instanceId: "card-rest", name: i18n("Vanilla 1.21.4"), iconKey: "grass", version: "1.21.4", loader: "", last: Date.now() - 2 * 86400000, running: false, hovered: false, selected: false },
                        { instanceId: "card-hovered", name: i18n("Skyblock Extreme"), iconKey: "diamond", version: "1.20.1", loader: "Fabric", last: Date.now(), running: false, hovered: true, selected: false },
                        { instanceId: "card-selected", name: i18n("A very long modpack name that needs to wrap"), iconKey: "gold", version: "1.20.1", loader: "Forge", last: Date.now() - 3 * 86400000, running: false, hovered: false, selected: true },
                        { instanceId: "card-running", name: i18n("Create: Above and Beyond"), iconKey: "enderman", version: "1.18.2", loader: "Fabric", last: Date.now(), running: true, hovered: false, selected: false },
                        { instanceId: "card-never-played", name: i18n("Superflat Creative"), iconKey: "stone", version: "1.20.1", loader: "", last: 0, running: false, hovered: false, selected: false },
                        { instanceId: "card-launching", name: i18n("Better Minecraft"), iconKey: "tnt", version: "1.20.1", loader: "Forge", last: 0, running: true, hovered: false, selected: false, status: i18n("Downloading assets"), progress: 0.45 }
                    ]

                    delegate: InstanceCard {
                        required property var modelData
                        Layout.preferredWidth: 208
                        instanceId: modelData.instanceId
                        name: modelData.name
                        iconKey: modelData.iconKey
                        isRunning: modelData.running
                        canLaunch: true
                        lastLaunch: modelData.last
                        gameVersion: modelData.version
                        loader: modelData.loader
                        iconTint: root.iconTints[modelData.iconKey]
                        forceHovered: modelData.hovered
                        selected: modelData.selected
                        launchStatus: modelData.status || ""
                        launchProgress: modelData.progress !== undefined ? modelData.progress : -1
                        coverImage: ""
                    }
                }
            }

            // InstanceListRow mid-launch and running: the list view's side
            // of the card states above.
            ColumnLayout {
                Layout.leftMargin: Theme.space.lg
                Layout.preferredWidth: 760
                spacing: Theme.space.sm

                Repeater {
                    model: [
                        { instanceId: "row-launching", name: i18n("Better Minecraft"), iconKey: "tnt", version: "1.20.1", loader: "Forge", status: i18n("Downloading assets"), progress: 0.45 },
                        { instanceId: "row-running", name: i18n("Create: Above and Beyond"), iconKey: "enderman", version: "1.18.2", loader: "Fabric" }
                    ]

                    delegate: InstanceListRow {
                        required property var modelData
                        Layout.fillWidth: true
                        instanceId: modelData.instanceId
                        name: modelData.name
                        iconKey: modelData.iconKey
                        isRunning: true
                        canLaunch: true
                        lastLaunch: Date.now()
                        totalTimePlayed: 0
                        gameVersion: modelData.version
                        loader: modelData.loader
                        iconTint: root.iconTints[modelData.iconKey]
                        launchStatus: modelData.status || ""
                        launchProgress: modelData.progress !== undefined ? modelData.progress : -1
                    }
                }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Continue Card")
                collapsible: false
            }

            ContinueCard {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.space.lg
                Layout.rightMargin: Theme.space.lg
                instanceId: "continue-1"
                name: i18n("All the Mods 10")
                iconKey: "iron"
                isRunning: false
                canLaunch: true
                lastLaunch: Date.now() - 3600000
                totalTimePlayed: 5 * 3600 + 20 * 60
                gameVersion: "1.20.1"
                loader: "Fabric"
                iconTint: root.iconTints.iron
                coverImage: ""
                launchStatus: ""
                launchProgress: -1
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Instance Section")
                collapsible: false
            }

            InstanceSection {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.space.lg
                Layout.rightMargin: Theme.space.lg
                title: i18n("Vanilla")
                model: vanillaModel
                columns: 3
                cardWidth: 208
            }

            InstanceSection {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.space.lg
                Layout.rightMargin: Theme.space.lg
                title: i18n("Modded")
                model: moddedModel
                columns: 3
                cardWidth: 208
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Empty State")
                collapsible: false
            }

            EmptyState {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                title: i18n("No instances yet")
                body: i18n("Create an instance to see it here.")
                actionText: i18n("Create instance")

                MeshIcon {
                    iconName: "package"
                    size: Theme.icon.lg * 2
                    color: Theme.palette.textTertiary
                }
            }

            SectionHeader {
                Layout.leftMargin: Theme.space.lg
                title: i18n("Icons")
                collapsible: false
            }

            Grid {
                Layout.leftMargin: Theme.space.lg
                Layout.rightMargin: Theme.space.lg
                columns: 8
                columnSpacing: Theme.space.lg
                rowSpacing: Theme.space.lg

                Repeater {
                    model: root.iconNames

                    delegate: Column {
                        required property string modelData
                        width: 64
                        spacing: Theme.space.xs

                        MeshIcon {
                            anchors.horizontalCenter: parent.horizontalCenter
                            iconName: parent.modelData
                            size: Theme.icon.lg
                        }

                        Text {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: parent.modelData
                            color: Theme.palette.textTertiary
                            font.family: Theme.font.family
                            font.pixelSize: Theme.type.caption.pixelSize
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: Theme.space.xxl }
        }
    }
}
