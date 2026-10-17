// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick
import org.kde.kirigami as Kirigami

/*
 * "Sort" in a page's toolbar: a menu of the sorts a provider offers, the
 * current one checked. `options` is the provider's list, [{id, label}];
 * `current` the id in use. One sort is no choice, so the action hides.
 */
Kirigami.Action {
    id: sort

    property var options: []
    property var current
    signal picked(var id)

    text: i18nc("@action:button", "Sort")
    icon.name: "view-sort"
    visible: sort.options.length > 1

    function refresh() {
        var list = []
        for (var i = 0; i < sort.choices.count; ++i) {
            if (sort.choices.objectAt(i))
                list.push(sort.choices.objectAt(i))
        }
        sort.children = list
    }

    // A property, not a child: an action's children are its menu entries.
    readonly property Instantiator choices: Instantiator {
        model: sort.options
        delegate: Kirigami.Action {
            required property var modelData
            text: modelData.label
            checkable: true
            checked: sort.current === modelData.id
            onTriggered: {
                // A checkable action unchecks itself on a second click.
                checked = Qt.binding(() => sort.current === modelData.id)
                if (sort.current !== modelData.id)
                    sort.picked(modelData.id)
            }
        }
        onObjectAdded: Qt.callLater(sort.refresh)
        onObjectRemoved: Qt.callLater(sort.refresh)
    }
}
