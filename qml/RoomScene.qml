import QtQuick 6.5

import rocks.touhousatsu 1.0

CppRoomScene {
    id: roomScene

    signal spaceClicked

    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        source: G.getUrl(Config.TableBgImage)

        MouseArea {
            anchors.fill: parent

            onClicked: roomScene.spaceClicked()
        }
        QSanButton {
            id: startSceneButton

            anchors.centerIn: parent
            font.pixelSize: 50
            height: parent.height / 4
            source: G.getUrl("image/system/button/button.png")
            text: "return to start scene"
            width: parent.width / 4

            onClicked: MainWindowInstance.gotoStartScene()
        }
        Rectangle {
            anchors.bottom: startSceneButton.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            color: Qt.rgba(0, 0, 0, 0.9)
            height: roomScene.height
            width: roomScene.width

            Row {
                anchors.fill: parent

                HandcardNum {
                    kingdom: "touhougod"
                    num: 1
                }
                HandcardNum {
                    kingdom: "zhan"
                    num: 1
                }
                HandcardNum {
                    kingdom: "hmx"
                    num: 1
                }
                HandcardNum {
                    kingdom: "yym"
                    num: 3
                }
                HandcardNum {
                    kingdom: "pc98"
                    num: 5
                }
                HandcardNum {
                    kingdom: "tkz"
                    num: 7
                }
                KingdomImage {
                    kingdom: "hzc"
                }
                KingdomImage {
                    kingdom: "hmx"
                }
                KingdomImage {
                    kingdom: "pc98"
                }
                Photo {
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
                    banling: true
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
            }
        }
    }
}
