import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    id: magatama

    width: 53
    height: 44

    fillMode: Image.PreserveAspectFit

    property int n: 5
    property bool lost: false
    property bool dyingThreshold: false

    function refresh() {
        if (lost) {
            if (dyingThreshold)
                source = G.getUrl("image/system/magatamas/0d.png");
            else
                source = G.getUrl("image/system/magatamas/0.png");
        } else {
            if (dyingThreshold)
                source = G.getUrl("image/system/magatamas/" + n.toString() + "d.png");
            else
                source = G.getUrl("image/system/magatamas/" + n.toString() + ".png");
        }
    }

    onNChanged: refresh()
    onLostChanged: refresh()
    onDyingThresholdChanged: refresh()

    Component.onCompleted: refresh()
}
