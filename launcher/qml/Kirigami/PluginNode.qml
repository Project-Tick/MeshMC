// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/*
 * One node of a plugin's "mmco-ui/1" document, drawn with Kirigami and
 * FormCard -- a plugin names what it needs (a toggle, a list), never how it
 * looks. Containers draw their children through this same file.
 *
 * Every user action goes back to the plugin as (surface, node, event,
 * value) through `sink.sendEvent`; the plugin answers by patching its
 * document, which arrives here as a new `node`.
 *
 * `inCard`: the node sits inside a FormCard, so leaves are card delegates;
 * outside a card they are plain controls.
 */
Item {
    id: root

    property var node: ({})
    property string surfaceId
    // Has sendEvent(surfaceId, nodeId, event, value).
    property var sink: null
    property bool inCard: false
    // Children of a row keep their natural width; everything else fills.
    property bool stretch: true

    readonly property string type: node && node.type ? node.type : ""
    readonly property var props: node && node.props ? node.props : ({})
    readonly property var children_: node && node.children ? node.children : []

    function send(event, value) {
        if (root.sink)
            root.sink.sendEvent(root.surfaceId, root.node.id || "", event, value)
    }

    visible: props.visible !== false
    Layout.fillWidth: root.stretch
    implicitWidth: loader.implicitWidth
    implicitHeight: root.visible ? loader.implicitHeight : 0
    Binding on width {
        when: !root.stretch
        value: root.implicitWidth
    }

    Loader {
        id: loader
        width: root.stretch ? root.width : implicitWidth
        sourceComponent: {
            switch (root.type) {
            case "column": return columnNode
            case "row": return rowNode
            case "section": return sectionNode
            case "heading": return headingNode
            case "text": return root.props.format === "markdown" ? markdownNode : textNode
            case "separator": return separatorNode
            case "progress": return progressNode
            case "button": return buttonNode
            case "toggle": return toggleNode
            case "text_field": return textFieldNode
            case "number_field": return numberFieldNode
            case "choice": return choiceNode
            case "list": return listNode
            case "link": return linkNode
            default: return null
            }
        }
    }

    // A child of a container: another PluginNode, bound to its node so a
    // patch reaches it. Children of a column keep this node's card-ness,
    // those of a section sit in the card, those of a row are plain and
    // keep their natural width.
    Component {
        id: childNode
        Loader {
            id: child
            required property var modelData
            Layout.fillWidth: true
            source: Qt.resolvedUrl("PluginNode.qml")
            onLoaded: {
                item.inCard = root.inCard
                item.surfaceId = root.surfaceId
                item.sink = root.sink
                item.node = Qt.binding(() => child.modelData)
            }
        }
    }
    Component {
        id: cardChildNode
        Loader {
            id: child
            required property var modelData
            Layout.fillWidth: true
            source: Qt.resolvedUrl("PluginNode.qml")
            onLoaded: {
                item.inCard = true
                item.surfaceId = root.surfaceId
                item.sink = root.sink
                item.node = Qt.binding(() => child.modelData)
            }
        }
    }

    Component {
        id: columnNode
        ColumnLayout {
            spacing: root.inCard ? 0 : Kirigami.Units.smallSpacing
            Repeater {
                model: root.children_
                delegate: childNode
            }
        }
    }

    Component {
        id: rowNode
        Flow {
            spacing: Kirigami.Units.smallSpacing
            padding: root.inCard ? Kirigami.Units.largeSpacing : 0
            Repeater {
                model: root.children_
                delegate: Loader {
                    id: flowChild
                    required property var modelData
                    source: Qt.resolvedUrl("PluginNode.qml")
                    onLoaded: {
                        item.stretch = false
                        item.inCard = false
                        item.surfaceId = root.surfaceId
                        item.sink = root.sink
                        item.node = Qt.binding(() => flowChild.modelData)
                    }
                }
            }
        }
    }

    // A section is a titled card. Inside a card already, only its children.
    Component {
        id: sectionNode
        ColumnLayout {
            spacing: 0
            FormCard.FormHeader {
                visible: !root.inCard && (root.props.title || "").length > 0
                title: root.props.title || ""
            }
            QQC2.Label {
                visible: root.inCard && (root.props.title || "").length > 0
                text: root.props.title || ""
                font.bold: true
                Layout.margins: Kirigami.Units.largeSpacing
            }
            Loader {
                Layout.fillWidth: true
                sourceComponent: root.inCard ? sectionBody : sectionCard
            }
        }
    }
    Component {
        id: sectionCard
        FormCard.FormCard {
            Repeater {
                model: root.children_
                delegate: cardChildNode
            }
        }
    }
    Component {
        id: sectionBody
        ColumnLayout {
            spacing: 0
            Repeater {
                model: root.children_
                delegate: cardChildNode
            }
        }
    }

    Component {
        id: headingNode
        Kirigami.Heading {
            level: 2
            wrapMode: Text.Wrap
            text: root.props.text || ""
            leftPadding: root.inCard ? Kirigami.Units.largeSpacing : 0
            topPadding: root.inCard ? Kirigami.Units.largeSpacing : 0
        }
    }

    Component {
        id: textNode
        QQC2.Label {
            wrapMode: Text.Wrap
            textFormat: Text.PlainText
            text: root.props.text || ""
            color: Kirigami.Theme.disabledTextColor
            padding: root.inCard ? Kirigami.Units.largeSpacing : 0
        }
    }

    // Markdown may link images from anywhere; see RemoteRichText.
    Component {
        id: markdownNode
        RemoteRichText {
            source: root.props.text || ""
            leftPadding: root.inCard ? Kirigami.Units.largeSpacing : 0
            rightPadding: leftPadding
            topPadding: leftPadding
            bottomPadding: leftPadding
            onLinkActivated: (link) => root.send("click", link)
        }
    }

    Component {
        id: separatorNode
        Kirigami.Separator {}
    }

    Component {
        id: progressNode
        Item {
            readonly property real margin: root.inCard ? Kirigami.Units.largeSpacing : 0
            implicitHeight: bar.implicitHeight + margin * 2
            LaunchProgressBar {
                id: bar
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: parent.margin
                anchors.rightMargin: parent.margin
                progress: root.props.value === undefined || root.props.value < 0 ? -1 : root.props.value / 100
            }
        }
    }

    Component {
        id: buttonNode
        Item {
            implicitWidth: root.inCard ? cardButton.implicitWidth : button.implicitWidth
            implicitHeight: root.inCard ? cardButton.implicitHeight : button.implicitHeight
            FormCard.FormButtonDelegate {
                id: cardButton
                visible: root.inCard
                width: parent.width
                text: root.props.label || ""
                enabled: root.props.enabled !== false
                onClicked: root.send("click", null)
            }
            QQC2.Button {
                id: button
                visible: !root.inCard
                text: root.props.label || ""
                enabled: root.props.enabled !== false
                highlighted: root.props.style === "primary"
                onClicked: root.send("click", null)
            }
        }
    }

    Component {
        id: toggleNode
        FormCard.FormSwitchDelegate {
            text: root.props.label || ""
            description: root.props.description || ""
            enabled: root.props.enabled !== false
            checked: root.props.value === true
            onToggled: {
                root.send("change", checked)
                checked = Qt.binding(() => root.props.value === true)
            }
        }
    }

    Component {
        id: textFieldNode
        FormCard.FormTextFieldDelegate {
            label: root.props.label || ""
            placeholderText: root.props.placeholder || ""
            enabled: root.props.enabled !== false
            // Follows the plugin's value while the user is not editing.
            readonly property string pluginValue: root.props.value !== undefined && root.props.value !== null
                                                  ? String(root.props.value) : ""
            onPluginValueChanged: if (!fieldActiveFocus) text = pluginValue
            Component.onCompleted: text = pluginValue
            // Same moment the widget renderer reports it: when editing
            // finishes, so a plugin never sees half-typed values.
            onEditingFinished: root.send("change", text)
        }
    }

    Component {
        id: numberFieldNode
        SpinBoxDelegate {
            label: root.props.label || ""
            enabled: root.props.enabled !== false
            from: root.props.min !== undefined ? root.props.min : 0
            to: root.props.max !== undefined ? root.props.max : 1000000
            stepSize: root.props.step !== undefined ? root.props.step : 1
            value: root.props.value !== undefined ? root.props.value : 0
            onValueModified: {
                root.send("change", value)
                // Back to following the plugin's value, not the click.
                value = Qt.binding(() => root.props.value !== undefined ? root.props.value : 0)
            }
        }
    }

    Component {
        id: choiceNode
        FormCard.FormComboBoxDelegate {
            id: combo
            readonly property var options: (root.props.options || []).map(o => typeof o === "string" ? { id: o, label: o } : o)
            text: root.props.label || ""
            enabled: root.props.enabled !== false
            model: combo.options
            textRole: "label"
            valueRole: "id"
            // Through `count`, so it is worked out again once the model has
            // arrived rather than once, before it.
            currentIndex: count > 0 ? Math.max(0, combo.options.findIndex(o => o.id === root.props.value)) : -1
            onActivated: {
                root.send("change", currentValue)
                currentIndex = Qt.binding(() => count > 0 ? Math.max(0, combo.options.findIndex(o => o.id === root.props.value)) : -1)
            }
        }
    }

    // Rows of cells, one row selectable at a time; a double click activates.
    Component {
        id: listNode
        ColumnLayout {
            id: listColumn
            property string selectedId: ""
            spacing: 0

            RowLayout {
                visible: (root.props.columns || []).length > 1
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.smallSpacing
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Repeater {
                    model: root.props.columns || []
                    delegate: QQC2.Label {
                        required property string modelData
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: modelData
                        elide: Text.ElideRight
                        font.bold: true
                        color: Kirigami.Theme.disabledTextColor
                    }
                }
            }
            Repeater {
                model: root.props.rows || []
                delegate: QQC2.ItemDelegate {
                    id: listRow
                    required property var modelData
                    Layout.fillWidth: true
                    highlighted: listColumn.selectedId === modelData.id
                    onClicked: {
                        listColumn.selectedId = modelData.id
                        root.send("select", modelData.id)
                    }
                    onDoubleClicked: root.send("activate", modelData.id)
                    contentItem: RowLayout {
                        Repeater {
                            model: listRow.modelData.cells || []
                            delegate: QQC2.Label {
                                required property string modelData
                                required property int index
                                Layout.fillWidth: true
                                Layout.preferredWidth: 1
                                text: modelData
                                elide: Text.ElideRight
                                color: index === 0 ? Kirigami.Theme.textColor : Kirigami.Theme.disabledTextColor
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: linkNode
        Item {
            readonly property real margin: root.inCard ? Kirigami.Units.largeSpacing : 0
            implicitHeight: link.implicitHeight + margin * 2
            Kirigami.LinkButton {
                id: link
                anchors.left: parent.left
                anchors.leftMargin: parent.margin
                anchors.verticalCenter: parent.verticalCenter
                text: root.props.text || root.props.href || ""
                onClicked: root.send("click", root.props.href || "")
            }
        }
    }
}
