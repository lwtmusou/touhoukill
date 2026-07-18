import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    id: qSanButton

    property bool checkable: false
    property bool checked: false
    property font font
    property string text

    signal clicked
    signal doubleClicked

    font.family: G.ButtonFontFace
    state: "exited"

    states: [
        State {
            name: "exited"

            PropertyChanges {
                hover.visible: false
            }
        },
        State {
            name: "entered"

            PropertyChanges {
                hover.visible: true
            }
        },
        State {
            name: "downEntered"

            PropertyChanges {
                hover.visible: true
            }
        },
        State {
            name: "downExited"

            PropertyChanges {
                hover.visible: true
            }
        },
        State {
            name: "disabled"

            PropertyChanges {
                hover.visible: true
            }
        }
    ]

    onClicked: {
        if (checkable)
            checked = !checked;
    }
    onEnabledChanged: {
        if (!enabled)
            state = "disabled";
        else
            state = "exited";
    }

    Rectangle {
        id: hover

        anchors.fill: parent
        color: Qt.rgba(1, 1, 1, .25)
        visible: false
    }

    Text {
        anchors.fill: parent
        font: parent.font
        fontSizeMode: Text.Fit
        horizontalAlignment: Text.AlignHCenter
        text: parent.text
        textFormat: Text.PlainText
        verticalAlignment: Text.AlignVCenter
    }

    MouseArea {
        id: qSanButtonMouseArea

        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            if (parent.enabled) {
                parent.clicked();
                if (!parent.checkable || !parent.checked)
                    parent.state = "entered";
                else
                    parent.state = "downEntered";
            }
        }
        onDoubleClicked: {
            if (parent.enabled) {
                parent.doubleClicked();
            }
        }
        onEntered: {
            if (parent.enabled) {
                if (!parent.checkable || !parent.checked)
                    parent.state = "entered";
                else
                    parent.state = "downEntered";
            }
        }
        onExited: {
            if (parent.enabled) {
                if (!parent.checkable || !parent.checked)
                    parent.state = "exited";
                else
                    parent.state = "downExited";
            }
        }
        onPressed: mouse => {
            if (parent.enabled) {
                parent.state = "downEntered";
            } else {
                mouse.accepted = false;
            }
        }
        onReleased: {
            if (parent.enabled) {
                if (!parent.checkable || !parent.checked)
                    parent.state = "entered";
                else
                    parent.state = "downEntered";
            }
        }
    }
}
