import QtQuick 6.5
import rocks.touhousatsu 1.0

Item {
    id: dashboard

    property bool cancelEnabled: false
    property var clientInstance: parent ? parent.ClientInstance : null
    property bool discardEnabled: false

    // Equip area (SelfEquipArea) lives inside equipBg; exposed via alias so
    // RoomScene can call equipArea.addEquip/removeEquip for PlaceEquip moves.
    property alias equipArea: equipArea
    property bool okEnabled: false
    required property var photo
    property var roomScene: null

    // Hand card sync (subtask A): add/remove CardItem in cardArea + relayout.
    // self hand cards only (other players' hidden cards sync via handcardNum, not here).
    function addHandCard(cardId: int) {
        cardArea.createItem(cardId);
        cardArea.lay(Qt.AlignLeft, 1, 0, true, true);
    }

    function removeHandCard(cardId: int) {
        cardArea.removeItem(cardId);
        cardArea.lay(Qt.AlignLeft, 1, 0, true, true);
    }

    // Button enabled state, set imperatively by updateStatus() on status change (mirrors
    // old RoomScene::updateStatus in uibackup/roomscene.cpp:2784-2949). OK cannot rely solely
    // on bindings because clicking it under some statuses drives complex flows (card
    // response with target choosing, skill confirm, etc.) that updateStatus must prepare.
    function updateStatus() {
        // Defaults; each case overrides as needed.
        okEnabled = false;
        cancelEnabled = false;
        discardEnabled = false;

        if (clientInstance === null)
            return;

        var s = clientInstance.status;
        var refusable = clientInstance.discardActionRefusable;
        var box = parent !== null ? parent.activeBox : null;

        switch (s & Client.ClientStatusBasicMask) {
        case Client.NotActive:
            break;
        case Client.Responding:
            cancelEnabled = refusable;
            // TODO: okEnabled after card+target selection (CardItem TODO)
            break;
        case Client.Playing:
            discardEnabled = true;
            break;
        case Client.Discarding:
        case Client.Exchanging:
            cancelEnabled = refusable;
            break;
        case Client.ExecDialog:
            cancelEnabled = true;
            break;
        case Client.AskForSkillInvoke:
            okEnabled = true;
            cancelEnabled = true;
            break;
        case Client.AskForPlayerChoose:
            cancelEnabled = refusable;
            break;
        case Client.AskForShowOrPindian:
            break;
        case Client.AskForGeneralTaken:
            // Choose-general: OK confirms via activeBox.canAccept.
            okEnabled = box !== null && box.canAccept;
            break;
        default:
            break;
        }

        // TODO: remaining updateStatus logic per old uibackup/roomscene.cpp:2784-2949:
        //   prompt box show/hide, card pending (response_skill/discard_skill/showorpindian_skill),
        //   skill button enable/highlight, target selection prep, card enable/disable.
    }

    height: 360

    Component.onCompleted: {
        okEnabled = false;
        cancelEnabled = false;
        discardEnabled = false;
    }
    onClientInstanceChanged: {
        okEnabled = false;
        cancelEnabled = false;
        discardEnabled = false;
    }

    Connections {
        function onStatusChanged() {
            dashboard.updateStatus();
        }

        target: dashboard.clientInstance
    }

    Connections {
        function onCanAcceptChanged() {
            dashboard.okEnabled = dashboard.parent.activeBox.canAccept;
        }

        target: dashboard.parent !== null ? dashboard.parent.activeBox : null
    }

    Image {
        id: equipBg

        anchors.left: parent.left
        anchors.top: parent.top
        height: parent.height
        source: G.getAssetUrl("image/system/dashboard-equip.png")
        width: 347

        // Self equip area (5 slots). equipCardIds/addEquip/removeEquip live inside
        // SelfEquipArea (EquipAreaBase). TODO (equip-area task): click routing /
        // valid state / broken placeholder / distance / equip skill button.
        SelfEquipArea {
            id: equipArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 72
        }
    }

    Image {
        id: cardBg

        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height
        source: G.getAssetUrl("image/system/dashboard-hand.png")
        width: parent.width - 347

        CardContainer {
            id: cardArea

            anchors.bottom: parent.bottom
            anchors.bottomMargin: 10
            anchors.left: parent.left
            anchors.right: buttonSet.left
            anchors.top: parent.top
            anchors.topMargin: 94
            rootScene: dashboard.roomScene
        }

        // Button set: platter bg.png (100x195) + 4 platter buttons stacked per old layout
        // (skins/defaultSkin.layout.json: buttonSetSize/confirmButtonArea/cancelButtonArea/
        // discardButtonArea/trustButtonArea). Mirrors old RoomScene button_widget creation
        // (uibackup/roomscene.cpp:800-815).
        Item {
            id: buttonSet

            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.top: parent.top
            width: 184

            Image {
                anchors.fill: parent
                source: G.getAssetUrl("image/system/button/platter/bg.png")
            }

            QSanButton {
                disabledSource: G.getAssetUrl("image/system/button/platter/confirm/disabled.png")
                downSource: G.getAssetUrl("image/system/button/platter/confirm/down.png")
                enabled: dashboard.okEnabled
                height: 129
                hoverSource: G.getAssetUrl("image/system/button/platter/confirm/hover.png")
                normalSource: G.getAssetUrl("image/system/button/platter/confirm/normal.png")
                overlayEnabled: false
                width: 129
                x: 11
                y: 18

                onClicked: {
                    if (dashboard.parent && dashboard.parent.activeBox !== null)
                        dashboard.parent.activeBox.accept();
                    // TODO: handle AskForSkillInvoke / card-response OK once wired
                }
            }

            QSanButton {
                disabledSource: G.getAssetUrl("image/system/button/platter/cancel/disabled.png")
                downSource: G.getAssetUrl("image/system/button/platter/cancel/down.png")
                enabled: dashboard.cancelEnabled
                height: 127
                hoverSource: G.getAssetUrl("image/system/button/platter/cancel/hover.png")
                normalSource: G.getAssetUrl("image/system/button/platter/cancel/normal.png")
                overlayEnabled: false
                width: 129
                x: 2
                y: 206

                // TODO: doCancelButton based on ClientInstance.status
            }

            QSanButton {
                disabledSource: G.getAssetUrl("image/system/button/platter/discard/disabled.png")
                downSource: G.getAssetUrl("image/system/button/platter/discard/down.png")
                enabled: dashboard.discardEnabled
                height: 138
                hoverSource: G.getAssetUrl("image/system/button/platter/discard/hover.png")
                normalSource: G.getAssetUrl("image/system/button/platter/discard/normal.png")
                overlayEnabled: false
                width: 64
                x: 123
                y: 110

                // TODO: dashboard.clientInstance.onPlayerDiscardCards(selectedCard)
            }

            QSanButton {
                checkable: true
                disabledSource: G.getAssetUrl("image/system/button/platter/trust/disabled.png")
                downSource: G.getAssetUrl("image/system/button/platter/trust/down.png")
                enabled: dashboard.clientInstance != null
                height: 57
                hoverSource: G.getAssetUrl("image/system/button/platter/trust/hover.png")
                normalSource: G.getAssetUrl("image/system/button/platter/trust/normal.png")
                overlayEnabled: false
                width: 57
                x: 114
                y: 299

                onClicked: dashboard.clientInstance.trust()
            }
        }
    }

    PhaseItem {
        anchors.bottom: cardBg.top
        anchors.right: cardBg.right
        phase: photo.phase
        visible: photo.gameStarted
    }
}
