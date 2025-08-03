import QtQuick 6.5
import rocks.touhousatsu 1.0

Item {
    id: cardItem

    height: 256
    width: 183

    property real homeX
    property real homeY
    property real homeOpacity

    property int cardId: -1
    property string general

    property bool selected

    property string footnoteText

    Item {
        id: cardContent
        anchors.centerIn: parent
        width: parent.width
        height: parent.height

        transformOrigin: Item.Center
        scale: 1

        Image {
            id: cardImage

            anchors.fill: parent

            clip: true
            fillMode: Image.PreserveAspectCrop
        }

        Image {
            id: cardSuitImage

            anchors.top: parent.top
            anchors.left: parent.left

            anchors.topMargin: 7
            anchors.leftMargin: 2

            height: 34
            width: 42

            fillMode: Image.PreserveAspectFit
        }

        Image {
            id: cardNumberImage

            anchors.top: cardSuitImage.bottom
            anchors.left: parent.left
            anchors.topMargin: -13
            anchors.leftMargin: -2

            height: 56
            width: 54

            fillMode: Image.PreserveAspectFit
        }

        Text {
            id: cardFootNoteText

            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height / 6

            width: parent.width * 3 / 4

            wrapMode: Text.Wrap

            text: cardItem.footnoteText
            font.pixelSize: 22
            color: "#555500"

            style: Text.Sunken
            styleColor: "#AAAAFF"
        }
    }

    MouseArea {
        anchors.fill: parent
    }

    function goBack() {
    }

    function setUnknownCard() {
        cardId = -1;
        general = "";

        cardImage.source = G.getUrl("image/system/card-back.png");
        cardSuitImage.visible = false;
        cardNumberImage.visible = false;
    }

    onCardIdChanged: {
        if (cardId == -1)
            return;
        general = "";

        var card = Sanguosha.getEngineCard(cardId);
        cardImage.source = G.getUrl("image/card/" + card.objectName + ".png");
        cardSuitImage.source = G.getUrl("image/system/cardsuit/" + card.suit + ".png");

        if (card.red)
            cardNumberImage.source = G.getUrl("image/system/red/" + card.number.toString() + ".png");
        else
            cardNumberImage.source = G.getUrl("image/system/black/" + card.number.toString() + ".png");

        cardSuitImage.visible = true;
        cardNumberImage.visible = true;
    }

    onGeneralChanged: {
        if (general == "")
            return;
        cardId = -1;

        cardImage.source = G.getUrl("image/generals/card/" + general + ".jpg");
        cardSuitImage.visible = false;
        cardNumberImage.visible = false;
    }
}
