import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    property int num: 0
    property string kingdom

    width: 60
    height: 36
    source: G.getUrl("image/fullskin/system/handcard/touhougod.png")

    Text {
        id: numText
        anchors.centerIn: parent
        height: parent.height / 1.5
        width: parent.width
        fontSizeMode: Text.Fit
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 15
        color: "black"
        text: "0"
    }

    onNumChanged: numText.text = num.toString()
    onKingdomChanged: {
        source = G.getUrl("image/fullskin/system/handcard/" + kingdom + ".png");
        var color = Sanguosha.getKingdomColor(kingdom);
        if ((color.r * 0.3 + color.g * 0.59 + color.b * 0.11) < 0.5)
            numText.color = "white";
        else
            numText.color = "black";
    }
}
