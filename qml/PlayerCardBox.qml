import QtQuick 6.5

import rocks.touhousatsu 1.0

// askForCardChosen (S_COMMAND_CHOOSE_CARD) popup: shows a target player's handcard /
// equip / judging cards and lets the local player pick one. Built on GraphicsBox
// (draggable image background). Card data is fetched directly from the target
// player object via Player/ClientPlayer Q_INVOKABLE accessors (handcardIds /
// shownHandcardIds / equipIds / judgingIds) -- no assembly in the RoomScene bridge.
// When the target's hand is only partially public, hidden handcards collapse to a
// single card back (cardId = -1 = S_UNKNOWN_CARD_ID); clicking it lets the server
// pick a random hidden card. Clicking any card calls
// ClientInstance.onPlayerChooseCard(id) and destroys the box. There is no cancel --
// the player must choose (server picks randomly on timeout if no reply).
GraphicsBox {
    id: playerCardBox

    // Request params (passed from RoomScene.qml onNotifyCardsGot).
    property var player: null
    property string reason: ""
    property string flags: ""
    property bool handcardVisible: false
    property int method: 0
    property var disabledIds: []
    property bool enableEmptyCard: false

    signal cardChosen(int cardId)

    function _choose(cardId) {
        cardChosen(cardId);
        if (parent && parent.activeBox === playerCardBox)
            parent.activeBox = null;
        playerCardBox.destroy();
    }

    // A card is disabled if in disabledIds, or (discard method && Self cannot
    // discard the target's card). The hidden back uses cardId -1 (Card::S_UNKNOWN_CARD_ID).
    function _isDisabled(cardId) {
        if (disabledIds.indexOf(cardId) >= 0)
            return true;
        if (method === Card.MethodDiscard && cardId !== -1)
            return !roomScene.Self.canDiscard(player, cardId);
        return false;
    }

    function _footnote(cardId, isShown, isJudging) {
        if (isShown)
            return Sanguosha.translate("shown_card");
        if (isJudging) {
            var c = Sanguosha.getEngineCard(cardId);
            return c ? Sanguosha.translate(c.objectName) : "";
        }
        return "";
    }

    // Build the per-area card descriptors: [ {cardId, disabled, footnote} ].
    function _handcards() {
        var cards = [];
        if (handcardVisible || player === roomScene.Self) {
            var shown = player.getShownHandcards();
            var ids = player.handcardIds();
            for (var i = 0; i < ids.length; ++i) {
                var id = ids[i];
                cards.push({
                    cardId: id,
                    disabled: _isDisabled(id),
                    footnote: _footnote(id, shown.indexOf(id) >= 0, false)
                });
            }
        } else {
            var shownIds = player.getShownHandcards();
            for (var j = 0; j < shownIds.length; ++j) {
                var sid = shownIds[j];
                cards.push({
                    cardId: sid,
                    disabled: _isDisabled(sid),
                    footnote: _footnote(sid, true, false)
                });
            }
            // Collapse all hidden handcards into a single card back (cardId = -1).
            var hidden = player.handcard - shownIds.length;
            if (hidden > 0)
                cards.push({
                    cardId: -1,
                    disabled: !enableEmptyCard,
                    footnote: ""
                });
        }
        return cards;
    }

    function _equips() {
        var cards = [];
        var ids = player.equipIds();
        for (var i = 0; i < ids.length; ++i) {
            var id = ids[i];
            cards.push({
                cardId: id,
                disabled: _isDisabled(id),
                footnote: ""
            });
        }
        return cards;
    }

    function _judgings() {
        var cards = [];
        var ids = player.getJudgingAreaID();
        for (var i = 0; i < ids.length; ++i) {
            var id = ids[i];
            cards.push({
                cardId: id,
                disabled: _isDisabled(id),
                footnote: _footnote(id, false, true)
            });
        }
        return cards;
    }

    function _areas() {
        var a = [];
        if ((flags.indexOf("h") >= 0 || flags.indexOf("s") >= 0) && !player.kongcheng)
            a.push({
                label: qsTr("Handcard area"),
                cards: _handcards()
            });
        if (flags.indexOf("e") >= 0 && player.equipIds().length > 0)
            a.push({
                label: qsTr("Equip area"),
                cards: _equips()
            });
        if (flags.indexOf("j") >= 0 && player.getJudgingAreaID().length > 0)
            a.push({
                label: qsTr("Judging area"),
                cards: _judgings()
            });
        return a;
    }

    property var areas: _areas()

    height: Math.max(360, contentColumn.implicitHeight + 60)
    source: G.getAssetUrl("image/system/card-container.png")
    width: Math.max(720, contentColumn.implicitWidth + 60)

    // Title
    Text {
        id: titleText

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 12
        color: "#E4D5A0"
        font.pixelSize: 22
        horizontalAlignment: Text.AlignHCenter
        style: Text.Sunken
        styleColor: "#000000"
        text: qsTr("%1: please choose %2's card").arg(Sanguosha.translate(reason)).arg(player ? roomScene.ClientInstance.getPlayerName(player.objectName) : "")
        width: parent.width - 40
        wrapMode: Text.Wrap
    }

    // Areas (handcard / equip / judging), each a label + a Flow of CardItems.
    Column {
        id: contentColumn

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: titleText.bottom
        anchors.topMargin: 12
        spacing: 14
        width: parent.width - 40

        Repeater {
            model: playerCardBox.areas

            Column {
                id: areaColumn

                property var area: modelData
                spacing: 4
                width: contentColumn.width

                Text {
                    color: "#E4D5A0"
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    style: Text.Sunken
                    styleColor: "#000000"
                    text: areaColumn.area.label ?? ""
                    width: areaColumn.width
                }

                Flow {
                    id: areaFlow

                    property var cards: areaColumn.area.cards

                    spacing: 4
                    width: areaColumn.width

                    Repeater {
                        model: areaFlow.cards

                        CardItem {
                            id: chosenCard

                            property var desc: modelData

                            // CardItem renders cardId=-1 as a card back automatically.
                            cardId: desc.cardId ?? -1
                            enabled: !(desc.disabled ?? false)
                            footnoteText: desc.footnote ?? ""
                            opacity: 1
                            scale: 0.6  // fit more cards in the box
                            transformOrigin: Item.TopLeft

                            onClicked: playerCardBox._choose(chosenCard.cardId)
                        }
                    }
                }
            }
        }
    }
}
