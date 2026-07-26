import QtQuick 6.5
import rocks.touhousatsu 1.0

Item {
    id: dashboard

    property var clientInstance: parent ? parent.ClientInstance : null

    // equipArea (SelfEquipArea), judgeArea (JudgeArea), phaseItem (PhaseItem):
    // exposed via alias so RoomScene can bind selfPhoto's corresponding properties
    // (selfPhoto does not construct its own). equipArea/judgeArea also used for
    // move dispatch (addEquip/removeEquip, addDelayedTrick/removeDelayedTrick).
    property alias equipArea: equipArea
    property alias judgeArea: judgeArea
    readonly property bool okEnabled: {
        if (clientInstance === null)
            return false;
        if ((clientInstance.status & Client.ClientStatusBasicMask) === Client.AskForGeneralTaken) {
            var box = parent !== null ? parent.activeBox : null;
            return box !== null && box.canAccept;
        }
        return roomScene !== null && roomScene.okEnabled;
    }
    property alias phaseItem: phaseItem
    required property var photo
    property var roomScene: null

    // Hand card sync (subtask A): add/remove CardItem in cardArea + relayout.
    // self hand cards only (other players' hidden cards sync via handcardNum, not here).
    function addHandCard(cardId: int) {
        var item = cardArea.createItem(cardId);
        item.clicked.connect(function () {
            item.toggleSelected();
            // Notify RoomScene for target-selection activation (Task F).
            if (dashboard.roomScene !== null)
                dashboard.roomScene.selectCard(dashboard.selectedCardIds());
        });
        cardArea.lay(Qt.AlignLeft, 1, 0, true, true);
    }

    function removeHandCard(cardId: int) {
        cardArea.removeItem(cardId);
        cardArea.lay(Qt.AlignLeft, 1, 0, true, true);
    }

    // Collect selected hand card IDs as an array.
    function selectedCardIds() {
        var ids = [];
        var sel = cardArea.getSelectedItems();
        for (var i = 0; i < sel.length; ++i)
            ids.push(sel[i].cardId);
        return ids;
    }

    // Hand-card enablement only (status-driven button enablement is in CppRoomScene::
    // updateDashboardStatus). Hand-card isAvailable stays in QML until CardContainer
    // is bridged.
    function updateStatus() {
        if (clientInstance === null)
            return;

        var s = clientInstance.status;
        var rs = dashboard.roomScene;
        var selfPlayer = rs !== null ? rs.Self : null;
        var sc = s & Client.ClientStatusBasicMask;
        var handCards = cardArea.cardItems;
        for (var i = 0; i < handCards.length; ++i) {
            var cid = handCards[i].cardId;
            var enab = true;
            if (cid !== -1 && selfPlayer !== null) {
                var card = dashboard.clientInstance.getCard(cid);
                if (sc === Client.Playing || sc === Client.Responding)
                    enab = card.isAvailable(selfPlayer);
            }
            handCards[i].enabled = enab;
        }

        // If status changed away from selection mode, clear selections.
        if (sc !== Client.Playing && sc !== Client.Responding && sc !== Client.Discarding && sc !== Client.Exchanging && sc !== Client.AskForShowOrPindian && sc
                !== Client.AskForGeneralTaken)
            cardArea.unselectAll(null);
    }

    height: 360

    Component.onCompleted: {
        dashboard.updateStatus();
        if (dashboard.roomScene !== null)
            dashboard.roomScene.updateDashboardStatus();
    }
    onClientInstanceChanged: {
        dashboard.updateStatus();
        if (dashboard.roomScene !== null)
            dashboard.roomScene.updateDashboardStatus();
    }

    // Status change: C++ updateDashboardStatus is called internally before
    // notifyStatusChanged; QML only needs to update hand-card isAvailable enablement.
    Connections {
        function onStatusChanged() {
            dashboard.updateStatus();
        }

        target: dashboard.clientInstance
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

    // Prompt box: shows server prompt text above the card area when the player needs
    // to respond. Driven by clientInstance.promptText (non-empty = visible).
    PromptBox {
        id: promptBox

        anchors.bottom: cardBg.top
        anchors.bottomMargin: 4
        anchors.horizontalCenter: cardBg.horizontalCenter
        clientInstance: dashboard.clientInstance
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
                    if (dashboard.parent && dashboard.parent.activeBox !== null) {
                        dashboard.parent.activeBox.accept();
                        return;
                    }

                    var s = dashboard.clientInstance.status;
                    var basic = s & Client.ClientStatusBasicMask;
                    var rs = dashboard.roomScene;

                    if (basic === Client.AskForSkillInvoke) {
                        if (rs !== null)
                            rs.respondToSkillInvoke(true);
                    } else if (basic === Client.Playing || basic === Client.Responding || basic === Client.AskForShowOrPindian) {
                        var ids = dashboard.selectedCardIds();
                        if (ids.length > 0 && rs !== null)
                            rs.submitCardResponse(ids);
                    } else if (basic === Client.Discarding || basic === Client.Exchanging) {
                        var ids2 = dashboard.selectedCardIds();
                        if (ids2.length > 0 && rs !== null)
                            rs.submitDiscard(ids2);
                    }
                }
            }

            QSanButton {
                disabledSource: G.getAssetUrl("image/system/button/platter/cancel/disabled.png")
                downSource: G.getAssetUrl("image/system/button/platter/cancel/down.png")
                enabled: dashboard.roomScene !== null && dashboard.roomScene.cancelEnabled
                height: 127
                hoverSource: G.getAssetUrl("image/system/button/platter/cancel/hover.png")
                normalSource: G.getAssetUrl("image/system/button/platter/cancel/normal.png")
                overlayEnabled: false
                width: 129
                x: 2
                y: 206

                onClicked: {
                    var s = dashboard.clientInstance.status;
                    var basic = s & Client.ClientStatusBasicMask;
                    var rs = dashboard.roomScene;

                    if (basic === Client.Playing) {
                        // Just unselect all (no server response for Playing cancel).
                        dashboard.cardArea.unselectAll(null);
                    } else if (basic === Client.Responding || basic === Client.AskForShowOrPindian || basic === Client.Discarding || basic === Client.Exchanging || basic
                               === Client.AskForSkillInvoke || basic === Client.AskForPlayerChoose) {
                        if (rs !== null)
                            rs.cancelResponse(s);
                    }
                    // ExecDialog, AskForSuit/Kingdom etc. deferred.
                }
            }

            QSanButton {
                disabledSource: G.getAssetUrl("image/system/button/platter/discard/disabled.png")
                downSource: G.getAssetUrl("image/system/button/platter/discard/down.png")
                enabled: dashboard.roomScene !== null && dashboard.roomScene.discardEnabled
                height: 138
                hoverSource: G.getAssetUrl("image/system/button/platter/discard/hover.png")
                normalSource: G.getAssetUrl("image/system/button/platter/discard/normal.png")
                overlayEnabled: false
                width: 64
                x: 123
                y: 110

                onClicked: {
                    // Playing: end the play phase by sending an empty card (nullptr).
                    var s = dashboard.clientInstance.status;
                    var basic = s & Client.ClientStatusBasicMask;
                    var rs = dashboard.roomScene;
                    if (basic === Client.Playing && rs !== null)
                        rs.finishPlayPhase();
                }
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
        id: phaseItem

        anchors.bottom: cardBg.top
        anchors.right: cardBg.right
        phase: photo.phase
        visible: photo.gameStarted
    }

    // Judge area (delayed tricks): one icon per card. RoomScene dispatches
    // PlaceDelayedTrick moves to judgeArea.addDelayedTrick/removeDelayedTrick.
    JudgeArea {
        id: judgeArea

        anchors.bottom: cardBg.top
        anchors.left: cardBg.left
    }
}
