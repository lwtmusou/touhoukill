import QtQuick 6.5
import rocks.touhousatsu 1.0

Item {
    id: cardItem

    property int cardId: -1
    property string footnoteText
    property string general
    property real homeOpacity: 1
    property real homeX: 0
    property real homeY: 0

    // Selection state. Dashboard hand cards toggle via clicked; ChooseGeneralBox
    // binds this externally per the selected-generals list. Lift-up visual (Translate
    // transform) is driven by this property.
    property bool selected: false
    // ChooseGeneralBox uses a border instead of lift-up (GridView clip hides the offset).
    // Dashboard hand cards do not set this; they rely on Translate lift-up alone.
    property bool useSelectionBorder: false

    signal clicked
    signal rightClicked

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

        cardImage.source = G.getAssetUrl("image/system/card-back.png");
        cardSuitImage.visible = false;
        cardNumberImage.visible = false;
    }

    function toggleSelected() {
        selected = !selected;
    }

    height: 256
    opacity: 0
    width: 183

    // Lift-up on selection: Translate.y offset (does NOT modify cardItem.y / homeY,
    // so it does not interfere with lay() / goBack()). Hand cards use this;
    // ChooseGeneralBox uses border highlight instead (selected visual per context).
    transform: Translate {
        y: cardItem.selected ? -25 : 0

        Behavior on y {
            NumberAnimation {
                duration: 150
                easing.type: Easing.OutQuad
            }
        }
    }

    onCardIdChanged: {
        if (cardId == -1) {
            // Unknown card (other players' hidden hand cards during move, cardId=-1 by design
            // for client data asymmetry). Show card back, no suit/number.
            general = "";
            cardImage.source = G.getAssetUrl("image/system/card-back.png");
            cardSuitImage.visible = false;
            cardNumberImage.visible = false;
            return;
        }
        general = "";

        var card = Sanguosha.getEngineCard(cardId);
        cardImage.source = G.getAssetUrl("image/card/" + card.objectName + ".png");
        cardSuitImage.source = G.getAssetUrl("image/system/cardsuit/" + card.suit + ".png");

        if (card.red)
            cardNumberImage.source = G.getAssetUrl("image/system/red/" + card.number.toString() + ".png");
        else
            cardNumberImage.source = G.getAssetUrl("image/system/black/" + card.number.toString() + ".png");

        cardSuitImage.visible = true;
        cardNumberImage.visible = true;
    }
    onGeneralChanged: {
        if (general == "")
            return;
        cardId = -1;

        cardImage.source = G.getAssetUrl("image/generals/card/" + general + ".jpg");
        cardSuitImage.visible = false;
        cardNumberImage.visible = false;
    }

    Item {
        id: cardContent

        anchors.centerIn: parent
        height: parent.height
        scale: 1
        transformOrigin: Item.Center
        width: parent.width

        Image {
            id: cardImage

            anchors.fill: parent
            clip: true
            fillMode: Image.PreserveAspectCrop
        }

        Image {
            id: cardSuitImage

            anchors.left: parent.left
            anchors.leftMargin: 2
            anchors.top: parent.top
            anchors.topMargin: 7
            fillMode: Image.PreserveAspectFit
            height: 34
            width: 42
        }

        Image {
            id: cardNumberImage

            anchors.left: parent.left
            anchors.leftMargin: -2
            anchors.top: cardSuitImage.bottom
            anchors.topMargin: -13
            fillMode: Image.PreserveAspectFit
            height: 56
            width: 54
        }

        Text {
            id: cardFootNoteText

            anchors.bottom: parent.bottom
            anchors.bottomMargin: parent.height / 6
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#555500"
            font.pixelSize: 22
            style: Text.Sunken
            styleColor: "#AAAAFF"
            text: cardItem.footnoteText
            width: parent.width * 3 / 4
            wrapMode: Text.Wrap
        }
    }

    MouseArea {
        id: cardItemMouseArea

        acceptedButtons: Qt.LeftButton | Qt.RightButton
        anchors.fill: parent

        onClicked: mouse => {
            if (mouse.button === Qt.RightButton)
                parent.rightClicked();
            else
                parent.clicked();
        }
    }

    // Dim overlay shown when the card is disabled (enabled == false). Uses an overlay
    // rectangle instead of opacity so the GraphicsBox background does not show through.
    // Tied to the standard `enabled` property so all CardItem use cases (choose-general
    // candidates, hand cards, equips, etc.) share one disable mechanism. Relies on
    // declaration order (after cardContent/MouseArea) to render on top -- no z property.
    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.5)
        visible: !cardItem.enabled
    }

    // Selection border (ChooseGeneralBox context, where Translate lift-up is clipped
    // by GridView clip:true). Gold 2px border when selected.
    Rectangle {
        anchors.fill: parent
        border.color: "#FFD700"
        border.width: 2
        color: "transparent"
        visible: cardItem.selected && cardItem.useSelectionBorder
    }

    ParallelAnimation {
        id: goBackAnimation

        running: false

        PropertyAnimation {
            id: goBackXAmimation

            duration: 250
            easing.type: Easing.OutQuad
            property: "x"
            target: cardItem
            to: cardItem.homeX
        }

        PropertyAnimation {
            id: goBackYAnimation

            duration: 250
            easing.type: Easing.OutQuad
            property: "y"
            target: cardItem
            to: cardItem.homeY
        }

        PropertyAnimation {
            id: goBackOpacityAnimation

            duration: 250
            easing.type: Easing.InSine
            property: "opacity"
            target: cardItem
            to: cardItem.homeOpacity
        }
    }
}
