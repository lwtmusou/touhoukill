import QtQuick 6.5
import rocks.touhousatsu 1.0

Item {
    id: cardItem

    height: 256
    width: 183
    opacity: 0

    // property set by CardContainer for goBack
    property real homeX: 0
    property real homeY: 0
    property real homeOpacity: 1

    // property set by CardContainer to enable Drag
    property bool enableDrag: false

    // property for displaying
    property int cardId: -1
    property string general
    property string footnoteText

    // property for recording its status
    // property bool selected

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
        id: cardItemMouseArea

        anchors.fill: parent

        // set by onEnableDragChanged to enable draging programatically
        // drag.target: cardItem
        drag.axis: Drag.XAndYAxis
        drag.threshold: 30
        drag.smoothed: false
    }

    onEnableDragChanged: {
        if (enableDrag)
            cardItemMouseArea.drag.target = cardItem;
        else
            cardItemMouseArea.drag.target = undefined;
    }

    ParallelAnimation {
        id: goBackAnimation
        running: false

        PropertyAnimation {
            id: goBackXAmimation

            target: cardItem
            property: "x"
            to: cardItem.homeX
            easing.type: Easing.OutQuad
            duration: 250
        }

        PropertyAnimation {
            id: goBackYAnimation

            target: cardItem
            property: "y"
            to: cardItem.homeY
            easing.type: Easing.OutQuad
            duration: 250
        }

        PropertyAnimation {
            id: goBackOpacityAnimation

            target: cardItem
            property: "opacity"
            to: cardItem.homeOpacity
            easing.type: Easing.InSine
            duration: 250
        }
    }

    function goBack() {
        goBackAnimation.stop();

        goBackXAmimation.from = cardItem.x;
        goBackYAnimation.from = cardItem.y;
        goBackOpacityAnimation.from = cardItem.opacity;

        goBackAnimation.start();
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
