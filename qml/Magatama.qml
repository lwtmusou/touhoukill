import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    id: magatama

    property bool dyingThreshold: false
    property bool lost: false
    property int n: 5

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

    fillMode: Image.PreserveAspectFit
    height: 44
    width: 53

    Component.onCompleted: refresh()
    onDyingThresholdChanged: refresh()
    onLostChanged: refresh()
    onNChanged: refresh()
}
