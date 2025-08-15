import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: roleComboBox

    width: 50
    height: 52

    property bool roleShown: false
    property bool fixed: false
    property string role: "unknown"

    property bool expandFinish: false

    Image {
        id: displayArea

        anchors.fill: parent
        fillMode: Image.PreserveAspectFit

        source: G.getUrl("image/system/roles/unknown.png")

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (!fixed)
                    expandArea.visible = true;
            }
        }
    }

    Image {
        anchors.centerIn: parent
        width: parent.width * 1.3
        height: parent.height * 1.3

        visible: roleComboBox.roleShown
        fillMode: Image.PreserveAspectFit

        source: G.getUrl("image/system/role_shown_icon.png")
    }

    Column {
        id: expandArea

        spacing: 3

        visible: false
        opacity: 0

        anchors.top: displayArea.top
        anchors.horizontalCenter: displayArea.horizontalCenter

        width: roleComboBox.width

        transformOrigin: Item.TopLeft
        transform: Scale {
            id: expandYScale
            yScale: 0.5
        }

        ParallelAnimation {
            id: appearAnimation

            running: false

            PropertyAnimation {
                target: expandArea
                property: "opacity"
                from: 0
                to: 1
                easing.type: Easing.Linear
                duration: 200
            }

            PropertyAnimation {
                target: expandYScale
                property: "yScale"
                from: 0.5
                to: 1
                easing.type: Easing.Linear
                duration: 200
            }

            onFinished: expandFinish = true
        }

        Image {
            anchors.horizontalCenter: parent.horizontalCenter

            width: roleComboBox.width
            height: roleComboBox.height

            fillMode: Image.PreserveAspectFit

            source: G.getUrl("image/system/roles/unknown.png")
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (expandFinish) {
                        expandArea.visible = false;
                        roleComboBox.role = "unknown";
                    }
                }
            }
        }
        Image {
            anchors.horizontalCenter: parent.horizontalCenter

            width: roleComboBox.width
            height: roleComboBox.height

            fillMode: Image.PreserveAspectFit

            source: G.getUrl("image/system/roles/loyalist-1.png")
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (expandFinish) {
                        expandArea.visible = false;
                        roleComboBox.role = "loyalist";
                    }
                }
            }
        }
        Image {
            anchors.horizontalCenter: parent.horizontalCenter

            width: roleComboBox.width
            height: roleComboBox.height

            fillMode: Image.PreserveAspectFit

            source: G.getUrl("image/system/roles/rebel-1.png")
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (expandFinish) {
                        expandArea.visible = false;
                        roleComboBox.role = "rebel";
                    }
                }
            }
        }
        Image {
            anchors.horizontalCenter: parent.horizontalCenter

            width: roleComboBox.width
            height: roleComboBox.height

            fillMode: Image.PreserveAspectFit

            source: G.getUrl("image/system/roles/renegade-1.png")
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (expandFinish) {
                        expandArea.visible = false;
                        roleComboBox.role = "renegade";
                    }
                }
            }
        }

        onVisibleChanged: {
            if (visible) {
                appearAnimation.start();
            } else {
                appearAnimation.stop();
                expandFinish = false;
                expandArea.opacity = 0;
                expandYScale.yScale = 0.5;
            }
        }
    }

    onRoleShownChanged: {
        if (roleShown)
            fixed = true;
    }

    onRoleChanged: {
        var pngname = role;
        if (role !== "unknown")
            pngname = pngname + "-1";

        displayArea.source = G.getUrl("image/system/roles/" + pngname + ".png");
    }

    onFixedChanged: {
        if (fixed)
            expandArea.visible = false;
    }

    Connections {
        target: roomScene
        function onSpaceClicked() {
            expandArea.visible = false;
        }
    }
}
