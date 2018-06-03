import QtQuick 2.6

Image {
    id: qSanButton
    property bool checkable: false
    property bool checked: false

    property bool enabled: true

    property url upSource
    property url upHoveredSource
    property url downSource
    property url downHoveredSource
    property url disabledSource

    property string text
    property font font

    signal clicked()
    signal doubleClicked()

    onClicked: {
        if (checkable)
            checked = !checked;
    }

    state: "exited"

    states: [
        State {
            name: "exited"
            PropertyChanges {
                target: qSanButton
                source: qSanButton.upSource
            }
        },
        State {
            name: "entered"
            PropertyChanges {
                target: qSanButton
                source: qSanButton.upHoveredSource
            }
        },
        State {
            name: "downEntered"
            PropertyChanges {
                target: qSanButton
                source: qSanButton.downHoveredSource
            }
        },
        State {
            name: "downExited"
            PropertyChanges {
                target: qSanButton
                source: qSanButton.downSource
            }
        },
        State {
            name: "disabled"
            PropertyChanges {
                target: qSanButton
                source: qSanButton.disabledSource
            }
        }
    ]

    onEnabledChanged: {
        if (!enabled)
            state = "disabled"
        else
            state = "exited"
    }

    Text {
        anchors.fill: parent
        clip: true
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: parent.text
        font: parent.font
    }

    MouseArea {
        id: qSanButtonMouseArea
        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            if (parent.enabled) {
                parent.clicked()
                if (!parent.checkable || !parent.checked)
                    parent.state = "entered"
                else
                    parent.state = "downEntered"
            }
        }

        onDoubleClicked: {
            if (parent.enabled) {
                parent.doubleClicked()
            }
        }

        onEntered: {
            if (parent.enabled) {
                if (!parent.checkable || !parent.checked)
                    parent.state = "entered"
                else
                    parent.state = "downEntered"
            }
        }

        onExited: {
            if (parent.enabled) {
                if (!parent.checkable || !parent.checked)
                    parent.state = "exited"
                else
                    parent.state = "downExited"
            }
        }

        onPressed: {
            if (parent.enabled) {
                parent.state = "downEntered"
            } else {
                mouse.accepted = false
            }
        }

        onReleased:  {
            if (parent.enabled) {
                if (!parent.checkable || !parent.checked)
                    parent.state = "entered"
                else
                    parent.state = "downEntered"
            }
        }
    }
}
