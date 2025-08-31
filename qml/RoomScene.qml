import QtQuick 6.5

import rocks.touhousatsu 1.0

CppRoomScene {
    id: roomScene

    signal spaceClicked

    Image {
        source: G.getUrl(Config.TableBgImage)
        fillMode: Image.PreserveAspectCrop

        anchors.fill: parent

        MouseArea {
            anchors.fill: parent
            onClicked: roomScene.spaceClicked()
        }

        QSanButton {
            id: startSceneButton

            anchors.centerIn: parent
            width: parent.width / 4
            height: parent.height / 4

            text: "return to start scene"
            font.pixelSize: 50

            source: G.getUrl("image/system/button/button.png")

            onClicked: MainWindowInstance.gotoStartScene()
        }

        Rectangle {
            anchors.top: startSceneButton.bottom
            anchors.horizontalCenter: parent.horizontalCenter

            color: Qt.rgba(0, 0, 0, 0.9)
            width: roomScene.width
            height: roomScene.height

            Row {
                anchors.fill: parent

                HandcardNum {
                    num: 1
                    kingdom: "touhougod"
                }
                HandcardNum {
                    num: 1
                    kingdom: "zhan"
                }
                HandcardNum {
                    num: 1
                    kingdom: "hmx"
                }
                HandcardNum {
                    num: 3
                    kingdom: "yym"
                }
                HandcardNum {
                    num: 5
                    kingdom: "pc98"
                }
                HandcardNum {
                    num: 7
                    kingdom: "tkz"
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
                    general: "luize"
                    kingdom: "pc98"
                    phase: Player.Start
                }

                Photo {
                    general: "yingyingguai"
                    kingdom: "touhougod"
                    huashenGeneral: "sujiangf"
                }

                Column{
                    PhaseItem{
                        phase : Player.Start
                    }
                    PhaseItem {
                        phase : Player.RoundStart
                    }
                    PhaseItem {
                        phase : Player.NotActive
                    }
                    PhaseItem {
                        phase : Player.Play
                    }
                }

            }
        }
    }
}
