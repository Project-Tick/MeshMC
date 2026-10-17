// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * What the setting delegates read and write through: one SettingsAdapter,
 * the launcher's (shell.settings) or one instance's (details.settings).
 * `revision` moves on every change, so a binding that reads a value through
 * here follows it, whoever changed it -- this window, another page, a
 * plugin.
 *
 * Values read back from the config file can be text ("false" is truthy in
 * JavaScript), so the delegates go through bool(), number() and string()
 * rather than value().
 */
QtObject {
    id: store

    property var adapter: null
    property int revision: 0

    // After any setting has changed, with its id.
    signal changed(string key)

    readonly property Connections watcher: Connections {
        target: store.adapter
        ignoreUnknownSignals: true
        function onValueChanged(id) {
            store.revision++
            store.changed(id)
        }
    }

    function value(key) {
        // Reading revision makes every caller's binding depend on it.
        return store.revision >= 0 && store.adapter ? store.adapter.value(key) : undefined
    }

    function bool(key) {
        var v = store.value(key)
        return v === true || v === "true" || v === 1 || v === "1"
    }

    function number(key) {
        var n = Number(store.value(key))
        return isNaN(n) ? 0 : n
    }

    function string(key) {
        var v = store.value(key)
        return v === undefined || v === null ? "" : String(v)
    }

    function setValue(key, value) {
        if (store.adapter)
            store.adapter.setValue(key, value)
    }

    function reset(key) {
        if (store.adapter)
            store.adapter.reset(key)
    }
}
