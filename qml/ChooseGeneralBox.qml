import QtQuick 6.5

import rocks.touhousatsu 1.0

// General chooser built on GraphicsBox. Receives a list of general names and lets the
// player pick one (singleResult) or two (hegemony dual-general). Confirmation is done via
// the Dashboard OK button (roomScene.activeChooseGeneralBox.accept()).
// Right-click a card with ServerInfo.FreeChoose to pop the C++ FreeChooseDialog and swap it.
// KnownBoth (know-each-other card effect) logic is NOT implemented here.
GraphicsBox {
    id: chooseGeneralBox

    property var generals: []
    property var selectedGenerals: []
    property bool singleResult: true

    signal generalChosen(string generalName)

    function _toggle(general) {
        var idx = selectedGenerals.indexOf(general);
        if (idx >= 0) {
            var arr = selectedGenerals.slice();
            arr.splice(idx, 1);
            selectedGenerals = arr;
        } else {
            if (singleResult) {
                selectedGenerals = [general];
            } else if (selectedGenerals.length < 2) {
                var arr2 = selectedGenerals.slice();
                arr2.push(general);
                selectedGenerals = arr2;
            }
        }
    }

    function accept() {
        if (selectedGenerals.length > 0) {
            generalChosen(selectedGenerals.join("+"));
            if (parent && parent.activeChooseGeneralBox === chooseGeneralBox)
                parent.activeChooseGeneralBox = null;
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
            general: modelData
            opacity: 1
            scale: chooseGeneralBox.selectedGenerals.indexOf(modelData) >= 0 ? 1.1 : 1.0

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
