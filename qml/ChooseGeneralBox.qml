import QtQuick 6.5

import rocks.touhousatsu 1.0

// General chooser built on GraphicsBox. Receives a list of general names and lets the
// player pick one (singleResult) or two (hegemony dual-general). Confirmation is done via
// the Dashboard OK button (roomScene.activeBox.accept()).
// Right-click a card with ServerInfo.FreeChoose to pop the C++ FreeChooseDialog and swap it.
// KnownBoth (know-each-other card effect) logic is NOT implemented here.
GraphicsBox {
    id: chooseGeneralBox

    // Whether the current selection can be confirmed: single-general needs >=1, dual needs 2.
    property bool canAccept: selectedGenerals.length >= (singleResult ? 1 : 2)
    property var generals: []
    property var selectedGenerals: []
    property bool singleResult: true

    signal generalChosen(string generalName)

    // Hegemony pairing rule: two generals must share a kingdom, or at least one must be
    // "zhu" (wildcard kingdom). Mirrors server-side check in room.cpp:3723 and the old
    // uibackup/choosegeneralbox.cpp:489.
    // Hegemony general names carry a "_hegemony" suffix and are distinct generals -- do NOT
    // strip it, that would resolve to a different general with a possibly different kingdom.
    function _canPair(g1, g2) {
        var k1 = Sanguosha.getGeneral(g1).kingdom;
        var k2 = Sanguosha.getGeneral(g2).kingdom;
        return k1 === k2 || k1 === "zhu" || k2 === "zhu";
    }

    // Whether a candidate should be dimmed (unselectable) in dual-general mode:
    //  - 0 picked: none dimmed
    //  - 1 picked: dim those that cannot pair with the picked one
    //  - 2 picked: dim all remaining (selection is full)
    function _isDimmed(g) {
        if (singleResult)
            return false;
        var sel = selectedGenerals;
        if (sel.indexOf(g) >= 0)
            return false;
        if (sel.length === 0)
            return false;
        if (sel.length >= 2)
            return true;
        return !_canPair(sel[0], g);
    }

    function _toggle(general) {
        var idx = selectedGenerals.indexOf(general);
        if (idx >= 0) {
            var arr = selectedGenerals.slice();
            arr.splice(idx, 1);
            selectedGenerals = arr;
        } else {
            if (singleResult) {
                selectedGenerals = [general];
            } else if (selectedGenerals.length === 0) {
                selectedGenerals = [general];
            } else if (selectedGenerals.length === 1) {
                // Dual-general: the second pick must satisfy the kingdom pairing rule;
                // otherwise it is rejected (the candidate is also dimmed via opacity).
                if (_canPair(selectedGenerals[0], general)) {
                    var arr2 = selectedGenerals.slice();
                    arr2.push(general);
                    selectedGenerals = arr2;
                }
            }
        }
    }

    function accept() {
        if (canAccept) {
            generalChosen(selectedGenerals.join("+"));
            if (parent && parent.activeBox === chooseGeneralBox)
                parent.activeBox = null;
            chooseGeneralBox.destroy();
        }
    }

    height: 460
    source: G.getAssetUrl("image/system/card-container.png")
    width: 720

    GridView {
        id: generalGrid

        anchors.fill: parent
        anchors.margins: 10
        cellHeight: 266
        cellWidth: 193
        clip: true
        model: chooseGeneralBox.generals

        delegate: CardItem {
            enabled: !chooseGeneralBox._isDimmed(modelData)
            general: modelData
            opacity: 1
            selected: chooseGeneralBox.selectedGenerals.indexOf(modelData) >= 0
            useSelectionBorder: true

            onClicked: chooseGeneralBox._toggle(modelData)

            // Right-click + FreeChoose: pop C++ FreeChooseDialog to swap this general.
            onRightClicked: {
                if (ServerInfo.FreeChoose) {
                    var name = chooseGeneralBox.parent.freeChooseGeneral();
                    if (name.length > 0) {
                        var arr = chooseGeneralBox.generals.slice();
                        arr[index] = name;
                        chooseGeneralBox.generals = arr;
                    }
                }
            }
        }
    }
}
