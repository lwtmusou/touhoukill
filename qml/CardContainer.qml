import QtQuick 6.5

import rocks.touhousatsu 1.0

CppCardContainer {
    id: cardContainer

    property bool autoBack: false
    property list<CardItem> cardItems
    property int lastAlign: Qt.AlignLeft
    property real lastRowOffest: 0
    property int lastRownum: 0
    // QObject parent for created CardItems -- roomScene (see plan "CardItem and card container design").
    // visual parent is cardContainer; QObject parent stays roomScene so cards can move
    // between containers without reparent issues.
    property var roomScene: null

    function createItem(cardId: int): CardItem {
        var item = cardItemComponent.createObject(roomScene, {
                                                      cardId: cardId,
                                                      visible: true,
                                                      opacity: 0
                                                  });
        item.parent = cardContainer;
        cardItems.push(item);
        cardContainer.registerCardItem(item);

        return item;
    }

    // Selection: scan all CardItem children and return those with selected == true.
    // Called imperatively (not a binding) because child-property changes inside the
    // list do not trigger QML re-evaluation of container-level property expressions.
    function getSelectedItems() {
        var result = [];
        for (var i = 0; i < cardItems.length; ++i) {
            if (cardItems[i].selected)
                result.push(cardItems[i]);
        }
        return result;
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
        } else if (align === Qt.AlignHCenter) {
            // Single-row centering (TablePile usage). Total row width = step*(count-1) + cardWidth.
            var rowWidth = step * (cardItems.length - 1) + cardItems[0].width;
            x = (width - rowWidth) / 2;
            if (x < 0)
                x = 0;
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
                cardContainer.unregisterCardItem(item);
                item.destroy();
                return item;
            }
        }
        return null;
    }

    function selectOnlyCard(item: CardItem) {
        for (var i = 0; i < cardItems.length; ++i) {
            cardItems[i].selected = (cardItems[i] === item);
        }
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

    function unselectAll(except: CardItem) {
        for (var i = 0; i < cardItems.length; ++i) {
            if (cardItems[i] !== except)
                cardItems[i].selected = false;
        }
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
