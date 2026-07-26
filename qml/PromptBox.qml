import QtQuick 6.5

// Prompt box showing the current server prompt text. Appears when the prompt is
// non-empty and the player needs to respond (Responding/Discarding/Exchanging etc.).
// Anchored above Dashboard.cardArea, driven by clientInstance.promptText changes.
Item {
    id: promptBox

    property var clientInstance: null
    property string promptText: clientInstance !== null ? clientInstance.promptText : ""

    height: promptLabel.implicitHeight + 40
    visible: promptText !== "" && (clientInstance !== null)
    width: promptLabel.implicitWidth + 60

    Rectangle {
        anchors.fill: parent
        border.color: "#FFD700"
        border.width: 2
        color: Qt.rgba(0, 0, 0, 0.85)
        radius: 10

        Text {
            id: promptLabel

            anchors.centerIn: parent
            color: "white"
            font.pixelSize: 28
            horizontalAlignment: Text.AlignHCenter
            text: promptBox.promptText
            wrapMode: Text.WordWrap
        }
    }
}
