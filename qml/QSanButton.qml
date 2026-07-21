import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    id: qSanButton

    property bool checkable: false
    property bool checked: false
    property url disabledSource
    property url downSource
    property font font
    property url hoverSource
    // Per-state image sources. Callers set all four for platter buttons, or just
    // normalSource for single-image buttons (others fallback to normalSource).
    // QSanButton picks source by state; no path concatenation or filename convention inside.
    property url normalSource
    // Whether to show the hover overlay Rectangle and Text label. Set false for platter
    // buttons (they carry icons + hover state in their own image, no overlay/text needed).
    property bool overlayEnabled: true
    property string text

    signal clicked
    signal doubleClicked

    font.family: G.ButtonFontFace
    source: {
        switch (qSanButton.state) {
        case "disabled":
            return disabledSource.toString() !== "" ? disabledSource : normalSource;
        case "downEntered":
        case "downExited":
            return downSource.toString() !== "" ? downSource : normalSource;
        case "entered":
            return hoverSource.toString() !== "" ? hoverSource : normalSource;
        default:
            return normalSource;
        }
    }
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
                hover.color: Qt.rgba(1, 1, 1, .25)
                hover.visible: true
            }
        },
        State {
            name: "downEntered"

            PropertyChanges {
                hover.color: Qt.rgba(1, 1, 1, .25)
                hover.visible: true
            }
        },
        State {
            name: "downExited"

            PropertyChanges {
                hover.color: Qt.rgba(1, 1, 1, .25)
                hover.visible: true
            }
        },
        State {
            name: "disabled"

            PropertyChanges {
                hover.color: Qt.rgba(0, 0, 0, .25)
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
        // Hide hover overlay in platter mode (platter has its own hover state image).
        opacity: overlayEnabled ? 1 : 0
        visible: false
    }

    Text {
        anchors.fill: parent
        font: parent.font
        fontSizeMode: Text.Fit
        horizontalAlignment: Text.AlignHCenter
        // Hide text in platter mode (platter images carry icons, no text label).
        text: parent.text
        textFormat: Text.PlainText
        verticalAlignment: Text.AlignVCenter
        visible: overlayEnabled
    }

    MouseArea {
        id: qSanButtonMouseArea

        anchors.fill: parent
        hoverEnabled: true

        onClicked: {
            if (parent.enabled) {
                parent.clicked();
                // re-check enabled: clicked handler may have disabled the button
                // (e.g. OK accept() -> status change -> okEnabled=false). Without this,
                // state would be set to "entered" after onEnabledChanged set "disabled",
                // leaving the hover image stuck.
                if (parent.enabled) {
                    if (!parent.checkable || !parent.checked)
                        parent.state = "entered";
                    else
                        parent.state = "downEntered";
                }
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
