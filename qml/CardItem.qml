import QtQuick 6.5
import rocks.touhousatsu 1.0

Image {
    property real homeX
    property real homeY
    property real homeOpacity

    property var currentAnimation

    property int id: -1
    property string general

    property bool selected

    onIdChanged: {
        if (id == -1)
            return;
        general = "";
    }

    onGeneralChanged: {
        if (general == "")
            return;
        id = -1;
    }
}
