import QtQuick 6.5
import rocks.touhousatsu 1.0

Item {
    id: dashboard

    property bool cancelEnabled: false
    property var clientInstance: parent ? parent.ClientInstance : null
    property bool discardEnabled: false
    property bool okEnabled: false
    required property var photo

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

        // TODO: Switch to QSanSkillButton!
        Image {
            id: weaponArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 72
            height: 51
            visible: false
            width: 307
        }

        Image {
            id: armorArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 129
            height: 51
            visible: false
            width: 307
        }

        Image {
            id: dhorseArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 186
            height: 51
            visible: false
            width: 307
        }

        Image {
            id: ohorseArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 243
            height: 51
            visible: false
            width: 307
        }

        Image {
            id: treasureArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 300
            height: 51
            visible: false
            width: 307
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

            anchors.fill: parent
        }

        // Operation buttons floating above the hand area (negative y).
        // Trust is fully functional (Client::trust slot); OK/Cancel/Discard depend on CardItem
        // selection & target choosing (CardItem.selected is currently commented out), so they are
        // UI stubs with TODO until card selection lands.
        Row {
            anchors.bottom: cardArea.top
            anchors.bottomMargin: 8
            anchors.horizontalCenter: cardArea.horizontalCenter
            spacing: 8

            QSanButton {
                enabled: dashboard.clientInstance != null
                font.pixelSize: 50
                height: 133
                source: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Trust")
                width: 268

                onClicked: dashboard.clientInstance.trust()
            }

            QSanButton {
                // Enabled in Playing phase (used to end the play / skip). Discarding-phase
                // discard submits via OK after card selection (TODO: wire onPlayerDiscardCards
                // once CardItem selection lands).
                enabled: dashboard.discardEnabled
                font.pixelSize: 50
                height: 133
                source: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Discard")
                width: 268

                // TODO: dashboard.clientInstance.onPlayerDiscardCards(selectedCard)
            }

            QSanButton {
                // Enabled in ExecDialog / AskForSkillInvoke, or in Responding/Discarding/Exchanging
                // when the discard action is refusable (Client::discardActionRefusable).
                enabled: dashboard.cancelEnabled
                font.pixelSize: 50
                height: 133
                source: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Cancel")
                width: 268

                // TODO: doCancelButton based on ClientInstance.status
            }

            QSanButton {
                // OK: confirms activeBox selection, or acknowledges AskForSkillInvoke.
                // Other response statuses enable OK after card selection lands (CardItem.selected TODO).
                enabled: dashboard.okEnabled
                font.pixelSize: 50
                height: 133
                source: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("OK")
                width: 268

                onClicked: {
                    if (dashboard.parent && dashboard.parent.activeBox !== null)
                        dashboard.parent.activeBox.accept();
                    // TODO: handle AskForSkillInvoke / card-response OK once wired
                }
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
