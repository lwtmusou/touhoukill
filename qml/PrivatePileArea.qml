import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: privatePileArea

    property string activePile: ""
    property var pileNames: []

    // Private piles (PlaceSpecial): one button per pile, click toggles a dropdown
    // showing the pile's cards. Driven by player.pileChanged (no move dispatch needed --
    // Client::getCards/loseCards calls changePile which emits pile_changed). Pile cardIds
    // may contain -1 (hidden, pileOpen false); CardItem shows card back for cardId=-1.
    property var player: null

    function refresh() {
        if (player === null) {
            privatePileArea.pileNames = [];
            return;
        }
        privatePileArea.pileNames = player.getPileNames();
    }

    function togglePile(name: string) {
        privatePileArea.activePile = (privatePileArea.activePile === name) ? "" : name;
    }

    Component.onCompleted: refresh()
    onPlayerChanged: refresh()

    Connections {
        target: privatePileArea.player

        function onPile_changed(name) {
            privatePileArea.refresh();
        }
    }

    Column {
        spacing: 2

        Repeater {
            model: privatePileArea.pileNames

            Rectangle {
                border.color: "#AAAAAA"
                color: privatePileArea.activePile === modelData ? "#553300" : "#222222"
                height: 24
                width: 80

                Text {
                    anchors.centerIn: parent
                    color: "white"
                    font.pixelSize: 14
                    text: Sanguosha.translate(modelData) + "(" + privatePileArea.player.getPile(modelData).length + ")"
                }

                MouseArea {
                    anchors.fill: parent

                    onClicked: privatePileArea.togglePile(modelData)
                }
            }
        }
    }

    // Dropdown popup showing the active pile's cards. Click outside (on the popup
    // background) to close.
    Rectangle {
        id: dropdown

        anchors.left: parent.left
        anchors.top: parent.bottom
        color: Qt.rgba(0, 0, 0, 0.85)
        height: 256
        visible: privatePileArea.activePile !== ""
        width: Math.max(183, dropdownRow.implicitWidth + 20)

        Row {
            id: dropdownRow

            anchors.centerIn: parent
            spacing: 4

            Repeater {
                model: privatePileArea.activePile !== "" ? privatePileArea.player.getPile(privatePileArea.activePile) : []

                CardItem {
                    cardId: modelData
                    opacity: 1
                }
            }
        }

        MouseArea {
            anchors.fill: parent

            onClicked: privatePileArea.activePile = ""
        }
    }
}
