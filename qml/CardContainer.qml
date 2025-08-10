import QtQuick 6.5

Item {
    property list<CardItem> cardItems

    property int lastAlign: Qt.AlignLeft
    property bool autoBack: false

    Component {
        id: cardItemComponent

        CardItem {}
    }

    function lay(align: int, useHomePos: bool, goBack: bool) {
        if (cardItems.length === 0)
            return;
        align = align | Qt.AlignHorizontal_Mask;

        var step = Math.min(cardItems[0].width + 5, (width - cardItems[0].width) / (cardItems.length - 1));

        var currentLeftPos = 0;
        if (align === Qt.AlignRight)
            currentLeftPos = width - (step * (cardItems.length - 1));

        var i;

        for (i = 0; i < cardItems.length; ++i) {
            if (useHomePos) {
                cardItems[i].homeX = currentLeftPos;
                cardItems[i].homeY = 0;
            } else {
                cardItems[i].x = currentLeftPos;
                cardItems[i].y = 0;
            }
            cardItems[i].z = i;

            currentLeftPos += step;
        }

        if (useHomePos && goBack) {
            for (i = 0; i < cardItems.length; ++i)
                cardItems[i].goBack();
        }

        lastAlign = align;
    }

    onWidthChanged: {
        if (autoBack)
            lay(lastAlign, true, true);
    }

    function createItem(cardId: int): CardItem {
        var item = cardItemComponent.createObject(this, {
            cardId: cardId,
            visible: true,
            opacity: 0
        });
        cardItems.push(item);

        return item;
    }

    function insertItem(item: CardItem) {
        cardItems.push(item);
    }

    function takeItem(item: CardItem) {
        var index = cardItems.indexOf(item);
        if (index !== -1)
            cardItems.splice(index, 1);
    }

    function takeItemAt(index: int): CardItem {
        if (index >= 0 && index < cardItems.length) {
            var item = cardItems[index];
            cardItems.splice(index, 1);
            return item;
        }

        return null;
    }
}
