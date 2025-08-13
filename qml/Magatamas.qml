import QtQuick 6.5

Item {
    id: magatamas

    property int hp: 5
    property int maxhp: 5
    property int dyingThreshold: 0

    // one of following two display method is selected based on maxhp

    Column {
        id: magatamasLe5

        anchors.centerIn: parent
        visible: false

        Magatama {
            id: magatama1

            anchors.horizontalCenter: parent.horizontalCenter
        }
        Magatama {
            id: magatama2

            anchors.horizontalCenter: parent.horizontalCenter
        }
        Magatama {
            id: magatama3

            anchors.horizontalCenter: parent.horizontalCenter
        }
        Magatama {
            id: magatama4

            anchors.horizontalCenter: parent.horizontalCenter
        }
        Magatama {
            id: magatama5

            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Column {
        id: magatamasGe6

        anchors.centerIn: parent
        visible: false

        Magatama {
            id: magatama6

            visible: true
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            id: hpText6

            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            id: hpSlash6

            text: "/"
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            id: maxHpText6

            anchors.horizontalCenter: parent.horizontalCenter
        }
        Text {
            id: dyingThresholdText6

            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    Component {
        id: magatamaComponent

        Magatama {
            id: createdMagatama

            transformOrigin: Item.Center
            visible: true
            ParallelAnimation {
                running: true

                PropertyAnimation {
                    target: createdMagatama
                    property: "scale"
                    from: 1
                    to: 4
                    duration: 500
                }

                PropertyAnimation {
                    target: createdMagatama
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 500
                }

                onFinished: {
                    createdMagatama.destroy();
                }
            }
        }
    }

    function playLostHpAnimation(from: int, to: int) {
        if (to >= from)
            return;

        var displayHp = from;
        if (from === maxhp)
            displayHp = 5;
        else if (from >= 4)
            displayHp = 4;
        else if (from < 1)
            displayHp = 1;

        if (maxhp >= 6) {
            var m6 = magatamaComponent.createObject(magatamas);
            m6.x = magatama6.x + magatamasGe6.x;
            m6.y = magatama6.y + magatamasGe6.y;
            m6.n = displayHp;
        } else {
            var vfrom = from;
            if (vfrom < 1)
                vfrom = 1;
            else if (vfrom >= maxhp)
                vfrom = maxhp;
            var vto = to;
            if (vto < 0)
                vto = 0;
            else if (vto >= maxhp)
                vto = maxhp;

            for (var i = vfrom; i > vto; --i) {
                var mi = magatama5;

                if (i === 1)
                    mi = magatama1;
                else if (i === 2)
                    mi = magatama2;
                else if (i === 3)
                    mi = magatama3;
                else if (i === 4)
                    mi = magatama4;

                var m5 = magatamaComponent.createObject(magatamas);
                m5.x = mi.x + magatamasLe5.x;
                m5.y = mi.y + magatamasLe5.y;
                m5.n = displayHp;
            }
        }
    }

    readonly property list<color> displayHpColorList: ["#E90000", "#E92222", "#E97422", "#C3C322", "#8DC322", "#42AE22"]

    function refresh() {
        var displayHp = hp;
        if (hp === maxhp)
            displayHp = 5;
        else if (hp >= 4)
            displayHp = 4;
        else if (hp < 0)
            displayHp = 0;

        if (maxhp <= 5) {
            magatamasGe6.visible = false;
            magatamasLe5.visible = true;

            magatama1.visible = maxhp >= 1;
            magatama2.visible = maxhp >= 2;
            magatama3.visible = maxhp >= 3;
            magatama4.visible = maxhp >= 4;
            magatama5.visible = maxhp === 5;

            magatama1.n = displayHp;
            magatama2.n = displayHp;
            magatama3.n = displayHp;
            magatama4.n = displayHp;
            magatama5.n = displayHp;

            magatama1.lost = hp < 1;
            magatama2.lost = hp < 2;
            magatama3.lost = hp < 3;
            magatama4.lost = hp < 4;
            magatama5.lost = hp < 5;

            magatama1.dyingThreshold = dyingThreshold >= 1;
            magatama2.dyingThreshold = dyingThreshold >= 2;
            magatama3.dyingThreshold = dyingThreshold >= 3;
            magatama4.dyingThreshold = dyingThreshold >= 4;
            magatama5.dyingThreshold = dyingThreshold >= 5;
        } else {
            magatamasGe6.visible = true;
            magatamasLe5.visible = false;

            magatama6.n = displayHp;
            magatama6.lost = false;
            magatama6.dyingThreshold = false;

            hpText6.text = hp.toString();
            maxHpText6.text = maxhp.toString();
            if (dyingThreshold > 0)
                dyingThresholdText6.text = "(" + (dyingThreshold + 1).toString() + ")";
            else
                dyingThresholdText6.visible = false;

            hpText6.color = displayHpColorList[displayHp];
            hpSlash6.color = displayHpColorList[displayHp];
            maxHpText6.color = displayHpColorList[displayHp];
            dyingThresholdText6.color = displayHpColorList[displayHp];
        }
    }

    onHpChanged: refresh()
    onMaxhpChanged: refresh()
    onDyingThresholdChanged: refresh()

    Component.onCompleted: refresh()
}
