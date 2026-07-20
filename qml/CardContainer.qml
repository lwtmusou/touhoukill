import QtQuick 6.5

Item {
    id: cardContainer

    property bool autoBack: false
    property list<CardItem> cardItems
    property int lastAlign: Qt.AlignLeft
    property real lastRowOffest: 0
    property int lastRownum: 0
    // QObject parent for created CardItems -- roomScene (see plan "CardItem and card container design").
    // visual parent is cardContainer; QObject parent stays roomScene so cards can move
    // between containers without reparent issues.
    property var rootScene: null

    function createItem(cardId: int): CardItem {
        var item = cardItemComponent.createObject(rootScene, {
                                                      cardId: cardId,
                                                      visible: true,
                                                      opacity: 0
                                                  });
        item.parent = cardContainer;
        cardItems.push(item);

        return item;
    }

    function insertItem(item: CardItem) {
        cardItems.push(item);
    }

    function lay(align: int, rownum: int, rowOffset: real, useHomePos: bool, goBack: bool) {
        if (cardItems.length === 0)
            return;
        if (rownum === 0)
            rownum = 1;
        if (rowOffset < 0.0001)
            rowOffset = (height / (rownum - 1));

        align = align & Qt.AlignHorizontal_Mask;

        var step = Math.min(cardItems[0].width + 5, (width - cardItems[0].width) / (cardItems.length - 1));

        var nperrow = Math.ceil((cardItems.length + 1) / rownum);
        var y = 0;

        var x = 0;
        if (align === Qt.AlignRight) {
            x = width - (step * (nperrow - 1));
            step = -step;
        }

        var i;

        for (i = 0; i < cardItems.length; ++i) {
            if ((i !== 0) && (i % nperrow === 0)) {
                if (align === Qt.AlignRight)
                    x = width - (step * (nperrow - 1));
                else
                    x = 0;

                y += rowOffset;
            }

            if (useHomePos) {
                cardItems[i].homeX = x;
                cardItems[i].homeY = y;
            } else {
                cardItems[i].x = x;
                cardItems[i].y = y;
            }
            cardItems[i].z = i;

            x += step;
        }

        if (useHomePos && goBack) {
            for (i = 0; i < cardItems.length; ++i)
                cardItems[i].goBack();
        }

        lastAlign = align;
        lastRownum = rownum;
        lastRowOffest = rowOffset;
    }

    // Remove and destroy the CardItem with the given cardId. self hand cards have unique
    // cardIds; for other players' hidden cards (cardId == -1) this is not used (they don't
    // enter Dashboard.cardArea, only handcardNum is synced).
    function removeItem(cardId: int): CardItem {
        for (var i = 0; i < cardItems.length; ++i) {
            if (cardItems[i].cardId === cardId) {
                var item = cardItems[i];
                cardItems.splice(i, 1);
                item.destroy();
                return item;
            }
        }
        return null;
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

    onWidthChanged: {
        if (autoBack)
            lay(lastAlign, lastRownum, lastRowOffest, true, true);
    }

    Component {
        id: cardItemComponent

        CardItem {
        }
    }
}
