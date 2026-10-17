// SPDX-FileCopyrightText: 2026 Project Tick
// SPDX-FileContributor: Project Tick
// SPDX-License-Identifier: Apache-2.0

import QtQuick

/*
 * What the skin page (AccountSkinPage.qml) is showing and doing for one
 * Microsoft account: its skin and capes (AccountsController.
 * accountSkinInfo()), its skin library (AccountsController.skins(), a
 * SkinsController), the skin picked for the preview and "Wear", and the
 * one change running at a time.
 *
 * The picked skin is a library skin by name, or one of the game's nine
 * default skins by name with the arm width it is shown with. Neither
 * picked shows what the account wears now. Classic/Slim follows the picked
 * skin: on a library skin a click only asks the library to remember the
 * other width (SkinsController.setSlim()), and the radios show whatever the
 * library then says; on a default skin it picks its other texture.
 */
QtObject {
    id: state

    // AccountsController.
    property var controller: null
    // AccountList row; -1 is the review demo (accountSkinInfo(-1)).
    property int row: -1
    property string accountId

    property var info: state.emptyInfo()
    property var skins: null
    readonly property var skinList: state.skins ? state.skins.skins : []
    readonly property var defaultList: state.skins ? state.skins.defaultSkins : []
    readonly property bool defaultsLoading: !!state.skins && state.skins.defaultSkinsLoading

    property string selectedName: ""
    property string selectedDefault: ""
    property bool defaultSlim: false
    readonly property var selected: {
        if (state.selectedDefault.length > 0) {
            var character = state.defaultList.find(d => d.name === state.selectedDefault)
            return character ? character.variants[state.defaultSlim ? 1 : 0] : null
        }
        return state.skinList.find(s => s.name === state.selectedName) || null
    }
    readonly property bool selectedReady: !!state.selected && state.selected.ready !== false
    // The arm width the radios show: the picked skin's, else the account's.
    readonly property bool slim: state.selected ? !!state.selected.slim : !!state.info.slim

    // The change running, if any (a TaskWatcher), and what the last one said.
    property var watcher: null
    readonly property bool busy: !!state.watcher && state.watcher.running
    property string message: ""
    property bool messageIsError: false

    // A cape was put on: the preview turns round to show it.
    signal capeWorn()

    function emptyInfo() {
        return { valid: false, slim: false, hasSkin: false, currentCapeId: "", currentCapeUrl: "", capes: [] }
    }
    function reload() {
        state.info = state.controller ? state.controller.accountSkinInfo(state.row) : state.emptyInfo()
    }
    function start() {
        state.reload()
        state.skins = state.controller ? state.controller.skins(state.row) : null
        // The review demo shows its first skin straight away.
        state.selectedName = state.row === -1 && state.skinList.length > 0 ? state.skinList[0].name : ""
        state.selectedDefault = ""
    }
    function say(text, isError) {
        state.message = text
        state.messageIsError = !!isError
    }

    function selectLibrary(name) {
        state.selectedDefault = ""
        state.selectedName = state.selectedName === name ? "" : name
    }
    function selectDefault(character) {
        state.selectedName = ""
        if (state.selectedDefault === character.name) {
            state.selectedDefault = ""
            return
        }
        state.defaultSlim = character.slim
        state.selectedDefault = character.name
    }
    function setSlim(slim) {
        if (slim === state.slim || !state.selected)
            return
        if (state.selectedDefault.length > 0)
            state.defaultSlim = slim
        else if (state.skins)
            state.skins.setSlim(state.selectedName, slim)
    }

    // Starts @p w; @p done(ok) runs once it ends.
    function run(w, failText, done) {
        if (!w) {
            state.say(failText, true)
            return
        }
        state.say("", false)
        state.watcher = w
        w.finished.connect(function(ok) {
            state.reload()
            if (!ok)
                state.say(w.error.length > 0 ? w.error : failText, true)
            if (done)
                done(ok)
        })
    }
    function wearSelected() {
        var skin = state.selected
        if (!state.selectedReady)
            return
        // A default skin is not in the library, so there is nothing to mark.
        var fromLibrary = state.selectedDefault.length === 0
        state.run(state.controller.changeSkin(state.row, skin.path, skin.slim),
                  i18n("The skin could not be uploaded."),
                  function(ok) {
                      if (ok && fromLibrary && state.skins)
                          state.skins.markWorn(skin.name)
                      if (ok)
                          state.say(i18n("The account wears the new skin."), false)
                  })
    }
    function resetSkin() {
        state.run(state.controller.resetSkin(state.row), i18n("The skin could not be reset."))
    }
    function wearCape(capeId) {
        state.run(state.controller.changeCape(state.row, capeId), i18n("The cape could not be changed."),
                  function(ok) {
                      if (ok && capeId.length > 0)
                          state.capeWorn()
                  })
    }

    readonly property Connections accountsWatcher: Connections {
        target: state.controller ? state.controller.accounts : null
        ignoreUnknownSignals: true
        function onDataChanged() { state.reload() }
    }
    readonly property Connections defaultsWatcher: Connections {
        target: state.controller
        ignoreUnknownSignals: true
        function onDefaultSkinsRevisionChanged() { state.reload() }
    }
}
