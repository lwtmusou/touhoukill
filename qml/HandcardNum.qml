import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    property string kingdom
    property int num: 0

    height: 36
    source: G.getUrl("image/fullskin/system/handcard/touhougod.png")
    width: 60

    onKingdomChanged: {
        source = G.getUrl("image/fullskin/system/handcard/" + kingdom + ".png");
        var color = Sanguosha.getKingdomColor(kingdom);
        if ((color.r * 0.3 + color.g * 0.59 + color.b * 0.11) < 0.5)
            numText.color = "white";
        else
            numText.color = "black";
    }
    onNumChanged: numText.text = num.toString()

    Text {
        id: numText

        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 15
        fontSizeMode: Text.Fit
        height: parent.height / 1.5
        horizontalAlignment: Text.AlignHCenter
        text: "0"
        verticalAlignment: Text.AlignVCenter
        width: parent.width
    }
}
