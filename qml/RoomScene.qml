import QtQuick 6.5

import rocks.touhousatsu 1.0

CppRoomScene {
    id: roomScene

    property int layBorderMargin: 20
    property list<Photo> otherPhotos

    signal spaceClicked

    function lay() {
        // TODO: replace following with static variable (Does QML support static variable?)
        // [0: right, 1: top, 2: left]
        const arrangementRegular = [[0, 1, 0] //
                                    , [1, 0, 1] //
                                    , [1, 1, 1] //
                                    , [1, 2, 1] //
                                    , [1, 3, 1] //
                                    , [2, 2, 2] //
                                    , [2, 3, 2] //
                                    , [2, 4, 2] //
                                    , [2, 5, 2], //
              ];
        const arrangement2v2 = [[0, 2, 1] // seat mod 2 = 0 (seat = 2 or 4)
                                , [1, 2, 0], // seat mod 2 = 1 (seat = 1 or 3)
              ];
        const arrangement1v2 = [[0, 2, 0]//
                                , [1, 1, 0]//
                                , [0, 1, 1],//
              ];
        const arrangement1v3 = [[0, 3, 0]//
                                , [2, 1, 0]//
                                , [1, 1, 1]//
                                , [0, 1, 2],//
              ];
        const arrangement3v3 = [[0, 3, 2]// seat mod 3 = 0 (seat = 3 or 6)
                                , [1, 3, 1]// seat mod 3 = 1 (seat = 1 or 4)
                                , [2, 3, 0], // seat mod 3 = 2 (seat = 2 or 5)
              ];

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
                photo.originalX = roomScene.width - photo.width - roomScene.layBorderMargin;

                if (verticalAlignment === Qt.AlignVCenter) {
                    verticalSpacing = (roomScene.height / (arrangement[0] + 1)) - photo.height;
                    photo.originalY = verticalSpacing * (rightPosition + 1) + photo.height * rightPosition;
                } else {
                    verticalSpacing = 40;
                    verticalTopMargin = roomScene.height - arrangement[0] * verticalSpacing - (arrangement[0] + 1) * photo.height;
                    photo.originalY = verticalTopMargin + (verticalSpacing + photo.height) * rightPosition;
                }
                continue;
            }

            inTop = (arrangement[1] >= (effectiveSeat - arrangement[0]));
            if (inTop) {
                topPosition = arrangement[1] - (effectiveSeat - arrangement[0]);
                topPosition += 1;
                photo.originalY = roomScene.layBorderMargin;

                horizontalSpacing = (roomScene.width - 2 * roomScene.layBorderMargin - (arrangement[1] + 2) * photo.width) / (arrangement[1] + 1);
                photo.originalX = roomScene.layBorderMargin + (horizontalSpacing + photo.width) * topPosition;

                continue;
            }

            inLeft = (arrangement[2] >= (effectiveSeat - arrangement[0] - arrangement[1]));
            if (inLeft) {
                leftPosition = arrangement[2] - (effectiveSeat - arrangement[0] - arrangement[1]);
                leftPosition = -leftPosition + arrangement[2] - 1;
                photo.originalX = roomScene.layBorderMargin;

                if (verticalAlignment === Qt.AlignVCenter) {
                    verticalSpacing = (roomScene.height / (arrangement[2] + 1)) - photo.height;
                    photo.originalY = verticalSpacing * (leftPosition + 1) + photo.height * leftPosition;
                } else {
                    verticalSpacing = 40;
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
        backgroundImage.source = G.getUrl(Config.TableBgImage);

        var playercount = Sanguosha.getPlayerCount(ServerInfo.GameMode);

        for (var i = 1; i < playercount; ++i) {
            var photo = photoComponent.createObject(this, {
                                                        seat: i + 1,
                                                        gameStarted: Qt.binding(function() {return roomScene.gameStarted;})
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

    Dashboard {
        id: dashboard

        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: parent.width - selfPhoto.width
    }

    Photo {
        id: selfPhoto

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        gameStarted: roomScene.gameStarted
        player: roomScene.Self
        seat: 1
    }


    Column {
        spacing: roomScene.height * 0.01
        anchors.centerIn: parent
        visible: !roomScene.gameStarted && !roomScene.gameOver

        Item {
            anchors.horizontalCenter: parent.horizontalCenter
            height: roomScene.height / 15
            width: roomScene.width / 6

            QSanButton {
                id: addRobotButton

                anchors.fill: parent
                font.pixelSize: 50
                source: G.getUrl("image/system/button/button.png")
                text: qsTr("Add Robot")

                visible: !roomScene.gameStarted && roomScene.Self.owner

                // onClicked: roomScene.addRobot() // not implemented
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
                source: G.getUrl("image/system/button/button.png")
                text: qsTr("Fill Robot")

                visible: !roomScene.gameStarted && roomScene.Self.owner

                // onClicked: roomScene.addRobot() // not implemented
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
                source: G.getUrl("image/system/button/button.png")
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
        visible: true
        width: parent.width-2*336-2*20
        height: 600

        Row {
            anchors.fill: parent

            Photo {
                banling: false
                gameStarted: true
                general: "luize"
                kingdom: "pc98"
                phase: Player.Start
                seat: 3
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
            }

            Photo {
                general: "youmu_god"
                hp: 10
                kingdom: "touhougod"
                linghp: 20
                maxhp: 30
                seat: 7
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

            QSanButton {
                source: G.getUrl("image/system/button/button.png")
                text: "toggle"
                height: 100
                width: 100

                onClicked: roomScene.gameStarted = !roomScene.gameStarted
            }
        }
    }

    Component {
        id: photoComponent

        Photo {
            seat: 0
        }
    }
}
