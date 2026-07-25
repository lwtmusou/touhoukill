import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: judgeArea

    // Delayed-trick judge area: one icon per card (no CardItem), mirroring old
    // PlayerCardContainer::addDelayedTricks (uibackup/GenericCardContainerUI.cpp:941).
    // Icon source = image/icon/<objectName>.png (defaultSkin.image.json: judgeCardIcon-default).
    property var cardIds: []

    function addDelayedTrick(cardId: int) {
        judgeArea.cardIds = judgeArea.cardIds.concat([cardId]);
    }

    function removeDelayedTrick(cardId: int) {
        judgeArea.cardIds = judgeArea.cardIds.filter(c => c !== cardId);
    }

    Row {
        spacing: 2

        Repeater {
            model: judgeArea.cardIds

            Image {
                height: 43
                source: G.getAssetUrl("image/icon/" + Sanguosha.getEngineCard(modelData).objectName + ".png")
                width: 43
            }
        }
    }
}
