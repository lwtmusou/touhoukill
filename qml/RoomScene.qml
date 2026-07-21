import QtQuick 6.5

import rocks.touhousatsu 1.0

CppRoomScene {
    id: roomScene

    // Currently active response box (null when none); Dashboard OK button confirms it via activeBox.accept().
    property var activeBox: null

    // arrangementXXX: [0: right, 1: top, 2: left]

    readonly property var arrangement1v2: [//
        [0, 2, 0]//
        , [1, 1, 0]//
        , [0, 1, 1],//
    ]
    readonly property var arrangement1v3: [//
        [0, 3, 0]//
        , [2, 1, 0]//
        , [1, 1, 1]//
        , [0, 1, 2],//
    ]
    readonly property var arrangement2v2: [//
        [0, 2, 1] // seat mod 2 = 0 (seat = 2 or 4)
        , [1, 2, 0], // seat mod 2 = 1 (seat = 1 or 3)
    ]
    readonly property var arrangement3v3: [//
        [0, 3, 2]// seat mod 3 = 0 (seat = 3 or 6)
        , [1, 3, 1]// seat mod 3 = 1 (seat = 1 or 4)
        , [2, 3, 0], // seat mod 3 = 2 (seat = 2 or 5)
    ]
    readonly property var arrangementRegular: [//
        [0, 1, 0]//
        , [1, 0, 1]//
        , [1, 1, 1]//
        , [1, 2, 1]//
        , [1, 3, 1]//
        , [2, 2, 2]//
        , [2, 3, 2]//
        , [2, 4, 2]//
        , [2, 5, 2],//
    ]
    property int layBorderMargin: 20
    property list<Photo> otherPhotos
    readonly property int zzzWorkaroundQTBUG147713: 0

    signal spaceClicked

    function lay() {
        var playerCount = Sanguosha.getPlayerCount(ServerInfo.GameMode);
        var verticalAlignment = Qt.AlignVCenter;
        var arrangement = arrangementRegular[playerCount - 2];

        if (!G.isNormalGameMode(ServerInfo.GameMode) && !G.isHegemonyGameMode(ServerInfo.GameMode))
            verticalAlignment = Qt.AlignBottom;

        if (ServerInfo.GameMode == "04_2v2")
            arrangement = arrangement2v2[selfPhoto.seat % 2];
        else if (ServerInfo.GameMode == "03_1v2")
            arrangement = arrangement1v2[selfPhoto.seat - 1];
        else if (ServerInfo.GameMode == "04_1v3")
            arrangement = arrangement1v3[selfPhoto.seat - 1];
        else if (ServerInfo.GameMode == "06_3v3")
            arrangement = arrangement3v3[selfPhoto.seat % 3];

        var effectiveSeat;
        var inRight;
        var inTop;
        var inLeft;
        var rightPosition;
        var topPosition;
        var leftPosition;
        var verticalTopMargin;
        var verticalSpacing;
        var horizontalSpacing;

        var photo;

        for (photo of otherPhotos) {
            // photo.seat can't be equal to selfPhoto.seat so following effectiveSeat can't be 0
            effectiveSeat = (photo.seat - selfPhoto.seat + playerCount) % playerCount;
            inRight = (arrangement[0] >= effectiveSeat);
            if (inRight) {
                rightPosition = arrangement[0] - effectiveSeat;
                photo.originalX = roomScene.width - photo.width - layBorderMargin;

                if (verticalAlignment === Qt.AlignVCenter) {
                    verticalSpacing = (roomScene.height / (arrangement[0] + 1)) - photo.height;
                    photo.originalY = verticalSpacing * (rightPosition + 1) + photo.height * rightPosition;
                } else {
                    verticalSpacing = 2 * layBorderMargin;
                    verticalTopMargin = roomScene.height - arrangement[0] * verticalSpacing - (arrangement[0] + 1) * photo.height;
                    photo.originalY = verticalTopMargin + (verticalSpacing + photo.height) * rightPosition;
                }
                continue;
            }

            inTop = (arrangement[1] >= (effectiveSeat - arrangement[0]));
            if (inTop) {
                topPosition = arrangement[1] - (effectiveSeat - arrangement[0]);
                topPosition += 1;
                photo.originalY = layBorderMargin;

                horizontalSpacing = (roomScene.width - 2 * layBorderMargin - (arrangement[1] + 2) * photo.width) / (arrangement[1] + 1);
                photo.originalX = layBorderMargin + (horizontalSpacing + photo.width) * topPosition;

                continue;
            }

            inLeft = (arrangement[2] >= (effectiveSeat - arrangement[0] - arrangement[1]));
            if (inLeft) {
                leftPosition = arrangement[2] - (effectiveSeat - arrangement[0] - arrangement[1]);
                leftPosition = -leftPosition + arrangement[2] - 1;
                photo.originalX = layBorderMargin;

                if (verticalAlignment === Qt.AlignVCenter) {
                    verticalSpacing = (roomScene.height / (arrangement[2] + 1)) - photo.height;
                    photo.originalY = verticalSpacing * (leftPosition + 1) + photo.height * leftPosition;
                } else {
                    verticalSpacing = 2 * layBorderMargin;
                    verticalTopMargin = roomScene.height - arrangement[2] * verticalSpacing - (arrangement[2] + 1) * photo.height;
                    photo.originalY = verticalTopMargin + (verticalSpacing + photo.height) * leftPosition;
                }

                continue;
            }

            // unreachable!
            console.log("unreachable - RoomScene.qml - lay()" + ", effectiveSeat = " + effectiveSeat + ", photo.seat = " + photo.seat + ", selfPhoto.seat = " + selfPhoto.seat
                        + ", ServerInfo.GameMode = " + ServerInfo.GameMode + ", playerCount = " + playerCount);
        }

        for (photo of otherPhotos) {
            photo.x = photo.originalX;
            photo.y = photo.originalY;
            photo.visible = true;
        }
    }

    Component.onCompleted: {
        backgroundImage.source = G.getAssetUrl(Config.TableBgImage);

        var playercount = Sanguosha.getPlayerCount(ServerInfo.GameMode);

        for (var i = 1; i < playercount; ++i) {
            var photo = photoComponent.createObject(this, {
                                                        seat: i + 1,
                                                        selfPhoto: false,
                                                        gameStarted: Qt.binding(function () {
                                                            return roomScene.gameStarted;
                                                        })
                                                    });
            otherPhotos.push(photo);
        }

        lay();
    }
    onWidthChanged: lay()

    MouseArea {
        anchors.fill: parent

        onClicked: roomScene.spaceClicked()
    }

    // C++ -> QML notification bridge.
    // Receives every notify* signal forwarded by CppRoomScene (src/qmlui/roomscene.cpp).
    Connections {
        function onNotifyAgCleared() {
            console.log("[bridge->qml] notifyAgCleared");
        }

        function onNotifyAgFilled(cardIds, disabledIds, shownHandcardIds) {
            console.log("[bridge->qml] notifyAgFilled", cardIds.length);
        }

        function onNotifyAgTaken(taker, cardId, moveCards) {
            console.log("[bridge->qml] notifyAgTaken", cardId);
        }

        function onNotifyAnimated(name, args) {
            console.log("[bridge->qml] notifyAnimated", name, args);
        }

        function onNotifyArrangeStarted(toArrange) {
            console.log("[bridge->qml] notifyArrangeStarted", toArrange);
        }

        function onNotifyAssignAsked() {
            roomScene.showRoleAssignDialog();
        }

        function onNotifyCardShown(playerName, cardId) {
            console.log("[bridge->qml] notifyCardShown", playerName, cardId);
        }

        function onNotifyCardsGot(player, flags, reason, handcardVisible, method, disabledIds, enableEmptyCard) {
            console.log("[bridge->qml] notifyCardsGot", reason, flags, method);
        }

        function onNotifyDashboardDeath(who) {
            console.log("[bridge->qml] notifyDashboardDeath", who);
        }

        function onNotifyDeputyPreshowed() {
            console.log("[bridge->qml] notifyDeputyPreshowed");
        }

        function onNotifyDirectionsGot() {
            console.log("[bridge->qml] notifyDirectionsGot");
        }

        function onNotifyEmotionSet(target, emotion) {
            console.log("[bridge->qml] notifyEmotionSet", target, emotion);
        }

        // Global game event dispatch hub (Client::event_received forwarded as-is).
        // Skill-related events -> skillDock.handleSkillEvent; other event types TODO.
        function onNotifyEventReceived(args) {
            skillDock.handleSkillEvent(args);
        }

        function onNotifyFocusMoved(focus, countdown) {
            console.log("[bridge->qml] notifyFocusMoved", focus);
        }

        function onNotifyGameOver() {
            roomScene.showGameOverDialog(false);
        }

        function onNotifyGameStarted() {
            console.log("[bridge->qml] notifyGameStarted");
        }

        function onNotifyGeneralAsked() {
            console.log("[bridge->qml] notifyGeneralAsked");
        }

        function onNotifyGeneralRecovered(index, name) {
            console.log("[bridge->qml] notifyGeneralRecovered", index, name);
        }

        function onNotifyGeneralRevealed(self, general) {
            console.log("[bridge->qml] notifyGeneralRevealed", self, general);
        }

        function onNotifyGeneralTaken(who, name, rule) {
            console.log("[bridge->qml] notifyGeneralTaken", who, name);
        }

        function onNotifyGeneralsFilled(generalNames) {
            console.log("[bridge->qml] notifyGeneralsFilled", generalNames.length);
        }

        function onNotifyGeneralsGot(generals, singleResult, canConvert) {
            var box = chooseGeneralBoxComponent.createObject(roomScene, {
                                                                 "generals": generals,
                                                                 "singleResult": singleResult
                                                             });
            roomScene.activeBox = box;
            box.generalChosen.connect(function (name) {
                roomScene.ClientInstance.onPlayerChooseGeneral(name);
            });
        }

        function onNotifyGeneralsViewed(reason, names) {
            console.log("[bridge->qml] notifyGeneralsViewed", reason, names);
        }

        function onNotifyGongxin(cardIds, enableHeart, enabledIds, shownHandcardIds) {
            console.log("[bridge->qml] notifyGongxin", cardIds.length);
        }

        function onNotifyGuanxing(cardIds, singleSide, skillName) {
            console.log("[bridge->qml] notifyGuanxing", skillName, cardIds.length);
        }

        function onNotifyHeadPreshowed() {
            console.log("[bridge->qml] notifyHeadPreshowed");
        }

        function onNotifyKingdomsGot(kingdoms) {
            console.log("[bridge->qml] notifyKingdomsGot", kingdoms);
        }

        function onNotifyLineSpoken(line) {
            console.log("[bridge->qml] notifyLineSpoken", line);
        }

        function onNotifyLogReceived(logStr) {
            console.log("[bridge->qml] notifyLogReceived", logStr);
        }

        // Subtask A: hand area sync. Handle moves where toPlace/fromPlace == PlaceHand && player == Self.
        // Other places (equip/judge/pile/table) deferred to later subtasks.
        function onNotifyMoveCardsGot(moveId, moves) {
            for (var i = 0; i < moves.length; ++i) {
                var m = moves[i];
                if (m.toPlace === Player.PlaceHand && m.toPlayer !== null && m.toPlayer.objectName === roomScene.Self.objectName) {
                    for (var j = 0; j < m.cardIds.length; ++j)
                        dashboard.addHandCard(m.cardIds[j]);
                }
            }
        }

        function onNotifyMoveCardsLost(moveId, moves) {
            for (var i = 0; i < moves.length; ++i) {
                var m = moves[i];
                if (m.fromPlace === Player.PlaceHand && m.fromPlayer !== null && m.fromPlayer.objectName === roomScene.Self.objectName) {
                    for (var j = 0; j < m.cardIds.length; ++j)
                        dashboard.removeHandCard(m.cardIds[j]);
                }
            }
        }

        function onNotifyNullificationAsked(asked) {
            console.log("[bridge->qml] notifyNullificationAsked", asked);
        }

        function onNotifyOptionsGot(skillName, options) {
            console.log("[bridge->qml] notifyOptionsGot", skillName, options);
        }

        function onNotifyOrdersGot(reason) {
            console.log("[bridge->qml] notifyOrdersGot", reason);
        }

        function onNotifyPerspectiveChanged(targetName, handCardIds, piles) {
            console.log("[bridge->qml] notifyPerspectiveChanged", targetName);
        }

        function onNotifyPlayerAdded(newPlayer) {
            for (var i = 0; i < otherPhotos.length; ++i) {
                if (otherPhotos[i].player === null) {
                    otherPhotos[i].player = newPlayer;
                    break;
                }
            }
        }

        function onNotifyPlayerKilled(who) {
            console.log("[bridge->qml] notifyPlayerKilled", who);
        }

        function onNotifyPlayerRemoved(playerName) {
            for (var i = 0; i < otherPhotos.length; ++i) {
                var p = otherPhotos[i].player;
                if (p !== null && p.objectName === playerName) {
                    otherPhotos[i].player = null;
                    break;
                }
            }
        }

        function onNotifyPlayerRevived(who) {
            console.log("[bridge->qml] notifyPlayerRevived", who);
        }

        function onNotifyPlayerSpoken(who, line) {
            console.log("[bridge->qml] notifyPlayerSpoken", who, line);
        }

        function onNotifyRoleStateChanged(stateStr) {
            console.log("[bridge->qml] notifyRoleStateChanged", stateStr);
        }

        function onNotifyRolesGot(scheme, roles) {
            console.log("[bridge->qml] notifyRolesGot", scheme, roles);
        }

        function onNotifySeatsArranged() {
            // Seat is assigned by arrangeSeats; read it back from each bound Photo.player.
            for (var i = 0; i < otherPhotos.length; ++i) {
                if (otherPhotos[i].player !== null)
                    otherPhotos[i].seat = otherPhotos[i].player.seat;
            }
            if (roomScene.Self !== null)
                selfPhoto.seat = roomScene.Self.seat;
            roomScene.lay();
        }

        function onNotifySkillAcquired(player, skillName, head) {
            console.log("[bridge->qml] notifySkillAcquired", skillName, head);
        }

        function onNotifySkillAttached(skillName, fromLeft) {
            console.log("[bridge->qml] notifySkillAttached", skillName, fromLeft);
        }

        function onNotifySkillDetached(skillName, head) {
            console.log("[bridge->qml] notifySkillDetached", skillName, head);
        }

        function onNotifySkillInvalidityChanged(player) {
            console.log("[bridge->qml] notifySkillInvalidityChanged");
        }

        function onNotifySkillInvoked(who, skillName) {
            console.log("[bridge->qml] notifySkillInvoked", who, skillName);
        }

        function onNotifyStandoff() {
            roomScene.showGameOverDialog(true);
        }

        function onNotifyStartInXs() {
            console.log("[bridge->qml] notifyStartInXs");
        }

        function onNotifyStatusChanged(newStatus) {
            console.log("[bridge->qml] notifyStatusChanged", newStatus);
        }

        function onNotifySuitsGot(suits) {
            console.log("[bridge->qml] notifySuitsGot", suits);
        }

        function onNotifySurrenderEnabled(enabled) {
            console.log("[bridge->qml] notifySurrenderEnabled", enabled);
        }

        function onNotifyTextSpoken(text) {
            console.log("[bridge->qml] notifyTextSpoken", text);
        }

        function onNotifyTriggersGot(options, optional) {
            console.log("[bridge->qml] notifyTriggersGot", options.length, optional);
        }

        target: roomScene
    }

    Dashboard {
        id: dashboard

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        photo: selfPhoto
        roomScene: roomScene
        width: parent.width - selfPhoto.width
    }

    Photo {
        id: selfPhoto

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        gameStarted: roomScene.gameStarted
        player: roomScene.Self
        seat: 1
        selfPhoto: true
    }

    // Self skill button dock: single dock above selfPhoto. Skill-specific notifies
    // (attached/detached/acquired/invalidity_changed) are handled inside SkillDock;
    // global game events arrive via onNotifyEventReceived below -> skillDock.handleSkillEvent.
    SkillDock {
        id: skillDock

        anchors.bottom: selfPhoto.top
        anchors.horizontalCenter: selfPhoto.horizontalCenter
        roomScene: roomScene
        visible: roomScene.gameStarted
        width: selfPhoto.width
    }

    Column {
        anchors.centerIn: parent
        spacing: roomScene.height * 0.01
        visible: !roomScene.gameStarted && !roomScene.gameOver

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            height: roomScene.height / 15
            width: roomScene.width / 6

            QSanButton {
                id: addRobotButton

                anchors.fill: parent
                font.pixelSize: 50
                normalSource: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Add Robot")
                visible: !roomScene.gameStarted && roomScene.Self.owner && ServerInfo.EnableAI

                onClicked: roomScene.ClientInstance.addRobot()
            }
        }

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            height: roomScene.height / 15
            width: roomScene.width / 6

            QSanButton {
                id: fillRobotsButton

                anchors.fill: parent
                font.pixelSize: 50
                normalSource: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Fill Robot")
                visible: !roomScene.gameStarted && roomScene.Self.owner && ServerInfo.EnableAI

                onClicked: roomScene.ClientInstance.fillRobots()
            }
        }

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            height: roomScene.height / 30
            width: roomScene.width / 6
        }

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            height: roomScene.height / 15
            width: roomScene.width / 6

            QSanButton {
                id: startSceneButton

                anchors.fill: parent
                font.pixelSize: 50
                normalSource: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Return to main menu")
                visible: !roomScene.gameStarted && !roomScene.gameOver

                onClicked: MainWindowInstance.gotoStartScene()
            }
        }
    }

    Rectangle {
        id: testItemToBeRemovedAfterTest

        anchors.centerIn: parent
        color: Qt.rgba(0, 0, 0, 0.9)
        height: 600
        visible: !gameStarted
        width: parent.width - 2 * 336 - 2 * 20

        Row {
            anchors.fill: parent

            Photo {
                banling: false
                gameStarted: true
                general: "luize"
                kingdom: "pc98"
                phase: Player.Start
                seat: 3
                selfPhoto: false
            }

            Photo {
                gameStarted: true
                general: "yingyingguai"
                general2: "yingyingguai"
                huashenGeneral: "sujiangf"
                huashenGeneral2: "sujiang"
                kingdom: "hmx"
                maxhp: 600
                seat: 5
                selfPhoto: false
            }

            Photo {
                general: "youmu_god"
                hp: 10
                kingdom: "touhougod"
                linghp: 20
                maxhp: 30
                seat: 7
                selfPhoto: false
            }

            CardItem {
                cardId: 0
                opacity: 1
            }

            CardItem {
                general: "remilia"
                opacity: 1
            }

            Column {
                PhaseItem {
                    phase: Player.Start
                }

                PhaseItem {
                    phase: Player.RoundStart
                }

                PhaseItem {
                    phase: Player.NotActive
                }

                PhaseItem {
                    phase: Player.Play
                }
            }
        }
    }

    Component {
        id: photoComponent

        Photo {
            seat: 0
        }
    }

    Component {
        id: chooseGeneralBoxComponent

        ChooseGeneralBox {
        }
    }
}
