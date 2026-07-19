import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: roleComboBox

    property bool expandFinish: false
    property bool fixed: false
    property string role: "unknown"
    property bool roleShown: false

    height: 52
    width: 50

    onFixedChanged: {
        if (fixed)
            expandArea.visible = false;
    }
    onRoleChanged: {
        var pngname = role;
        if (role !== "unknown")
            pngname = pngname + "-1";

        displayArea.source = G.getAssetUrl("image/system/roles/" + pngname + ".png");
    }
    onRoleShownChanged: {
        if (roleShown)
            fixed = true;
    }

    Image {
        id: displayArea

        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        source: G.getAssetUrl("image/system/roles/unknown.png")

        MouseArea {
            anchors.fill: parent
            enabled: !fixed

            onClicked: {
                if (!fixed)
                    expandArea.visible = true;
            }
        }
    }

    Image {
        anchors.centerIn: parent
        fillMode: Image.PreserveAspectFit
        height: parent.height * 1.3
        source: G.getAssetUrl("image/system/role_shown_icon.png")
        visible: roleComboBox.roleShown
        width: parent.width * 1.3
    }

    Column {
        id: expandArea

        anchors.horizontalCenter: displayArea.horizontalCenter
        anchors.top: displayArea.top
        opacity: 0
        spacing: 3
        transformOrigin: Item.TopLeft
        visible: false
        width: roleComboBox.width

        transform: Scale {
            id: expandYScale

            yScale: 0.5
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

        ParallelAnimation {
            id: appearAnimation

            running: false

            onFinished: expandFinish = true

            PropertyAnimation {
                duration: 200
                easing.type: Easing.Linear
                from: 0
                property: "opacity"
                target: expandArea
                to: 1
            }

            PropertyAnimation {
                duration: 200
                easing.type: Easing.Linear
                from: 0.5
                property: "yScale"
                target: expandYScale
                to: 1
            }
        }

        Repeater {
            model: ["unknown", "loyalist", "rebel", "renegade"]

            Image {
                required property string modelData

                anchors.horizontalCenter: parent.horizontalCenter
                fillMode: Image.PreserveAspectFit
                height: roleComboBox.height
                source: {
                    if (modelData == "unknown")
                        return G.getAssetUrl("image/system/roles/unknown.png");

                    return G.getAssetUrl("image/system/roles/" + modelData + "-1.png");
                }
                width: roleComboBox.width

                MouseArea {
                    anchors.fill: parent

                    onClicked: {
                        expandArea.visible = false;
                        roleComboBox.role = modelData;
                    }
                    onPressed: mouse => {
                        if (!expandFinish)
                            mouse.accepted = false;
                    }
                }
            }
        }
    }

    Connections {
        function onSpaceClicked() {
            expandArea.visible = false;
        }

        target: roomScene
    }
}
