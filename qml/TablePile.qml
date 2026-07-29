import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: tablePile

    // Clearance timestamp per cardId. Number.MAX_VALUE = not marked (just added).
    // A card is faded out once currentTime - timestamp > sClearanceDelayBuckets.
    property var clearTimestamps: ({})
    property int currentTime: 0

    // Max visible cards before oldest are marked for delayed clearance.
    // Mirrors old TablePile::setSize (src/uibackup/TablePile.cpp:34).
    readonly property int numCardsVisible: Math.floor(width / 183) + 1

    // QObject parent for created CardItems -- roomScene (see plan "CardItem and card
    // container design"). visual parent is the inner CardContainer.
    property var roomScene: null
    readonly property int sClearanceDelayBuckets: 3
    readonly property int sClearanceUpdateIntervalMsec: 1000

    // Periodic clearance check. Mirrors old TablePile::timerEvent (TablePile.cpp:39):
    // destroy cards whose clearance timestamp expired, then relayout.
    // Uses pile.removeItem (same-object list-property splice) -- do NOT splice a
    // cross-object copy (var items = pile.cardItems; items.splice(...)) as that acts
    // on a copy and leaves destroyed cards as ghosts in the real list, which breaks
    // later lay()/goBack() and leaves new cards stuck at opacity 0.
    function _checkClearance() {
        currentTime += 1;
        var items = pile.cardItems;
        var toDestroy = [];
        for (var i = 0; i < items.length; ++i) {
            var cid = items[i].cardId;
            var ts = clearTimestamps[cid];
            if (ts !== undefined && ts !== Number.MAX_VALUE && currentTime - ts > sClearanceDelayBuckets)
                toDestroy.push(cid);
        }
        if (toDestroy.length === 0)
            return;
        for (var j = 0; j < toDestroy.length; ++j) {
            pile.removeItem(toDestroy[j]);
            delete clearTimestamps[toDestroy[j]];
        }
        pile.lay(Qt.AlignHCenter, 1, 0, true, true);
    }

    // Mark oldest non-marked cards for clearance when count exceeds capacity.
    // Mirrors old TablePile::_addCardItems overflow marking (TablePile.cpp:149-154).
    function _markOverflowClearance() {
        var items = pile.cardItems;
        var numAdded = 1;
        var numRemoved = items.length - Math.max(numCardsVisible, numAdded + 1);
        for (var i = 0; i < numRemoved; ++i) {
            var cid = items[i].cardId;
            var ts = clearTimestamps[cid];
            if (ts === undefined || ts === Number.MAX_VALUE)
                clearTimestamps[cid] = currentTime;
        }
    }

    // Add a card to the pile. toPlace is one of PlaceTable/PlaceJudge/DiscardPile,
    // not distinguished (see plan "TablePile"). Mirrors old TablePile::_addCardItems
    // (src/uibackup/TablePile.cpp:127): append, mark overflow, relayout.
    function addCard(cardId: int) {
        pile.createItem(cardId);
        clearTimestamps[cardId] = Number.MAX_VALUE;
        _markOverflowClearance();
        pile.lay(Qt.AlignHCenter, 1, 0, true, true);
    }

    // Remove a card immediately (move_cards_lost: card taken away from the pile).
    function removeCard(cardId: int) {
        pile.removeItem(cardId);
        delete clearTimestamps[cardId];
        pile.lay(Qt.AlignHCenter, 1, 0, true, true);
    }

    anchors.centerIn: parent
    height: 256
    width: parent.width * 0.45

    CardContainer {
        id: pile

        anchors.fill: parent
        roomScene: tablePile.roomScene
    }

    Timer {
        interval: tablePile.sClearanceUpdateIntervalMsec
        repeat: true
        running: pile.cardItems.length > 0

        onTriggered: tablePile._checkClearance()
    }
}
