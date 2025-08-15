import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: roleComboBox

    width: 50
    height: 50

    property bool roleShown: false
    property bool fixed: false
    property string role: "unknown"

    property bool expandStart: false
    property bool expandFinish: false

    property real childwh: 24.5
    property int column: 2

    transformOrigin: Item.TopRight

    PropertyAnimation on scale {
        id: appearAnimation
        running: false
        from: 1
        to: column
        easing.type: Easing.Linear
        duration: 200

        onFinished: expandFinish = true
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (!expandStart && !fixed) {
                expandStart = true;
                appearAnimation.start();
            }
        }
    }

    Grid {
        id: itemGrid

        anchors.fill: parent
        columns: column
        spacing: 1
        visible: true
    }

    Component {
        id: subItemComponent

        Rectangle {
            property bool selected: true
            property color kingdomColor

            MouseArea {
                anchors.fill: parent
                onPressed: {
                    if (!expandFinish)
                        mouse.accepted = false;
                }
                onClicked: selected = !selected
            }

            function refresh() {
                if (selected)
                    color = kingdomColor;
                else
                    color = "black";
            }

            onSelectedChanged: refresh()
        }
    }

    Image {
        id: fixedImage

        visible: false
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
    }

    Component.onCompleted: {
        var kingdoms = Sanguosha.getHegemonyKingdoms();

        if (kingdoms.length <= 4) {
            childwh = 24.5;
            column = 2;
        } else if (kingdoms.length <= 9) {
            childwh = 16;
            column = 3;
        } else {
            childwh = 11.75;
            column = 4;
        }

        for (var i = 0; i < kingdoms.length; ++i) {
            var created = subItemComponent.createObject(itemGrid);
            created.kingdomColor = Sanguosha.getKingdomColor(kingdoms[i]);
            created.refresh();
            created.width = childwh;
            created.height = childwh;
            created.visible = true;
        }
    }

    onRoleShownChanged: {
        if (roleShown)
            fixed = true;
    }

    onRoleChanged: {
        fixedImage.source = G.getUrl("image/system/roles/" + role + ".png");
    }

    onFixedChanged: {
        if (fixed) {
            appearAnimation.stop();
            expandStart = false;
            expandFinish = false;
            roleComboBox.scale = 1;
            itemGrid.visible = false;
            fixedImage.visible = true;
        }
    }

    Connections {
        target: roomScene
        function onSpaceClicked() {
            appearAnimation.stop();
            expandStart = false;
            expandFinish = false;
            roleComboBox.scale = 1;
        }
    }
}
