import QtQuick 6.5

Item {
    id: magatamas

    property bool banling: false
    readonly property list<color> displayHpColorList: ["#E90000", "#E92222", "#E97422", "#C3C322", "#8DC322", "#42AE22"]
    property int dyingThreshold: 0
    property int hp: 5 // renhp when banling is true
    property int linghp: 5
    property int maxhp: 5

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

        if (banling) {
            var mb = magatamaComponent.createObject(magatamas);
            mb.x = magatamaBanling.x + magatamasBanling.x;
            mb.y = magatamaBanling.y + magatamasBanling.y;
            mb.n = displayHp;
        } else if (maxhp >= 6) {
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

    function refresh() {
        var displayHp = hp;
        if (hp === maxhp)
            displayHp = 5;
        else if (hp >= 4)
            displayHp = 4;
        else if (hp < 0)
            displayHp = 0;

        if (banling) {
            var displayLinghp = linghp;
            if (linghp === maxhp)
                displayLinghp = 5;
            else if (linghp >= 4)
                displayLinghp = 4;
            else if (linghp < 0)
                displayLinghp = 0;

            var displayTotalHp = Math.min(displayHp, displayLinghp);

            magatamasBanling.visible = true;
            magatamasLe5.visible = false;
            magatamasGe6.visible = false;

            magatamaBanling.n = displayTotalHp;
            magatamaBanling.lost = false;
            magatamaBanling.dyingThreshold = false;

            renTextBanling.text = hp.toString();
            lingTextBanling.text = linghp.toString();
            maxHpTextBanling.text = maxhp.toString();
            if (dyingThreshold > 0) {
                dyingThresholdTextBanling.text = "(" + (dyingThreshold + 1).toString() + ")";
                dyingThresholdTextBanling.visible = true;
            } else {
                dyingThresholdTextBanling.visible = false;
            }

            renTextBanling.color = displayHpColorList[displayHp];
            rlCommaBanling.color = displayHpColorList[displayTotalHp];
            lingTextBanling.color = displayHpColorList[displayLinghp];
            hpSlashBanling.color = displayHpColorList[displayTotalHp];
            maxHpTextBanling.color = displayHpColorList[displayTotalHp];
            dyingThresholdTextBanling.color = displayHpColorList[displayTotalHp];
        } else if (maxhp <= 5) {
            magatamasBanling.visible = false;
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
            magatamasBanling.visible = false;
            magatamasGe6.visible = true;
            magatamasLe5.visible = false;

            magatama6.n = displayHp;
            magatama6.lost = false;
            magatama6.dyingThreshold = false;

            hpText6.text = hp.toString();
            maxHpText6.text = maxhp.toString();
            if (dyingThreshold > 0) {
                dyingThresholdText6.text = "(" + (dyingThreshold + 1).toString() + ")";
                dyingThresholdText6.visible = true;
            } else {
                dyingThresholdText6.visible = false;
            }

            hpText6.color = displayHpColorList[displayHp];
            hpSlash6.color = displayHpColorList[displayHp];
            maxHpText6.color = displayHpColorList[displayHp];
            dyingThresholdText6.color = displayHpColorList[displayHp];
        }
    }

    Component.onCompleted: refresh()
    onDyingThresholdChanged: refresh()
    onHpChanged: refresh()
    onMaxhpChanged: refresh()
    onLinghpChanged: refresh()
    onBanlingChanged: refresh()

    // one of following display method is selected based on maxhp / banling
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

            anchors.horizontalCenter: parent.horizontalCenter
            visible: true
        }

        Text {
            id: hpText6

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatama6.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Text {
            id: hpSlash6

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatama6.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            text: "/"
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Text {
            id: maxHpText6

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatama6.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Text {
            id: dyingThresholdText6

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatama6.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }
    }

    Column {
        id: magatamasBanling

        anchors.centerIn: parent
        visible: false

        Magatama {
            id: magatamaBanling

            anchors.horizontalCenter: parent.horizontalCenter
            visible: true
        }

        Text {
            id: renTextBanling

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatamaBanling.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Text {
            id: rlCommaBanling

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatamaBanling.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            text: ","
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Text {
            id: lingTextBanling

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatamaBanling.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Text {
            id: hpSlashBanling

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatamaBanling.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            text: "/"
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Text {
            id: maxHpTextBanling

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatamaBanling.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
        }

        Text {
            id: dyingThresholdTextBanling

            anchors.horizontalCenter: parent.horizontalCenter
            font.pixelSize: 72
            fontSizeMode: Text.Fit
            height: magatamaBanling.height
            horizontalAlignment: Text.AlignHCenter
            minimumPixelSize: 1
            textFormat: Text.PlainText
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.NoWrap
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

                onFinished: {
                    createdMagatama.destroy();
                }

                PropertyAnimation {
                    duration: 500
                    from: 1
                    property: "scale"
                    target: createdMagatama
                    to: 4
                }

                PropertyAnimation {
                    duration: 500
                    from: 1
                    property: "opacity"
                    target: createdMagatama
                    to: 0
                }
            }
        }
    }
}
