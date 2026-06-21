import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: roleComboBox

    property real childwh: 24.5
    property int column: 2
    property bool expandFinish: false
    property bool expandStart: false
    property bool fixed: false
    property string role: "unknown"
    property bool roleShown: false

    height: 50
    transformOrigin: Item.TopRight
    width: 50

    PropertyAnimation on scale {
        id: appearAnimation

        duration: 200
        easing.type: Easing.Linear
        from: 1
        running: false
        to: column

        onFinished: expandFinish = true
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
    onRoleChanged: {
        fixedImage.source = G.getUrl("image/system/roles/" + role + ".png");
    }
    onRoleShownChanged: {
        if (roleShown)
            fixed = true;
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
            property color kingdomColor
            property bool selected: true

            function refresh() {
                if (selected)
                    color = kingdomColor;
                else
                    color = "black";
            }

            onSelectedChanged: refresh()

            MouseArea {
                anchors.fill: parent

                onClicked: selected = !selected
                onPressed: {
                    if (!expandFinish)
                        mouse.accepted = false;
                }
            }
        }
    }
    Image {
        id: fixedImage

        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        visible: false
    }
    Connections {
        function onSpaceClicked() {
            appearAnimation.stop();
            expandStart = false;
            expandFinish = false;
            roleComboBox.scale = 1;
        }

        target: roomScene
    }
}
