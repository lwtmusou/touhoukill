#include "serverplayer.h"
#include "ai.h"
#include "banpair.h"
#include "engine.h"
#include "gamerule.h"
#include "lua-wrapper.h"
#include "recorder.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"

using namespace QSanProtocol;
using namespace JsonUtils;

const int ServerPlayer::S_NUM_SEMAPHORES = 6;

ServerPlayer::ServerPlayer(Room *room)
    : Player(room)
    , m_isClientResponseReady(false)
    , m_isWaitingReply(false)
    , socket(NULL)
    , room(room)
    , ai(NULL)
    , trust_ai(new TrustAI(this))
    , recorder(NULL)
    , _m_phases_index(0)
//, next(NULL)
{
    semas = new QSemaphore *[S_NUM_SEMAPHORES];
    for (int i = 0; i < S_NUM_SEMAPHORES; i++)
        semas[i] = new QSemaphore(0);
}

void ServerPlayer::drawCard(const Card *card)
{
    handcards << card;
}

Room *ServerPlayer::getRoom() const
{
    return room;
}

void ServerPlayer::broadcastSkillInvoke(const QString &card_name) const
{
    room->broadcastSkillInvoke(card_name, isMale(), -1);
}

void ServerPlayer::broadcastSkillInvoke(const Card *card) const
{
    if (card->isMute())
        return;

    QString skill_name = card->getSkillName();
    const Skill *skill = Sanguosha->getSkill(skill_name);
    if (skill == NULL) {
        if (card->getCommonEffectName().isNull())
            broadcastSkillInvoke(card->objectName());
        else
            room->broadcastSkillInvoke(card->getCommonEffectName(), "common");
        return;
    } else {
        int index = skill->getEffectIndex(this, card);
        if (index == 0)
            return;

        if ((index == -1 && skill->getSources().isEmpty()) || index == -2) {
            if (card->getCommonEffectName().isNull())
                broadcastSkillInvoke(card->objectName());
            else
                room->broadcastSkillInvoke(card->getCommonEffectName(), "common");
        } else
            room->broadcastSkillInvoke(skill_name, index);
    }
}

int ServerPlayer::getRandomHandCardId() const
{
    return getRandomHandCard()->getEffectiveId();
}

const Card *ServerPlayer::getRandomHandCard() const
{
    int index = qrand() % handcards.length();
    return handcards.at(index);
}

void ServerPlayer::obtainCard(const Card *card, bool unhide)
{
    CardMoveReason reason(CardMoveReason::S_REASON_GOTCARD, objectName());
    room->obtainCard(this, card, reason, unhide);
}

void ServerPlayer::throwAllEquips()
{
    QList<const Card *> equips = getEquips();

    if (equips.isEmpty())
        return;

    DummyCard *card = new DummyCard;
    foreach (const Card *equip, equips) {
        if (!isJilei(card))
            card->addSubcard(equip);
    }
    if (card->subcardsLength() > 0)
        room->throwCard(card, this);
    card->deleteLater();
}

void ServerPlayer::throwAllHandCards()
{
    int card_length = getHandcardNum();
    room->askForDiscard(this, QString(), card_length, card_length);
}

void ServerPlayer::throwAllHandCardsAndEquips()
{
    int card_length = getCardCount(true);
    room->askForDiscard(this, QString(), card_length, card_length, false, true);
}

void ServerPlayer::throwAllMarks(bool visible_only)
{
    foreach (QString mark_name, marks.keys()) {
        if (!mark_name.startsWith("@"))
            continue;

        int n = marks.value(mark_name, 0);
        if (n != 0)
            room->setPlayerMark(this, mark_name, 0);
    }

    if (!visible_only)
        marks.clear();
}

void ServerPlayer::clearOnePrivatePile(QString pile_name)
{
    if (!piles.contains(pile_name))
        return;
    QList<int> &pile = piles[pile_name];

    DummyCard *dummy = new DummyCard(pile);
    CardMoveReason reason(CardMoveReason::S_REASON_REMOVE_FROM_PILE, objectName());
    bool notifyLog = true;
    QString new_name = pile_name;
    if (new_name.startsWith("#")) {
        foreach (QString flag, getFlagList()) {
            //if (flag.endsWith("_InTempMoving"))
            if (flag == new_name.mid(1) + "_InTempMoving") {
                notifyLog = false;
                break;
            }
        }
    }
    room->throwCard(dummy, reason, NULL, NULL, notifyLog);
    dummy->deleteLater();

    piles.remove(pile_name);
}

void ServerPlayer::clearPrivatePiles()
{
    foreach (QString pile_name, piles.keys())
        clearOnePrivatePile(pile_name);
    piles.clear();
}

void ServerPlayer::bury()
{
    clearFlags();
    clearHistory();
    throwAllCards();
    throwAllMarks();
    clearPrivatePiles();
    room->clearPlayerCardLimitation(this, false);
    room->setPlayerProperty(this, "dyingFactor", 0);
    if (isRemoved())
        room->setPlayerProperty(this, "removed", false);

    this->tag.remove("Huashen_skill");
    this->tag.remove("Huashen_target");
}

void ServerPlayer::throwAllCards()
{
    DummyCard *card = isKongcheng() ? new DummyCard : wholeHandCards();
    foreach (const Card *equip, getEquips())
        card->addSubcard(equip);
    if (card->subcardsLength() != 0)
        room->throwCard(card, this);
    delete card;

    QList<const Card *> tricks = getJudgingArea();
    foreach (const Card *trick, tricks) {
        CardMoveReason reason(CardMoveReason::S_REASON_THROW, objectName());
        room->throwCard(trick, reason, NULL);
    }
}

void ServerPlayer::drawCards(int n, const QString &reason)
{
    room->drawCards(this, n, reason);
}

bool ServerPlayer::askForSkillInvoke(const QString &skill_name, const QVariant &data)
{
    return room->askForSkillInvoke(this, skill_name, data);
}

bool ServerPlayer::askForSkillInvoke(const Skill *skill, const QVariant &data)
{
    return room->askForSkillInvoke(this, skill, data);
}

QList<int> ServerPlayer::forceToDiscard(int discard_num, bool include_equip, bool is_discard)
{
    QList<int> to_discard;

    QString flags = "h";
    if (include_equip)
        flags.append("e");

    QList<const Card *> all_cards = getCards(flags);
    qShuffle(all_cards);

    for (int i = 0; i < all_cards.length(); i++) {
        if (!is_discard || !isJilei(all_cards.at(i)))
            to_discard << all_cards.at(i)->getId();
        if (to_discard.length() == discard_num)
            break;
    }

    return to_discard;
}

int ServerPlayer::aliveCount(bool includeRemoved) const
{
    //return room->alivePlayerCount();
    int n = room->alivePlayerCount();
    if (!includeRemoved) {
        foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->isRemoved())
                n--;
        }
    }
    return n;
}

int ServerPlayer::getHandcardNum() const
{
    return handcards.length();
}

void ServerPlayer::setSocket(ClientSocket *socket)
{
    if (this->socket) {
        disconnect(this->socket);
        this->socket->disconnect(this);
        this->socket->disconnectFromHost();
        this->socket->deleteLater();
    }

    disconnect(this, SLOT(sendMessage(QString)));

    if (socket) {
        connect(socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
        connect(socket, SIGNAL(message_got(const char *)), this, SLOT(getMessage(const char *)));
        connect(this, SIGNAL(message_ready(QString)), this, SLOT(sendMessage(QString)));
    }

    this->socket = socket;
}

void ServerPlayer::getMessage(const char *message)
{
    QString request = message;
    if (request.endsWith("\n"))
        request.chop(1);

    emit request_got(request);
}

void ServerPlayer::unicast(const QString &message)
{
    emit message_ready(message);

    if (recorder)
        recorder->recordLine(message);
}

void ServerPlayer::startNetworkDelayTest()
{
    test_time = QDateTime::currentDateTime();
    Packet packet(S_SRC_ROOM | S_TYPE_NOTIFICATION | S_DEST_CLIENT, S_COMMAND_NETWORK_DELAY_TEST);
    invoke(&packet);
}

qint64 ServerPlayer::endNetworkDelayTest()
{
    return test_time.msecsTo(QDateTime::currentDateTime());
}

void ServerPlayer::startRecord()
{
    recorder = new Recorder(this);
}

void ServerPlayer::saveRecord(const QString &filename)
{
    if (recorder)
        recorder->save(filename);
}

void ServerPlayer::addToSelected(const QString &general)
{
    selected.append(general);
}

QStringList ServerPlayer::getSelected() const
{
    return selected;
}

QString ServerPlayer::findReasonable(const QStringList &generals, bool no_unreasonable)
{
    foreach (QString name, generals) {
        if (Config.Enable2ndGeneral) {
            if (getGeneral()) {
                if (!BanPair::isBanned(getGeneralName()) && BanPair::isBanned(getGeneralName(), name))
                    continue;
            } else {
                if (BanPair::isBanned(name))
                    continue;
            }
        }
        if (Config.GameMode == "zombie_mode") {
            QStringList ban_list = Config.value("Banlist/Zombie").toStringList();
            if (ban_list.contains(name))
                continue;
        }
        if (Config.GameMode.endsWith("p") || Config.GameMode.endsWith("pd") || Config.GameMode.endsWith("pz") || Config.GameMode.contains("_mini_")
            || Config.GameMode == "custom_scenario") {
            QStringList ban_list = Config.value("Banlist/Roles").toStringList();
            if (ban_list.contains(name))
                continue;
        }

        return name;
    }

    if (no_unreasonable)
        return QString();

    return generals.first();
}

void ServerPlayer::clearSelected()
{
    selected.clear();
}

void ServerPlayer::sendMessage(const QString &message)
{
    if (socket) {
#ifndef QT_NO_DEBUG
        printf("%s", qPrintable(objectName()));
#endif
        socket->send(message);
    }
}

void ServerPlayer::invoke(const AbstractPacket *packet)
{
    unicast(packet->toString());
}

void ServerPlayer::invoke(const char *method, const QString &arg)
{
    unicast(QString("%1 %2").arg(method).arg(arg));
}

QString ServerPlayer::reportHeader() const
{
    QString name = objectName();
    return QString("%1 ").arg(name.isEmpty() ? tr("Anonymous") : name);
}

void ServerPlayer::removeCard(const Card *card, Place place)
{
    switch (place) {
    case PlaceHand: {
        handcards.removeOne(card);
        break;
    }
    case PlaceEquip: {
        const EquipCard *equip = qobject_cast<const EquipCard *>(card->getRealCard());
        if (equip == NULL)
            equip = qobject_cast<const EquipCard *>(Sanguosha->getEngineCard(card->getEffectiveId()));
        Q_ASSERT(equip != NULL);
        equip->onUninstall(this);

        WrappedCard *wrapped = Sanguosha->getWrappedCard(card->getEffectiveId());
        removeEquip(wrapped);

        bool show_log = true;
        foreach (QString flag, flags) {
            if (flag.endsWith("_InTempMoving")) {
                show_log = false;
                break;
            }
        }
        if (show_log) {
            LogMessage log;
            log.type = "$Uninstall";
            log.card_str = wrapped->toString();
            log.from = this;
            room->sendLog(log);
        }
        break;
    }
    case PlaceDelayedTrick: {
        removeDelayedTrick(card);
        break;
    }
    case PlaceSpecial: {
        int card_id = card->getEffectiveId();
        QString pile_name = getPileName(card_id);

        //@todo: sanity check required
        if (!pile_name.isEmpty())
            piles[pile_name].removeOne(card_id);

        break;
    }
    default:
        break;
    }
}

void ServerPlayer::addCard(const Card *card, Place place)
{
    switch (place) {
    case PlaceHand: {
        handcards << card;
        break;
    }
    case PlaceEquip: {
        WrappedCard *wrapped = Sanguosha->getWrappedCard(card->getEffectiveId());
        const EquipCard *equip = qobject_cast<const EquipCard *>(wrapped->getRealCard());
        setEquip(wrapped);
        equip->onInstall(this);
        break;
    }
    case PlaceDelayedTrick: {
        addDelayedTrick(card);
        break;
    }
    default:
        break;
    }
}

bool ServerPlayer::isLastHandCard(const Card *card, bool contain) const
{
    if (!card->isVirtualCard()) {
        return handcards.length() == 1 && handcards.first()->getEffectiveId() == card->getEffectiveId();
    } else if (card->getSubcards().length() > 0) {
        if (!contain) {
            foreach (int card_id, card->getSubcards()) {
                if (!handcards.contains(Sanguosha->getCard(card_id)))
                    return false;
            }
            return handcards.length() == card->getSubcards().length();
        } else {
            foreach (const Card *ncard, handcards) {
                if (!card->getSubcards().contains(ncard->getEffectiveId()))
                    return false;
            }
            return true;
        }
    }
    return false;
}

QList<int> ServerPlayer::handCards() const
{
    QList<int> cardIds;
    foreach (const Card *card, handcards)
        cardIds << card->getId();
    return cardIds;
}

QList<const Card *> ServerPlayer::getHandcards() const
{
    return handcards;
}

QList<const Card *> ServerPlayer::getCards(const QString &flags) const
{
    QList<const Card *> cards;
    if (flags.contains("h") && flags.contains("s"))
        cards << handcards;
    else if (flags.contains("h")) {
        foreach (const Card *c, handcards) {
            if (!shown_handcards.contains(c->getEffectiveId()))
                cards << c;
        }
    } else if (flags.contains("s")) {
        foreach (const Card *c, handcards) {
            if (shown_handcards.contains(c->getEffectiveId()))
                cards << c;
        }
    }

    if (flags.contains("e"))
        cards << getEquips();
    if (flags.contains("j"))
        cards << getJudgingArea();

    return cards;
}

DummyCard *ServerPlayer::wholeHandCards() const
{
    if (isKongcheng())
        return NULL;

    DummyCard *dummy_card = new DummyCard;
    foreach (const Card *card, handcards)
        dummy_card->addSubcard(card->getId());

    return dummy_card;
}

bool ServerPlayer::hasNullification() const
{
    foreach (const Card *card, handcards) {
        if (card->objectName() == "nullification")
            return true;
    }

    if (hasTreasure("wooden_ox")) {
        foreach (int id, getPile("wooden_ox")) {
            if (Sanguosha->getCard(id)->objectName() == "nullification")
                return true;
        }
    }

    if (hasSkill("chaoren")) {
        bool ok = false;
        int id = property("chaoren").toInt(&ok);
        if (ok && id > -1 && Sanguosha->getCard(id)->isKindOf("Nullification"))
            return true;
    }

    foreach (const Skill *skill, getVisibleSkillList(true)) {
        if (hasSkill(skill->objectName())) {
            if (skill->inherits("ViewAsSkill")) {
                const ViewAsSkill *vsskill = qobject_cast<const ViewAsSkill *>(skill);
                if (vsskill->isEnabledAtNullification(this))
                    return true;
            } else if (skill->inherits("TriggerSkill")) {
                const TriggerSkill *trigger_skill = qobject_cast<const TriggerSkill *>(skill);
                if (trigger_skill && trigger_skill->getViewAsSkill()) {
                    const ViewAsSkill *vsskill = qobject_cast<const ViewAsSkill *>(trigger_skill->getViewAsSkill());
                    if (vsskill && vsskill->isEnabledAtNullification(this))
                        return true;
                }
            }
        }
    }

    return false;
}

bool ServerPlayer::pindian(ServerPlayer *target, const QString &reason, const Card *card1)
{
    room->tryPause();

    LogMessage log;
    log.type = "#Pindian";
    log.from = this;
    log.to << target;
    room->sendLog(log);

    const Card *card2 = NULL;
    PindianStruct pindian_struct;
    pindian_struct.from = this;
    pindian_struct.to = target;
    if (card1 != NULL)
        pindian_struct.from_card = card1;
    pindian_struct.reason = reason;

    PindianStruct *pindian = &pindian_struct; //for tmp record.
    if (card1 == NULL) {
        //QList<const Card *> cards = room->askForPindianRace(this, target, reason);
        //card1 = cards.first();
        //card2 = cards.last();

        card1 = room->askForPindian(this, this, target, reason, pindian);
        //@todo: fix UI and log
        //if (targets.first()->isShownHandcard(card1->getEffectiveId()))
        //    need_reveal = false;
        //room->showCard(targets.first(), card1->getEffectiveId());
        CardMoveReason reason1(CardMoveReason::S_REASON_PINDIAN, objectName(), target->objectName(), pindian_struct.reason, QString());
        room->moveCardTo(card1, this, NULL, Player::PlaceTable, reason1, false);

        card2 = room->askForPindian(target, this, target, reason, pindian);
        CardMoveReason reason2(CardMoveReason::S_REASON_PINDIAN, target->objectName());
        room->moveCardTo(card2, target, NULL, Player::PlaceTable, reason2, true);

    } else {
        if (card1->isVirtualCard()) {
            int card_id = card1->getEffectiveId();
            card1 = Sanguosha->getCard(card_id);
        }
        CardMoveReason reason1(CardMoveReason::S_REASON_PINDIAN, objectName(), target->objectName(), pindian_struct.reason, QString());
        room->moveCardTo(card1, this, NULL, Player::PlaceTable, reason1, false);

        card2 = room->askForPindian(target, this, target, reason, pindian);
        CardMoveReason reason2(CardMoveReason::S_REASON_PINDIAN, target->objectName());
        room->moveCardTo(card2, target, NULL, Player::PlaceTable, reason2, false);
    }

    if (card1 == NULL || card2 == NULL)
        return false;

    //PindianStruct pindian_struct;
    //pindian_struct.from = this;
    //pindian_struct.to = target;
    pindian_struct.from_card = card1;
    pindian_struct.to_card = card2;
    pindian_struct.from_number = card1->getNumber();
    pindian_struct.to_number = card2->getNumber();
    //pindian_struct.askedPlayer = NULL;
    //pindian_struct.reason = reason;

    //CardMoveReason reason1(CardMoveReason::S_REASON_PINDIAN, pindian_struct.from->objectName(), pindian_struct.to->objectName(), pindian_struct.reason, QString());
    //room->moveCardTo(pindian_struct.from_card, pindian_struct.from, NULL, Player::PlaceTable, reason1, false);

    //CardMoveReason reason2(CardMoveReason::S_REASON_PINDIAN, pindian_struct.to->objectName());
    //room->moveCardTo(pindian_struct.to_card, pindian_struct.to, NULL, Player::PlaceTable, reason2, false);

    LogMessage log2;
    log2.type = "$PindianResult";
    log2.from = pindian_struct.from;
    log2.card_str = QString::number(pindian_struct.from_card->getEffectiveId());
    room->sendLog(log2);

    log2.type = "$PindianResult";
    log2.from = pindian_struct.to;
    log2.card_str = QString::number(pindian_struct.to_card->getEffectiveId());
    room->sendLog(log2);

    RoomThread *thread = room->getThread();
    PindianStruct *pindian_star = &pindian_struct;
    QVariant data = QVariant::fromValue(pindian_star);
    Q_ASSERT(thread != NULL);
    thread->trigger(PindianVerifying, room, data);

    PindianStruct *new_star = data.value<PindianStruct *>();
    pindian_struct.from_number = new_star->from_number;
    pindian_struct.to_number = new_star->to_number;
    pindian_struct.success = (new_star->from_number > new_star->to_number);

    log.type = pindian_struct.success ? "#PindianSuccess" : "#PindianFailure";
    log.from = this;
    log.to.clear();
    log.to << target;
    log.card_str.clear();
    room->sendLog(log);

    JsonArray arg;
    arg << (int)S_GAME_EVENT_REVEAL_PINDIAN;
    arg << objectName();
    arg << pindian_struct.from_card->getEffectiveId();
    arg << target->objectName();
    arg << pindian_struct.to_card->getEffectiveId();
    arg << pindian_struct.success;
    arg << reason;
    room->doBroadcastNotify(S_COMMAND_LOG_EVENT, arg);

    pindian_star = &pindian_struct;
    data = QVariant::fromValue(pindian_star);
    thread->trigger(Pindian, room, data);

    if (room->getCardPlace(pindian_struct.from_card->getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason1(CardMoveReason::S_REASON_PINDIAN, pindian_struct.from->objectName(), pindian_struct.to->objectName(), pindian_struct.reason, QString());
        room->moveCardTo(pindian_struct.from_card, pindian_struct.from, NULL, Player::DiscardPile, reason1, true);
    }

    if (room->getCardPlace(pindian_struct.to_card->getEffectiveId()) == Player::PlaceTable) {
        CardMoveReason reason2(CardMoveReason::S_REASON_PINDIAN, pindian_struct.to->objectName());
        room->moveCardTo(pindian_struct.to_card, pindian_struct.to, NULL, Player::DiscardPile, reason2, true);
    }

    ChoiceMadeStruct s;
    s.player = this;
    s.type = ChoiceMadeStruct::Pindian;
    s.args << reason << objectName() << QString::number(pindian_struct.from_card->getEffectiveId()) << target->objectName()
           << QString::number(pindian_struct.to_card->getEffectiveId());
    QVariant decisionData = QVariant::fromValue(s);
    thread->trigger(ChoiceMade, room, decisionData);

    return pindian_struct.success;
}

void ServerPlayer::turnOver()
{
    setFaceUp(!faceUp());
    room->broadcastProperty(this, "faceup");

    LogMessage log;
    log.type = "#TurnOver";
    log.from = this;
    log.arg = faceUp() ? "face_up" : "face_down";
    room->sendLog(log);

    Q_ASSERT(room->getThread() != NULL);
    QVariant v = QVariant::fromValue(this);
    room->getThread()->trigger(TurnedOver, room, v);
}

bool ServerPlayer::changePhase(Player::Phase from, Player::Phase to)
{
    RoomThread *thread = room->getThread();
    Q_ASSERT(room->getThread() != NULL);

    setPhase(PhaseNone);

    PhaseChangeStruct phase_change;
    phase_change.player = this;
    phase_change.from = from;
    phase_change.to = to;
    QVariant data = QVariant::fromValue(phase_change);

    bool skip = thread->trigger(EventPhaseChanging, room, data);
    if (skip && to != NotActive) {
        setPhase(from);
        return true;
    }

    setPhase(to);
    room->broadcastProperty(this, "phase");

    if (!phases.isEmpty())
        phases.removeFirst();

    QVariant thisVariant = QVariant::fromValue(this);

    if (!thread->trigger(EventPhaseStart, room, thisVariant)) {
        if (getPhase() != NotActive)
            thread->trigger(EventPhaseProceeding, room, thisVariant);
    }
    if (getPhase() != NotActive)
        thread->trigger(EventPhaseEnd, room, thisVariant);

    return false;
}

void ServerPlayer::play(QList<Player::Phase> set_phases)
{
    if (!set_phases.isEmpty()) {
        if (!set_phases.contains(NotActive))
            set_phases << NotActive;
    } else {
        set_phases << RoundStart << Start << Judge << Draw << Play << Discard << Finish << NotActive;
    }

    phases = set_phases;
    _m_phases_state.clear();
    for (int i = 0; i < phases.size(); i++) {
        PhaseStruct _phase;
        _phase.phase = phases[i];
        _m_phases_state << _phase;
    }

    for (int i = 0; i < _m_phases_state.size(); i++) {
        if (isDead() || hasFlag("Global_TurnTerminated")) {
            changePhase(getPhase(), NotActive);
            break;
        }

        _m_phases_index = i;
        PhaseChangeStruct phase_change;
        phase_change.player = this;
        phase_change.from = getPhase();
        phase_change.to = phases[i];

        RoomThread *thread = room->getThread();
        setPhase(PhaseNone);
        QVariant data = QVariant::fromValue(phase_change);

        bool skip = thread->trigger(EventPhaseChanging, room, data);
        phase_change = data.value<PhaseChangeStruct>();
        _m_phases_state[i].phase = phases[i] = phase_change.to;

        setPhase(phases[i]);
        room->broadcastProperty(this, "phase");

        if (phases[i] != NotActive && (skip || _m_phases_state[i].skipped != 0)) {
            PhaseSkippingStruct s;
            s.isCost = _m_phases_state[i].skipped < 0;
            s.phase = phases[i];
            s.player = this;
            QVariant d = QVariant::fromValue(s);
            bool cancel_skip = thread->trigger(EventPhaseSkipping, room, d);
            if (!cancel_skip)
                continue;
        }

        QVariant thisVariant = QVariant::fromValue(this);

        if (!thread->trigger(EventPhaseStart, room, thisVariant)) {
            if (getPhase() != NotActive)
                thread->trigger(EventPhaseProceeding, room, thisVariant);
        }
        if (getPhase() != NotActive)
            thread->trigger(EventPhaseEnd, room, thisVariant);
        else
            break;
    }
}

QList<Player::Phase> &ServerPlayer::getPhases()
{
    return phases;
}

void ServerPlayer::skip(Player::Phase phase, bool isCost, bool sendLog)
{
    for (int i = _m_phases_index; i < _m_phases_state.size(); i++) {
        if (_m_phases_state[i].phase == phase) {
            if (_m_phases_state[i].skipped != 0) {
                if (isCost && _m_phases_state[i].skipped == 1)
                    _m_phases_state[i].skipped = -1;
                return;
            }
            _m_phases_state[i].skipped = (isCost ? -1 : 1);
            //defaultly skip all phases even someone has same pahses.
            //break;
        }
    }

    static QStringList phase_strings;
    if (phase_strings.isEmpty())
        phase_strings << "round_start"
                      << "start"
                      << "judge"
                      << "draw"
                      << "play"
                      << "discard"
                      << "finish"
                      << "not_active";
    int index = static_cast<int>(phase);

    if (sendLog) {
        LogMessage log;
        log.type = "#SkipPhase";
        log.from = this;
        log.arg = phase_strings.at(index);
        room->sendLog(log);
    }
}

void ServerPlayer::insertPhases(QList<Player::Phase> new_phases, int index)
{
    if (index == -1)
        index = _m_phases_index;
    for (int i = 0; i < new_phases.size(); i++) {
        PhaseStruct _phase;
        _phase.phase = new_phases[i];
        phases.insert(index + i, new_phases[i]);
        _m_phases_state.insert(index + i, _phase);
        //_m_phases_state << _phase;
    }
    //PhaseStruct _phase;
    //_phase.phase = phase;
    //phases.insert(index, phase);
    //_m_phases_state.insert(index, _phase);
}

void ServerPlayer::exchangePhases(Player::Phase phase1, Player::Phase phase2)
{
    PhaseStruct _phase1;
    PhaseStruct _phase2;

    int index1 = phases.indexOf(phase1);
    int index2 = phases.indexOf(phase2);
    // make sure that "_m_phases_state" has already contain informations from "phases"
    if (index1 > -1 && index2 > -1) {
        _phase1 = _m_phases_state[index1];
        _phase2 = _m_phases_state[index2];

        phases.removeAt(index1);
        phases.insert(index1, phase2);
        _m_phases_state.removeAt(index1);
        _m_phases_state.insert(index1, _phase2);

        phases.removeAt(index2);
        phases.insert(index2, phase1);
        _m_phases_state.removeAt(index2);
        _m_phases_state.insert(index2, _phase1);
    }
}

bool ServerPlayer::isSkipped(Player::Phase phase)
{
    for (int i = _m_phases_index; i < _m_phases_state.size(); i++) {
        if (_m_phases_state[i].phase == phase)
            return (_m_phases_state[i].skipped != 0);
    }
    return false;
}

void ServerPlayer::gainMark(const QString &mark, int n)
{
    MarkChangeStruct change;
    change.name = mark;
    change.num = n;
    change.player = this;
    QVariant n_data = QVariant::fromValue(change);
    if (mark.startsWith("@")) {
        if (room->getThread()->trigger(PreMarkChange, room, n_data))
            return;
        n = n_data.value<MarkChangeStruct>().num;
    }
    if (n == 0)
        return;
    if (n < 0) {
        loseMark(mark, -n);
        return;
    }

    int value = getMark(mark) + n;

    LogMessage log;
    log.type = "#GetMark";
    log.from = this;
    log.arg = mark;
    log.arg2 = QString::number(n);

    room->sendLog(log);
    room->setPlayerMark(this, mark, value);

    if (mark.startsWith("@"))
        room->getThread()->trigger(MarkChanged, room, n_data);
}

void ServerPlayer::loseMark(const QString &mark, int n)
{
    if (getMark(mark) == 0)
        return;
    MarkChangeStruct change;
    change.name = mark;
    change.num = -n;
    change.player = this;

    QVariant n_data = QVariant::fromValue(change);

    if (mark.startsWith("@")) {
        if (room->getThread()->trigger(PreMarkChange, room, n_data))
            return;
        n = -(n_data.value<MarkChangeStruct>().num);
    }

    if (n == 0)
        return;
    if (n < 0) {
        gainMark(mark, -n);
        return;
    }

    int value = getMark(mark) - n;
    if (value < 0) {
        value = 0;
        n = getMark(mark);
    }

    LogMessage log;
    log.type = "#LoseMark";
    log.from = this;
    log.arg = mark;
    log.arg2 = QString::number(n);

    room->sendLog(log);
    room->setPlayerMark(this, mark, value);

    if (mark.startsWith("@"))
        room->getThread()->trigger(MarkChanged, room, n_data);
}

void ServerPlayer::loseAllMarks(const QString &mark_name)
{
    loseMark(mark_name, getMark(mark_name));
}

void ServerPlayer::addSkill(const QString &skill_name)
{
    Player::addSkill(skill_name);
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_ADD_SKILL;
    args << objectName();
    args << skill_name;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void ServerPlayer::loseSkill(const QString &skill_name)
{
    Player::loseSkill(skill_name);
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_LOSE_SKILL;
    args << objectName();
    args << skill_name;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

void ServerPlayer::setGender(General::Gender gender)
{
    if (gender == getGender())
        return;
    Player::setGender(gender);
    JsonArray args;
    args << QSanProtocol::S_GAME_EVENT_CHANGE_GENDER;
    args << objectName();
    args << (int)gender;
    room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, args);
}

bool ServerPlayer::isOnline() const
{
    return getState() == "online";
}

void ServerPlayer::setAI(AI *ai)
{
    this->ai = ai;
}

AI *ServerPlayer::getAI() const
{
    if (getState() == "online")
        return NULL;
    else if (getState() != "robot" && !Config.EnableCheat)
        return trust_ai;
    else
        return ai;
}

AI *ServerPlayer::getSmartAI() const
{
    return ai;
}

void ServerPlayer::addVictim(ServerPlayer *victim)
{
    victims.append(victim);
}

QList<ServerPlayer *> ServerPlayer::getVictims() const
{
    return victims;
}
/*
void ServerPlayer::setNext(ServerPlayer *next)
{
    this->next = next;
}

ServerPlayer *ServerPlayer::getNext() const
{
    return next;
}

ServerPlayer *ServerPlayer::getNextAlive(int n) const
{
    bool hasAlive = (room->getAlivePlayers().length() > 0);
    ServerPlayer *next = const_cast<ServerPlayer *>(this);
    if (!hasAlive)
        return next;
    for (int i = 0; i < n; i++) {
        do
            next = next->next;
        while (next->isDead());
    }
    return next;
}*/

int ServerPlayer::getGeneralMaxHp() const
{
    int max_hp = 0;

    if (getGeneral2() == NULL)
        max_hp = getGeneral()->getMaxHp();
    else {
        int first = getGeneral()->getMaxHp();
        int second = getGeneral2()->getMaxHp();

        int plan = Config.MaxHpScheme;
        if (Config.GameMode.contains("_mini_") || Config.GameMode == "custom_scenario")
            plan = 1;

        switch (plan) {
        case 3:
            max_hp = (first + second) / 2;
            break;
        case 2:
            max_hp = qMax(first, second);
            break;
        case 1:
            max_hp = qMin(first, second);
            break;
        default:
            max_hp = first + second - Config.Scheme0Subtraction;
            break;
        }

        max_hp = qMax(max_hp, 1);
    }

    if (room->hasWelfare(this))
        max_hp++;

    return max_hp;
}

QString ServerPlayer::getGameMode() const
{
    return room->getMode();
}

QString ServerPlayer::getIp() const
{
    if (socket)
        return socket->peerAddress();
    else
        return QString();
}

void ServerPlayer::introduceTo(ServerPlayer *player)
{
    QString screen_name = screenName();
    QString avatar = property("avatar").toString();

    JsonArray introduce_str;
    introduce_str << objectName() << screen_name.toUtf8().toBase64() << avatar;

    if (player)
        room->doNotify(player, S_COMMAND_ADD_PLAYER, introduce_str);
    else {
        QList<ServerPlayer *> players = room->getPlayers();
        players.removeOne(this);
        room->doBroadcastNotify(players, S_COMMAND_ADD_PLAYER, introduce_str);
    }
}

void ServerPlayer::marshal(ServerPlayer *player) const
{
    room->notifyProperty(player, this, "maxhp");
    room->notifyProperty(player, this, "hp");
    room->notifyProperty(player, this, "dyingFactor");

    if (getKingdom() != getGeneral()->getKingdom())
        room->notifyProperty(player, this, "kingdom");

    if (isAlive()) {
        room->notifyProperty(player, this, "seat");
        if (getPhase() != Player::NotActive)
            room->notifyProperty(player, this, "phase");
    } else {
        room->notifyProperty(player, this, "alive");
        room->notifyProperty(player, this, "role");
        room->doNotify(player, S_COMMAND_KILL_PLAYER, QVariant(objectName()));
    }

    if (!faceUp())
        room->notifyProperty(player, this, "faceup");

    if (isChained())
        room->notifyProperty(player, this, "chained");

    room->notifyProperty(player, this, "removed");

    QList<ServerPlayer *> players;
    players << player;

    QList<CardsMoveStruct> moves;

    if (!isKongcheng()) {
        CardsMoveStruct move;
        foreach (const Card *card, handcards) {
            move.card_ids << card->getId();
            if (player == this) {
                WrappedCard *wrapped = qobject_cast<WrappedCard *>(room->getCard(card->getId()));
                if (wrapped->isModified())
                    room->notifyUpdateCard(player, card->getId(), wrapped);
            }
        }
        move.from_place = DrawPile;
        move.to_player_name = objectName();
        move.to_place = PlaceHand;

        if (player == this)
            move.to = player;

        moves << move;
    }

    if (hasEquip()) {
        CardsMoveStruct move;
        foreach (const Card *card, getEquips()) {
            move.card_ids << card->getId();
            WrappedCard *wrapped = qobject_cast<WrappedCard *>(room->getCard(card->getId()));
            if (wrapped->isModified())
                room->notifyUpdateCard(player, card->getId(), wrapped);
        }
        move.from_place = DrawPile;
        move.to_player_name = objectName();
        move.to_place = PlaceEquip;

        moves << move;
    }

    if (!getJudgingAreaID().isEmpty()) {
        CardsMoveStruct move;
        foreach (int card_id, getJudgingAreaID()) {
            move.card_ids << card_id;
            WrappedCard *wrapped = qobject_cast<WrappedCard *>(room->getCard(card_id));
            if (wrapped->isModified())
                room->notifyUpdateCard(player, card_id, wrapped);
        }
        move.from_place = DrawPile;
        move.to_player_name = objectName();
        move.to_place = PlaceDelayedTrick;

        moves << move;
    }

    if (!moves.isEmpty()) {
        room->notifyMoveCards(true, moves, false, players);
        room->notifyMoveCards(false, moves, false, players);
    }

    if (!getPileNames().isEmpty()) {
        CardsMoveStruct move;
        move.from_place = DrawPile;
        move.to_player_name = objectName();
        move.to_place = PlaceSpecial;
        foreach (QString pile, piles.keys()) {
            move.card_ids.clear();
            move.card_ids.append(piles[pile]);
            move.to_pile_name = pile;

            QList<CardsMoveStruct> moves2;
            moves2 << move;

            bool open = pileOpen(pile, player->objectName());

            room->notifyMoveCards(true, moves2, open, players);
            room->notifyMoveCards(false, moves2, open, players);
        }
    }

    JsonArray arg_shownhandcard;
    arg_shownhandcard << objectName();
    arg_shownhandcard << JsonUtils::toJsonArray(shown_handcards);
    room->doNotify(player, S_COMMAND_SET_SHOWN_HANDCARD, arg_shownhandcard);

    JsonArray arg_brokenIds;
    arg_brokenIds << objectName();
    arg_brokenIds << JsonUtils::toJsonArray(broken_equips);
    room->doNotify(player, S_COMMAND_SET_BROKEN_EQUIP, arg_brokenIds);

    foreach (QString mark_name, marks.keys()) {
        if (mark_name.startsWith("@")) {
            int value = getMark(mark_name);
            if (value > 0) {
                JsonArray arg_mark;
                arg_mark << objectName();
                arg_mark << mark_name;
                arg_mark << value;
                room->doNotify(player, S_COMMAND_SET_MARK, arg_mark);
            }
        }
    }

    foreach (const Skill *skill, getVisibleSkillList(true)) {
        //should not nofity the lord skill
        if (skill->isLordSkill() && !hasLordSkill(skill->objectName()))
            continue;
        QString skill_name = skill->objectName();
        JsonArray arg_acquire;
        arg_acquire << S_GAME_EVENT_ACQUIRE_SKILL;
        arg_acquire << objectName();
        arg_acquire << skill_name;
        room->doNotify(player, S_COMMAND_LOG_EVENT, arg_acquire);
    }

    foreach (const QString &invalid_name, skill_invalid) {
        JsonArray arg_invalid;
        arg_invalid << objectName() << invalid_name << true;
        room->doNotify(player, S_COMMAND_SET_SKILL_INVALIDITY, arg_invalid);
    }

    //for AvatarTooltip
    JsonArray arg_tooltip;
    arg_tooltip << QSanProtocol::S_GAME_EVENT_UPDATE_SKILL;
    room->doNotify(player, QSanProtocol::S_COMMAND_LOG_EVENT, arg_tooltip);

    //since "banling", we should notify hp after notifying skill
    if (this->hasSkill("banling")) {
        room->notifyProperty(player, this, "renhp");
        room->notifyProperty(player, this, "linghp");
    }

    if (this->hasSkill("anyun", true)) {
        QString g = hidden_generals.join("|");
        JsonArray arg;
        arg << objectName();
        arg << g;

        room->doNotify(player, S_COMMAND_SET_HIDDEN_GENERAL, arg);
    }

    foreach (QString flag, flags)
        room->notifyProperty(player, this, "flags", flag);

    foreach (QString item, history.keys()) {
        int value = history.value(item);
        if (value > 0) {
            JsonArray arg;
            arg << item;
            arg << value;

            room->doNotify(player, S_COMMAND_ADD_HISTORY, arg);
        }
    }

    if (hasShownRole()) {
        room->notifyProperty(player, this, "role");
        room->notifyProperty(player, this, "role_shown"); // notify client!!
    }

    //for huashen  like skill pingyi
    QString huashen_skill = this->tag.value("Huashen_skill", QString()).toString();
    QString huashen_target = this->tag.value("Huashen_target", QString()).toString();
    if (huashen_skill != NULL && huashen_target != NULL) {
        JsonArray huanshen_arg;
        huanshen_arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
        huanshen_arg << objectName();
        huanshen_arg << huashen_target;
        huanshen_arg << huashen_skill;
        room->doNotify(player, QSanProtocol::S_COMMAND_LOG_EVENT, huanshen_arg);
    }

    // for chaoren
    if (player == this && hasSkill("chaoren"))
        room->notifyProperty(player, this, "chaoren");
}

void ServerPlayer::addToPile(const QString &pile_name, const Card *card, bool open, QList<ServerPlayer *> open_players)
{
    QList<int> card_ids;
    if (card->isVirtualCard())
        card_ids = card->getSubcards();
    else
        card_ids << card->getEffectiveId();
    return addToPile(pile_name, card_ids, open, open_players);
}

void ServerPlayer::addToPile(const QString &pile_name, int card_id, bool open, QList<ServerPlayer *> open_players)
{
    QList<int> card_ids;
    card_ids << card_id;
    return addToPile(pile_name, card_ids, open, open_players);
}

void ServerPlayer::addToPile(const QString &pile_name, QList<int> card_ids, bool open, QList<ServerPlayer *> open_players)
{
    return addToPile(pile_name, card_ids, open, CardMoveReason(), open_players);
}

void ServerPlayer::addToPile(const QString &pile_name, QList<int> card_ids, bool open, CardMoveReason reason, QList<ServerPlayer *> open_players)
{
    if (open)
        open_players = room->getAllPlayers();
    else if (open_players.isEmpty())
        open_players << this;
    foreach (ServerPlayer *p, open_players)
        setPileOpen(pile_name, p->objectName());
    piles[pile_name].append(card_ids);

    CardsMoveStruct move;
    move.card_ids = card_ids;
    move.to = this;
    move.to_place = Player::PlaceSpecial;
    move.reason = reason;
    room->moveCardsAtomic(move, open);
}

void ServerPlayer::addToShownHandCards(QList<int> card_ids)
{
    //check card_id
    foreach (int id, card_ids)
        if (shown_handcards.contains(id) || room->getCardOwner(id) != this)
            card_ids.removeOne(id);
    if (card_ids.isEmpty())
        return;

    shown_handcards.append(card_ids);

    JsonArray arg;
    arg << objectName();
    arg << JsonUtils::toJsonArray(shown_handcards);

    foreach (ServerPlayer *player, room->getAllPlayers(true))
        room->doNotify(player, S_COMMAND_SET_SHOWN_HANDCARD, arg);

    LogMessage log;
    log.type = "$AddShownHand";
    log.from = this;
    log.card_str = IntList2StringList(card_ids).join("+");
    room->sendLog(log);
    room->getThread()->delay();

    ShownCardChangedStruct s;
    s.ids = card_ids;
    s.player = this;
    s.shown = true;
    QVariant v = QVariant::fromValue(s);
    room->getThread()->trigger(ShownCardChanged, room, v);
    //need set Konwn cards?
    //room->doNotify(player, S_COMMAND_SET_KNOWN_CARDS, arg1);
}

void ServerPlayer::removeShownHandCards(QList<int> card_ids, bool sendLog, bool moveFromHand)
{
    if (card_ids.isEmpty())
        return;

    foreach (int id, card_ids)
        shown_handcards.removeOne(id);

    JsonArray arg;
    arg << objectName();
    arg << JsonUtils::toJsonArray(shown_handcards);

    foreach (ServerPlayer *player, room->getAllPlayers(true))
        room->doNotify(player, S_COMMAND_SET_SHOWN_HANDCARD, arg);

    if (sendLog) {
        LogMessage log;
        log.type = "$RemoveShownHand";
        log.from = this;
        log.card_str = IntList2StringList(card_ids).join("+");
        room->sendLog(log);
        room->getThread()->delay();
    }

    ShownCardChangedStruct s;
    s.ids = card_ids;
    s.player = this;
    s.shown = false;
    s.moveFromHand = moveFromHand;
    QVariant v = QVariant::fromValue(s);
    room->getThread()->trigger(ShownCardChanged, room, v);
}

void ServerPlayer::addBrokenEquips(QList<int> card_ids)
{
    broken_equips.append(card_ids);

    JsonArray arg;
    arg << objectName();
    arg << JsonUtils::toJsonArray(broken_equips);

    foreach (ServerPlayer *player, room->getAllPlayers(true))
        room->doNotify(player, S_COMMAND_SET_BROKEN_EQUIP, arg);

    LogMessage log;
    log.type = "$AddBrokenEquip";
    log.from = this;
    log.card_str = IntList2StringList(card_ids).join("+");
    room->sendLog(log);
    room->getThread()->delay();

    BrokenEquipChangedStruct b;
    b.player = this;
    b.ids = card_ids;
    b.broken = true;
    QVariant bv = QVariant::fromValue(b);
    room->getThread()->trigger(BrokenEquipChanged, room, bv);
}

void ServerPlayer::removeBrokenEquips(QList<int> card_ids, bool sendLog, bool moveFromEquip)
{
    if (card_ids.isEmpty())
        return;

    foreach (int id, card_ids)
        broken_equips.removeOne(id);

    JsonArray arg;
    arg << objectName();
    arg << JsonUtils::toJsonArray(broken_equips);

    foreach (ServerPlayer *player, room->getAllPlayers(true))
        room->doNotify(player, S_COMMAND_SET_BROKEN_EQUIP, arg);

    if (sendLog) {
        LogMessage log;
        log.type = "$RemoveBrokenEquip";
        log.from = this;
        log.card_str = IntList2StringList(card_ids).join("+");
        room->sendLog(log);
        room->getThread()->delay();
    }
    BrokenEquipChangedStruct b;
    b.player = this;
    b.ids = card_ids;
    b.broken = false;
    b.moveFromEquip = moveFromEquip;
    QVariant bv = QVariant::fromValue(b);
    room->getThread()->trigger(BrokenEquipChanged, room, bv);
}

void ServerPlayer::addHiddenGenerals(const QStringList &generals)
{
    hidden_generals << generals;

    QString g = hidden_generals.join("|");
    JsonArray arg;
    arg << objectName();
    arg << g;

    room->doBroadcastNotify(S_COMMAND_SET_HIDDEN_GENERAL, arg);
    //room->doNotify(this, S_COMMAND_SET_HIDDEN_GENERAL, arg);
}

void ServerPlayer::removeHiddenGenerals(const QStringList &generals)
{
    foreach (QString name, generals)
        hidden_generals.removeOne(name);

    QString g = hidden_generals.join("|");
    JsonArray arg;
    arg << objectName();
    arg << g;
    room->doBroadcastNotify(S_COMMAND_SET_HIDDEN_GENERAL, arg);

    shown_hidden_general = QString();
    JsonArray arg1;
    arg1 << objectName();
    arg1 << QString();
    room->doBroadcastNotify(S_COMMAND_SET_SHOWN_HIDDEN_GENERAL, arg1);

    foreach (QString name, generals) {
        room->touhouLogmessage("#RemoveHiddenGeneral", this, name);
        foreach (const Skill *skill, Sanguosha->getGeneral(name)->getVisibleSkillList()) {
            room->handleAcquireDetachSkills(this, "-" + skill->objectName(), true);
        }
    }
    room->filterCards(this, this->getCards("hes"), true);
}

void ServerPlayer::gainAnExtraTurn()
{
    room->getThread()->setNextExtraTurn(this);
}

void ServerPlayer::showHiddenSkill(const QString &skill_name)
{
    if (skill_name == NULL)
        return;
    if (!canShowHiddenSkill() || !isHiddenSkill(skill_name))
        return;
    if (hasSkill(skill_name)) {
        QString generalName;
        foreach (QString name, hidden_generals) {
            const General *hidden = Sanguosha->getGeneral(name);
            if (hidden->hasSkill(skill_name)) {
                generalName = name;
                break;
            }
        }
        if (generalName != NULL) {
            room->touhouLogmessage("#ShowHiddenGeneral", this, generalName);

            JsonArray arg;
            arg << (int)QSanProtocol::S_GAME_EVENT_HUASHEN;
            arg << objectName();
            arg << generalName;
            arg << skill_name;

            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

            shown_hidden_general = generalName;
            JsonArray arg1;
            arg1 << objectName();
            arg1 << generalName;
            room->doBroadcastNotify(S_COMMAND_SET_SHOWN_HIDDEN_GENERAL, arg1);

            foreach (const Skill *skill, Sanguosha->getGeneral(generalName)->getVisibleSkillList()) {
                if (!skill->isLordSkill() && !skill->isAttachedLordSkill() && skill->getFrequency() != Skill::Limited && skill->getFrequency() != Skill::Wake
                    && skill->getFrequency() != Skill::Eternal)
                    room->handleAcquireDetachSkills(this, skill->objectName(), true);
            }
            room->filterCards(this, this->getCards("hes"), true);

            //keep showing huashen for a short time
            if (getPhase() == Player::Finish)
                room->getThread()->delay(1000);
        }
    }
}

QStringList ServerPlayer::checkTargetModSkillShow(const CardUseStruct &use)
{
    if (use.card == NULL || use.card->getTypeId() == Card::TypeSkill)
        return QStringList();
    if (!canShowHiddenSkill())
        return QStringList();

    QList<const TargetModSkill *> tarmods;
    foreach (QString hidden, getHiddenGenerals()) {
        const General *g = Sanguosha->getGeneral(hidden);
        foreach (const Skill *skill, g->getSkillList()) {
            if (skill->inherits("TargetModSkill")) {
                const TargetModSkill *tarmod = qobject_cast<const TargetModSkill *>(skill);
                tarmods << tarmod;
            }
        }
    }
    if (tarmods.isEmpty())
        return QStringList();

    QSet<QString> showExtraTarget;
    QSet<QString> disShowExtraTarget;
    QSet<QString> showResidueNum;
    QSet<QString> disShowResidueNum;
    QSet<QString> showDistanceLimit; //QSet<QString> disShowDistanceLimit;
    QSet<QString> showTargetFix; // only for skill tianqu
    QSet<QString> showTargetProhibit; //only for skill tianqu
    //check extra target
    int num = use.to.length() - 1;
    if (num >= 1) {
        foreach (const TargetModSkill *tarmod, tarmods) {
            if (tarmod->getExtraTargetNum(use.from, use.card) >= num)
                showExtraTarget << tarmod->objectName();
            else
                disShowExtraTarget << tarmod->objectName();
        }
    }

    //check ResidueNum
    //only consider the folloing cards
    if (use.card->isKindOf("Slash") || use.card->isKindOf("Analeptic")) {
        num = 0;
        if (use.card->isKindOf("Slash"))
            num = use.from->getSlashCount() - 1;
        else if (use.card->isKindOf("Analeptic"))
            num = use.from->getAnalepticCount() - 1;

        if (num >= 1) {
            foreach (const TargetModSkill *tarmod, tarmods) {
                if (tarmod->getResidueNum(use.from, use.card) >= num)
                    showResidueNum << tarmod->objectName();
                else
                    disShowResidueNum << tarmod->objectName();
            }
        }
    }

    //check DistanceLimit
    //only consider the folloing cards
    if (use.card->isKindOf("Slash") || use.card->isKindOf("SupplyShortage") || use.card->isKindOf("Snatch")) {
        int distance = 1;
        foreach (ServerPlayer *p, use.to) {
            if (use.from->distanceTo(p) > distance)
                distance = use.from->distanceTo(p);

            distance = distance - 1;
            if (distance >= 1) {
                foreach (const TargetModSkill *tarmod, tarmods) {
                    if (tarmod->getDistanceLimit(use.from, use.card) >= distance)
                        showDistanceLimit << tarmod->objectName();
                    //else
                    //    disShowDistanceLimit << tarmod->objectName();
                }
            }
        }
    }

    //check TargetFix
    //only consider the folloing cards
    //Peach , EquipCard , ExNihilo, Analeptic, Lightning

    use.card->setFlags("IgnoreFailed");
    if (use.card->targetFixed() && !use.to.contains(use.from) && !use.card->isKindOf("AOE") && !use.card->isKindOf("GlobalEffect")) {
        if (isHiddenSkill("tianqu") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY)
            showTargetFix << "tianqu";
    }
    use.card->setFlags("-IgnoreFailed");

    //check prohibit
    foreach (ServerPlayer *p, use.to) {
        if (use.from->isProhibited(p, use.card) && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            showTargetProhibit << "tianqu";
            break;
        } else if (use.card->isKindOf("Peach")) {
            if (!p->isWounded() && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
                showTargetProhibit << "tianqu";
                break;
            }
            if (p != use.from && (!p->hasLordSkill("yanhui") || p->getKingdom() != "zhan") && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
                showTargetProhibit << "tianqu";
                break;
            }
        } else if (use.card->isKindOf("DelayedTrick") && p->containsTrick(use.card->objectName()) && Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_PLAY) {
            showTargetProhibit << "tianqu";
            break;
        }
    }

    QSet<QString> shows = showExtraTarget | showDistanceLimit | showResidueNum | showTargetFix | showTargetProhibit;
    shows = shows - disShowExtraTarget - disShowResidueNum;

    return shows.toList();
}

void ServerPlayer::copyFrom(ServerPlayer *sp)
{
    ServerPlayer *b = this;
    ServerPlayer *a = sp;

    b->handcards = QList<const Card *>(a->handcards);
    b->phases = QList<ServerPlayer::Phase>(a->phases);
    b->selected = QStringList(a->selected);

    Player *c = b;
    c->copyFrom(a);
}

bool ServerPlayer::CompareByActionOrder(ServerPlayer *a, ServerPlayer *b)
{
    Room *room = a->getRoom();
    return room->getFront(a, b) == a;
}
