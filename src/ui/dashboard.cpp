#include "dashboard.h"
#include "aux-skills.h"
#include "client.h"
#include "engine.h"
#include "graphicspixmaphoveritem.h"
#include "heroskincontainer.h"
#include "roomscene.h"
#include "settings.h"
#include "standard.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPixmapCache>
#include <QTimer>

#ifdef Q_OS_WIN
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

using namespace QSanProtocol;

Dashboard::Dashboard(QGraphicsItem *widget) //QGraphicsPixmapItem *widget
    : button_widget(widget)
    , selected(nullptr)
    , view_as_skill(nullptr)
    , filter(nullptr)
{
    Q_ASSERT(button_widget);
    _dlayout = &G_DASHBOARD_LAYOUT;
    _m_layout = _dlayout;
    m_player = Self;
    _m_leftFrame = _m_rightFrame = _m_middleFrame = nullptr;
    _m_rightFrameBg = nullptr;
    animations = new EffectAnimation();
    pending_card = nullptr;

    leftHiddenMark = nullptr; //?? initialization?
    rightHiddenMark = nullptr;
    _m_pile_expanded = QMap<QString, QList<int>>(); //QStringList();
    for (int i = 0; i < 5; i++) {
        _m_equipSkillBtns[i] = nullptr;
        _m_isEquipsAnimOn[i] = false;
    }
    // At this stage, we cannot decide the dashboard size yet, the whole
    // point in creating them here is to allow PlayerCardContainer to
    // anchor all controls and widgets to the correct frame.
    //
    // Note that 20 is just a random plug-in so that we can proceed with
    // control creation, the actual width is updated when setWidth() is
    // called by its graphics parent.
    //
    _m_width = G_DASHBOARD_LAYOUT.m_leftWidth + G_DASHBOARD_LAYOUT.m_rightWidth + 20;
    if (ServerInfo.Enable2ndGeneral)
        _m_width = G_DASHBOARD_LAYOUT.m_leftWidth + G_DASHBOARD_LAYOUT.m_rightWidthDouble + 20;

    leftDisableShowLock = nullptr;
    rightDisableShowLock = nullptr;

    _createLeft();
    _createMiddle();
    _createRight();

    _m_skillNameItem = new QGraphicsPixmapItem(_m_rightFrame);

    // only do this after you create all frames.
    _createControls();
    _createExtraButtons();

    _m_sort_menu = new QMenu(RoomSceneInstance->mainWindow());
    //_m_carditem_context_menu = NULL;

    connect(Self, SIGNAL(chaoren_changed()), this, SLOT(updateChaoren()));
    connect(Self, SIGNAL(showncards_changed()), this, SLOT(updateShown()));
    connect(Self, SIGNAL(brokenEquips_changed()), this, SLOT(updateHandPile()));

#ifdef Q_OS_WIN
    taskbarButton = new QWinTaskbarButton(this);
    taskbarButton->setWindow(RoomSceneInstance->mainWindow()->windowHandle());
    QWinTaskbarProgress *prog = taskbarButton->progress();
    prog->setVisible(false);
    prog->setMinimum(0);
    prog->reset();
#endif
}

bool Dashboard::isAvatarUnderMouse()
{
    return _m_avatarArea->isUnderMouse();
}

void Dashboard::hideControlButtons()
{
    m_btnReverseSelection->hide();
    m_btnSortHandcard->hide();
}

void Dashboard::showControlButtons()
{
    m_btnReverseSelection->show();
    m_btnSortHandcard->show();
}

void Dashboard::showProgressBar(QSanProtocol::Countdown countdown)
{
    _m_progressBar->setCountdown(countdown);
    connect(_m_progressBar, SIGNAL(timedOut()), this, SIGNAL(progressBarTimedOut()));
    _m_progressBar->show();
#ifdef Q_OS_WIN
    if (_m_progressBar->hasTimer()) {
        connect(_m_progressBar, &QSanCommandProgressBar::timerStep, this, &Dashboard::updateTimedProgressBar, Qt::UniqueConnection);
        QWinTaskbarProgress *prog = taskbarButton->progress();
        prog->reset();
        prog->resume();
        prog->setMaximum(countdown.max);
        prog->setMinimum(0);
        prog->setValue(countdown.max - countdown.current);
        prog->show();
    }
#endif
}

void Dashboard::hideProgressBar()
{
    PlayerCardContainer::hideProgressBar();
#ifdef Q_OS_WIN
    QWinTaskbarProgress *prog = taskbarButton->progress();
    prog->hide();
    prog->reset();
    prog->resume();
#endif
}

#ifdef Q_OS_WIN
void Dashboard::updateTimedProgressBar(time_t val, time_t max)
{
    QWinTaskbarProgress *prog = taskbarButton->progress();
    prog->setMaximum(max);
    prog->setValue(max - val);

    if (val > max * 0.8)
        prog->stop();
    else if (val > max * 0.5)
        prog->pause();
    else
        prog->resume();
}
#endif

QGraphicsItem *Dashboard::getMouseClickReceiver()
{
    return _m_avatarIcon;
}

void Dashboard::_createLeft()
{
    QRect rect = QRect(0, 0, G_DASHBOARD_LAYOUT.m_leftWidth, G_DASHBOARD_LAYOUT.m_normalHeight);
    _paintPixmap(_m_leftFrame, rect, _getPixmap(QSanRoomSkin::S_SKIN_KEY_LEFTFRAME), this);
    _m_leftFrame->setZValue(-1000); // nobody should be under me.
    _createEquipBorderAnimations();
}

int Dashboard::getButtonWidgetWidth() const
{
    Q_ASSERT(button_widget);
    return button_widget->boundingRect().width();
}

void Dashboard::_createMiddle()
{
    // this is just a random rect. see constructor for more details
    QRect rect = QRect(0, 0, 1, G_DASHBOARD_LAYOUT.m_normalHeight);
    _paintPixmap(_m_middleFrame, rect, _getPixmap(QSanRoomSkin::S_SKIN_KEY_MIDDLEFRAME), this);
    _m_middleFrame->setZValue(-1000); // nobody should be under me.
    button_widget->setParentItem(_m_middleFrame);

    trusting_item = new QGraphicsRectItem(this);
    trusting_text = new QGraphicsSimpleTextItem(tr("Trusting ..."), this);
    trusting_text->setPos(boundingRect().width() / 2, 50);

    QBrush trusting_brush(G_DASHBOARD_LAYOUT.m_trustEffectColor);
    trusting_item->setBrush(trusting_brush);
    trusting_item->setOpacity(0.36);
    trusting_item->setZValue(1002.0);

    trusting_text->setFont(Config.BigFont);
    trusting_text->setBrush(Qt::white);
    trusting_text->setZValue(1002.1);

    trusting_item->hide();
    trusting_text->hide();
}

void Dashboard::_adjustComponentZValues(bool killed)
{
    PlayerCardContainer::_adjustComponentZValues(killed);
    // make sure right frame is on top because we have a lot of stuffs
    // attached to it, such as the rolecomboBox, which should not be under
    // middle frame
    _layUnder(_m_rightFrame);
    _layUnder(_m_leftFrame);
    _layUnder(_m_middleFrame);

    if (isHegemonyGameMode(ServerInfo.GameMode)) {
        if ((button_widget != nullptr) && (_m_middleFrame != nullptr) && (_m_hegemonyroleComboBox != nullptr))
            _layBetween(button_widget, _m_middleFrame, _m_hegemonyroleComboBox);
    } else {
        if ((button_widget != nullptr) && (_m_middleFrame != nullptr) && (_m_roleComboBox != nullptr))
            _layBetween(button_widget, _m_middleFrame, _m_roleComboBox);
    }

    if ((_m_rightFrameBg != nullptr) && (_m_faceTurnedIcon != nullptr))
        _layBetween(_m_rightFrameBg, _m_faceTurnedIcon, _m_equipRegions[3]);
    if (leftHiddenMark != nullptr)
        _layUnder(leftHiddenMark);
    if (rightHiddenMark != nullptr)
        _layUnder(rightHiddenMark);
    if (_m_smallAvatarArea != nullptr)
        _layUnder(_m_smallAvatarArea);
    if (_m_avatarArea != nullptr)
        _layUnder(_m_avatarArea);
    if (isHegemonyGameMode(ServerInfo.GameMode) && ServerInfo.Enable2ndGeneral && (_m_shadow_layer2 != nullptr))
        _layUnder(_m_shadow_layer2);
    if (isHegemonyGameMode(ServerInfo.GameMode) && (_m_shadow_layer1 != nullptr))
        _layUnder(_m_shadow_layer1);
    if (_m_faceTurnedIcon2 != nullptr)
        _layUnder(_m_faceTurnedIcon2);
    if (_m_faceTurnedIcon != nullptr)
        _layUnder(_m_faceTurnedIcon);
    if (_m_smallAvatarIcon != nullptr)
        _layUnder(_m_smallAvatarIcon);
    if (_m_avatarIcon != nullptr)
        _layUnder(_m_avatarIcon);
    //_layUnder(_m_rightFrameBg);
    //the following  items must be on top
    //_m_smallAvatarNameItem
    //_m_avatarNameItem
    //_m_dashboardKingdomColorMaskIcon
    //_m_dashboardSecondaryKingdomColorMaskIcon
    if (_m_hpBox != nullptr)
        _m_hpBox->setZValue(2000);
    if (_m_sub_hpBox != nullptr)
        _m_sub_hpBox->setZValue(2000);

    //_m_rightFrameBg->setZValue(3000);
}

int Dashboard::width()
{
    return _m_width;
}

void Dashboard::_createRight()
{ //40 equals diff between middlefarme and rightframe
    int rwidth = (ServerInfo.Enable2ndGeneral) ? G_DASHBOARD_LAYOUT.m_rightWidthDouble : G_DASHBOARD_LAYOUT.m_rightWidth;
    QRect rect = QRect(_m_width - rwidth, -40, rwidth, G_DASHBOARD_LAYOUT.m_normalHeight + 40);
    _paintPixmap(_m_rightFrame, rect, QPixmap(1, 1), _m_groupMain);
    _paintPixmap(_m_rightFrameBg, QRect(0, 0, rect.width(), rect.height()), _getPixmap(QSanRoomSkin::S_SKIN_KEY_RIGHTFRAME), _m_rightFrame);
    _m_rightFrame->setZValue(-1000); // nobody should be under me.

    _m_skillDock = new QSanInvokeSkillDock(_m_rightFrame);
    QRect avatar = G_DASHBOARD_LAYOUT.m_avatarArea; //(ServerInfo.Enable2ndGeneral) ? G_DASHBOARD_LAYOUT.m_avatarAreaDouble : G_DASHBOARD_LAYOUT.m_avatarArea; // m_headAvatarArea;

    if (ServerInfo.Enable2ndGeneral) {
        _m_skillDock->setPos(avatar.left() + 5, avatar.bottom() + G_DASHBOARD_LAYOUT.m_skillButtonsSize[0].height() - 25);
        _m_skillDock->setWidth(avatar.width() / 2);
    } else {
        _m_skillDock->setPos(avatar.left() + 25, avatar.bottom() + G_DASHBOARD_LAYOUT.m_skillButtonsSize[0].height() - 25);
        _m_skillDock->setWidth(avatar.width() - 50);
    }

    _m_rightSkillDock = new QSanInvokeSkillDock(_m_rightFrame);
    QRect avatar2 = G_DASHBOARD_LAYOUT.m_smallAvatarArea; //m_smallAvatarArea;//G_DASHBOARD_LAYOUT.;
    _m_rightSkillDock->setPos(avatar2.left() - 10, avatar2.bottom() + G_DASHBOARD_LAYOUT.m_skillButtonsSize[0].height() - 25);
    _m_rightSkillDock->setWidth(avatar2.width() - 50);

    _m_skillDock->setObjectName("left");
    _m_rightSkillDock->setObjectName("right");

    //hegemony
    if (isHegemonyGameMode(ServerInfo.GameMode)) {
        _m_shadow_layer1 = new QGraphicsRectItem(_m_rightFrame);

        _m_shadow_layer1->setRect(G_DASHBOARD_LAYOUT.m_avatarArea);
        if (ServerInfo.Enable2ndGeneral) {
            _m_shadow_layer1->setRect(G_DASHBOARD_LAYOUT.m_headAvatarArea);
            _m_shadow_layer2 = new QGraphicsRectItem(_m_rightFrame);
            _m_shadow_layer2->setRect(G_DASHBOARD_LAYOUT.m_smallAvatarArea);
        }

        _paintPixmap(leftHiddenMark, G_DASHBOARD_LAYOUT.m_hiddenMarkRegion3, _getPixmap(QSanRoomSkin::S_SKIN_KEY_HIDDEN_MARK), _m_rightFrame);
        if (ServerInfo.Enable2ndGeneral) {
            _paintPixmap(leftHiddenMark, G_DASHBOARD_LAYOUT.m_hiddenMarkRegion1, _getPixmap(QSanRoomSkin::S_SKIN_KEY_HIDDEN_MARK), _m_rightFrame);

            _paintPixmap(rightHiddenMark, G_DASHBOARD_LAYOUT.m_hiddenMarkRegion2, _getPixmap(QSanRoomSkin::S_SKIN_KEY_HIDDEN_MARK), _m_rightFrame);
        }

        connect(ClientInstance, &Client::head_preshowed, this, &Dashboard::updateHiddenMark);
        connect(ClientInstance, &Client::deputy_preshowed, this, &Dashboard::updateRightHiddenMark);
    }
}

void Dashboard::_updateFrames()
{
    // Here is where we adjust all frames to actual width
    int rwidth = (ServerInfo.Enable2ndGeneral) ? G_DASHBOARD_LAYOUT.m_rightWidthDouble : G_DASHBOARD_LAYOUT.m_rightWidth;
    QRect rect = QRect(G_DASHBOARD_LAYOUT.m_leftWidth, 0, width() - rwidth - G_DASHBOARD_LAYOUT.m_leftWidth, G_DASHBOARD_LAYOUT.m_normalHeight);
    _paintPixmap(_m_middleFrame, rect, _getPixmap(QSanRoomSkin::S_SKIN_KEY_MIDDLEFRAME), this);
    QRect rect2 = QRect(0, 0, width(), G_DASHBOARD_LAYOUT.m_normalHeight);
    trusting_item->setRect(rect2);
    trusting_item->setPos(0, 0);
    trusting_text->setPos((rect2.width() - Config.BigFont.pixelSize() * 4.5) / 2, (rect2.height() - Config.BigFont.pixelSize()) / 2);
    _m_rightFrame->setX(_m_width - rwidth);
    Q_ASSERT(button_widget);
    button_widget->setX(rect.width() - getButtonWidgetWidth());
    button_widget->setY(0);
}

void Dashboard::setTrust(bool trust)
{
    trusting_item->setVisible(trust);
    trusting_text->setVisible(trust);
}

void Dashboard::killPlayer()
{
    trusting_item->hide();
    trusting_text->hide();
    if (isHegemonyGameMode(ServerInfo.GameMode)) {
        _m_hegemonyroleComboBox->fix(m_player->getRole() == "careerist" ? "careerist" : m_player->getRole()); //m_player->getKingdom()
        _m_hegemonyroleComboBox->setEnabled(false);
    } else {
        _m_roleComboBox->fix(m_player->getRole());
        _m_roleComboBox->setEnabled(false);
    }

    _updateDeathIcon();
    _m_saveMeIcon->hide();
    if (_m_votesItem != nullptr)
        _m_votesItem->hide();
    if (_m_distanceItem != nullptr)
        _m_distanceItem->hide();

    _m_deathIcon->show();
    if (ServerInfo.GameMode == "04_1v3" && !Self->isLord()) {
        _m_votesGot = 6;
        updateVotes(false);
    }
}

void Dashboard::revivePlayer()
{
    _m_votesGot = 0;
    setGraphicsEffect(nullptr);
    Q_ASSERT(_m_deathIcon);
    _m_deathIcon->hide();
    refresh();
}

void Dashboard::setDeathColor()
{
    QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect();
    effect->setColor(_m_layout->m_deathEffectColor);
    effect->setStrength(1.0);
    setGraphicsEffect(effect);
    refresh();
}

bool Dashboard::_addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo)
{
    Player::Place place = moveInfo.to_place;
    if (place == Player::PlaceSpecial) {
        foreach (CardItem *card, card_items)
            card->setHomeOpacity(0.0);
        QPoint avatarArea_center = (ServerInfo.Enable2ndGeneral) ? _dlayout->m_avatarAreaDouble.center() : _dlayout->m_avatarArea.center();
        QPointF center = mapFromItem(_getAvatarParent(), avatarArea_center);
        QRectF rect = QRectF(0, 0, _dlayout->m_disperseWidth, 0);
        rect.moveCenter(center);
        _disperseCards(card_items, rect, Qt::AlignCenter, true, false);
        return true;
    }

    m_mutexCardItemsAnimationFinished.lock();
    _m_cardItemsAnimationFinished << card_items;
    m_mutexCardItemsAnimationFinished.unlock();

    if (place == Player::PlaceEquip)
        addEquips(card_items);
    else if (place == Player::PlaceDelayedTrick)
        addDelayedTricks(card_items);
    else if (place == Player::PlaceHand)
        addHandCards(card_items);

    adjustCards(true);
    return false;
}

void Dashboard::addHandCards(QList<CardItem *> &card_items)
{
    foreach (CardItem *card_item, card_items)
        _addHandCard(card_item);
    updateHandcardNum();
}

void Dashboard::_addHandCard(CardItem *card_item, bool prepend, const QString &footnote)
{
    if (ClientInstance->getStatus() == Client::Playing && (card_item->getCard() != nullptr))
        card_item->setEnabled(card_item->getCard()->isAvailable(Self));
    else
        card_item->setEnabled(false); //false

    card_item->setHomeOpacity(1.0);
    card_item->setRotation(0.0);
    card_item->setFlags(ItemIsFocusable);
    card_item->setZValue(0.1);
    if (!footnote.isEmpty()) {
        card_item->setFootnote(footnote);
        card_item->showFootnote();
    }
    if (prepend)
        m_handCards.prepend(card_item);
    else
        m_handCards.append(card_item);

    //connect(card_item, SIGNAL(context_menu_event_triggered()), this, SLOT(onCardItemContextMenu()));

    connect(card_item, SIGNAL(clicked()), this, SLOT(onCardItemClicked()));
    connect(card_item, SIGNAL(double_clicked()), this, SLOT(onCardItemDoubleClicked()));
    connect(card_item, SIGNAL(thrown()), this, SLOT(onCardItemThrown()));
    connect(card_item, SIGNAL(enter_hover()), this, SLOT(onCardItemHover()));
    connect(card_item, SIGNAL(leave_hover()), this, SLOT(onCardItemLeaveHover()));

    //connect(card_item, &CardItem::enter_hover, this, &Dashboard::bringSenderToTop);
    //connect(card_item, &CardItem::leave_hover, this, &Dashboard::resetSenderZValue);

    //card_item->installSceneEventFilter(this);
}

void Dashboard::selectCard(const QString &pattern, bool forward, bool multiple)
{
    if (!multiple && (selected != nullptr) && selected->isSelected())
        selected->clickItem();

    // find all cards that match the card type
    QList<CardItem *> matches;
    foreach (CardItem *card_item, m_handCards) {
        if (card_item->isEnabled() && (pattern == "." || card_item->getCard()->matchTypeOrName(pattern)))
            matches << card_item;
    }

    if (matches.isEmpty()) {
        if (!multiple || (selected == nullptr)) {
            unselectAll();
            return;
        }
    }

    int index = matches.indexOf(selected), n = matches.length();
    index = (index + (forward ? 1 : n - 1)) % n;

    CardItem *to_select = matches[index];
    if (!to_select->isSelected())
        to_select->clickItem();
    else if (to_select->isSelected() && (!multiple || (multiple && to_select != selected)))
        to_select->clickItem();
    selected = to_select;

    adjustCards();
}

void Dashboard::selectEquip(int position)
{
    int i = position - 1;
    if ((_m_equipCards[i] != nullptr) && _m_equipCards[i]->isMarkable()) {
        _m_equipCards[i]->mark(!_m_equipCards[i]->isMarked());
        update();
    }
}

void Dashboard::selectOnlyCard(bool need_only)
{
    if ((selected != nullptr) && selected->isSelected())
        selected->clickItem();

    int count = 0;

    QList<CardItem *> items;
    foreach (CardItem *card_item, m_handCards) {
        if (card_item->isEnabled()) {
            items << card_item;
            count++;
            if (need_only && count > 1) {
                unselectAll();
                return;
            }
        }
    }

    QList<int> equip_pos;
    for (int i = 0; i < 5; i++) {
        if ((_m_equipCards[i] != nullptr) && _m_equipCards[i]->isMarkable()) {
            equip_pos << i;
            count++;
            if (need_only && count > 1)
                return;
        }
    }
    if (count == 0)
        return;
    if (!items.isEmpty()) {
        CardItem *item = items.first();
        item->clickItem();
        selected = item;
        adjustCards();
    } else if (!equip_pos.isEmpty()) {
        int pos = equip_pos.first();
        _m_equipCards[pos]->mark(!_m_equipCards[pos]->isMarked());
        update();
    }
}

const Card *Dashboard::getSelected() const
{
    if (view_as_skill != nullptr)
        return pending_card;
    else if (selected != nullptr)
        return selected->getCard();
    else
        return nullptr;
}

void Dashboard::selectCard(CardItem *item, bool isSelected)
{
    bool oldState = item->isSelected();
    if (oldState == isSelected)
        return;
    m_mutex.lock();

    item->setSelected(isSelected);
    QPointF oldPos = item->homePos();
    QPointF newPos = oldPos;
    newPos.setY(newPos.y() + (isSelected ? 1 : -1) * S_PENDING_OFFSET_Y);
    item->setHomePos(newPos);
    selected = item;

    m_mutex.unlock();
}

void Dashboard::unselectAll(const CardItem *except)
{
    selected = nullptr;

    foreach (CardItem *card_item, m_handCards) {
        if (card_item != except)
            selectCard(card_item, false);
    }

    adjustCards(true);
    for (int i = 0; i < 5; i++) {
        if ((_m_equipCards[i] != nullptr) && _m_equipCards[i] != except)
            _m_equipCards[i]->mark(false);
    }
    if (view_as_skill != nullptr) {
        pendings.clear();
        updatePending();
    }
}

QRectF Dashboard::boundingRect() const
{
    return QRectF(0, 0, _m_width, _m_layout->m_normalHeight);
}

void Dashboard::setWidth(int width)
{
    prepareGeometryChange();
    adjustCards(true);
    _m_width = width;
    _updateFrames();
    _updateDeathIcon();
}

QSanSkillButton *Dashboard::addSkillButton(const QString &skillName, const bool &head)
{
    // if it's a equip skill, add it to equip bar
    _mutexEquipAnim.lock();

    for (int i = 0; i < 5; i++) {
        if (_m_equipCards[i] == nullptr)
            continue;
        const EquipCard *equip = qobject_cast<const EquipCard *>(_m_equipCards[i]->getCard()->getRealCard());
        Q_ASSERT(equip);
        // @todo: we must fix this in the server side - add a skill to the card itself instead
        // of getting it from the engine.
        const Skill *skill = Sanguosha->getSkill(equip);

        if (skill == nullptr)
            continue;
        if (skill->objectName() == skillName) {
            // If there is already a button there, then we haven't removed the last skill before attaching
            // a new one. The server must have sent the requests out of order. So crash.
            Q_ASSERT(_m_equipSkillBtns[i] == nullptr);
            _m_equipSkillBtns[i] = new QSanInvokeSkillButton();
            _m_equipSkillBtns[i]->setSkill(skill);
            connect(_m_equipSkillBtns[i], SIGNAL(clicked()), this, SLOT(_onEquipSelectChanged()));
            connect(_m_equipSkillBtns[i], SIGNAL(enable_changed()), this, SLOT(_onEquipSelectChanged()));
            QSanSkillButton *btn = _m_equipSkillBtns[i];

            _mutexEquipAnim.unlock();
            return btn;
        }
    }
    _mutexEquipAnim.unlock();
#ifndef QT_NO_DEBUG
    const Skill *skill = Sanguosha->getSkill(skillName);
    Q_ASSERT(skill && !skill->inherits("WeaponSkill") && !skill->inherits("ArmorSkill") && !skill->inherits("TreasureSkill"));
#endif
    if (_m_skillDock->getSkillButtonByName(skillName) != nullptr && head)
        return nullptr;
    if (_m_rightSkillDock->getSkillButtonByName(skillName) != nullptr && !head)
        return nullptr;

    QSanInvokeSkillDock *dock = head ? _m_skillDock : _m_rightSkillDock;

    //hegemony
    if (dock == _m_skillDock)
        updateHiddenMark();
    else
        updateRightHiddenMark(); //check it is right skilldock?

    return dock->addSkillButtonByName(skillName);
}

QSanSkillButton *Dashboard::removeSkillButton(const QString &skillName, bool head)
{
    QSanSkillButton *btn = nullptr;
    _mutexEquipAnim.lock();
    for (int i = 0; i < 5; i++) {
        if (_m_equipSkillBtns[i] == nullptr)
            continue;
        const Skill *skill = _m_equipSkillBtns[i]->getSkill();
        Q_ASSERT(skill != nullptr);
        if (skill->objectName() == skillName) {
            btn = _m_equipSkillBtns[i];
            _m_equipSkillBtns[i] = nullptr;
            continue;
        }
    }
    _mutexEquipAnim.unlock();
    if (btn == nullptr) {
        QSanInvokeSkillDock *dock = head ? _m_skillDock : _m_rightSkillDock;
        QSanSkillButton *temp = dock->getSkillButtonByName(skillName);
        if (temp != nullptr)
            btn = dock->removeSkillButtonByName(skillName);
    }
    return btn;
}

void Dashboard::highlightEquip(QString skillName, bool highlight)
{
    int i = 0;
    for (i = 0; i < 5; i++) {
        if (_m_equipCards[i] == nullptr)
            continue;
        if (_m_equipCards[i]->getCard()->objectName() == skillName)
            break;
    }
    if (i != 5)
        _setEquipBorderAnimation(i, highlight);
}

void Dashboard::_createExtraButtons()
{
    m_btnReverseSelection = new QSanButton("handcard", "reverse-selection", this);
    m_btnSortHandcard = new QSanButton("handcard", "sort", this);
    m_btnNoNullification = new QSanButton("handcard", "nullification", this);

    m_btnReverseSelection->hide();

    m_btnNoNullification->setStyle(QSanButton::S_STYLE_TOGGLE);
    // @todo: auto hide.
    m_btnReverseSelection->setPos(G_DASHBOARD_LAYOUT.m_rswidth, -m_btnReverseSelection->boundingRect().height());
    //m_btnReverseSelection->boundingRect().right() + G_DASHBOARD_LAYOUT.m_rswidth,G_DASHBOARD_LAYOUT.m_leftWidth
    //QRectF sortHandRect = this->boundingRect();
    //sortHandRect = this->mapRectToItem(this, sortHandRect);//_m_rightFrame
    m_btnSortHandcard->setPos(0, -m_btnReverseSelection->boundingRect().height());

    m_btnNoNullification->setPos(m_btnSortHandcard->boundingRect().width(), -m_btnReverseSelection->boundingRect().height());
    //m_btnNoNullification->setPos(m_btnReverseSelection->boundingRect().right() + m_btnSortHandcard->boundingRect().width() + G_DASHBOARD_LAYOUT.m_rswidth,
    //                             -m_btnReverseSelection->boundingRect().height());
    m_btnNoNullification->hide();
    connect(m_btnReverseSelection, SIGNAL(clicked()), this, SLOT(reverseSelection()));
    connect(m_btnSortHandcard, SIGNAL(clicked()), this, SLOT(sortCards()));
    connect(m_btnNoNullification, SIGNAL(clicked()), this, SLOT(cancelNullification()));

    //_m_carditem_context_menu = new QMenu(RoomSceneInstance->mainWindow());
    //QAction *reverseSelectionAction = _m_carditem_context_menu->addAction(tr("Reverse selection"));
    //connect(reverseSelectionAction, SIGNAL(triggered()), this, SLOT(reverseSelection()));
    //_m_carditem_context_menu->addMenu(_m_sort_menu);
}

void Dashboard::skillButtonActivated()
{
    QSanSkillButton *button = qobject_cast<QSanSkillButton *>(sender());
    QList<QSanInvokeSkillButton *> buttons = _m_skillDock->getAllSkillButtons() + _m_rightSkillDock->getAllSkillButtons();
    foreach (QSanSkillButton *btn, buttons) {
        if (button == btn)
            continue;

        if (btn->getViewAsSkill() != nullptr && btn->isDown())
            btn->setState(QSanButton::S_STATE_UP);
    }

    for (int i = 0; i < 5; i++) {
        if (button == _m_equipSkillBtns[i])
            continue;

        if (_m_equipSkillBtns[i] != nullptr)
            _m_equipSkillBtns[i]->setEnabled(false);
    }
}

void Dashboard::skillButtonDeactivated()
{
    QList<QSanInvokeSkillButton *> buttons = _m_skillDock->getAllSkillButtons() + _m_rightSkillDock->getAllSkillButtons();
    foreach (QSanSkillButton *btn, buttons) {
        if (btn->getViewAsSkill() != nullptr && btn->isDown())
            btn->setState(QSanButton::S_STATE_UP);
    }

    for (int i = 0; i < 5; i++) {
        if (_m_equipSkillBtns[i] != nullptr) {
            _m_equipSkillBtns[i]->setEnabled(true);
            if (_m_equipSkillBtns[i]->isDown())
                _m_equipSkillBtns[i]->click();
        }
    }
}

void Dashboard::selectAll()
{
    foreach (const QString &pile, Self->getPileNames()) {
        if (pile.startsWith("&") || pile == "wooden_ox")
            retractPileCards(pile);
    }
    retractSpecialCard();

    if (view_as_skill != nullptr) {
        unselectAll();
        foreach (CardItem *card_item, m_handCards) {
            selectCard(card_item, true);
            pendings << card_item;
        }
        updatePending();
    }
    adjustCards(true);
}

void Dashboard::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

void Dashboard::mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    PlayerCardContainer::mouseReleaseEvent(mouseEvent);

    CardItem *to_select = nullptr;
    int i = 0;
    for (i = 0; i < 5; i++) {
        if (_m_equipRegions[i]->isUnderMouse()) {
            to_select = _m_equipCards[i];
            break;
        }
    }
    if (to_select == nullptr)
        return;
    if (_m_equipSkillBtns[i] != nullptr && _m_equipSkillBtns[i]->isEnabled())
        _m_equipSkillBtns[i]->click();
    else if (to_select->isMarkable()) {
        // According to the game rule, you cannot select a weapon as a card when
        // you are invoking the skill of that equip. So something must be wrong.
        // Crash.
        Q_ASSERT(_m_equipSkillBtns[i] == nullptr || !_m_equipSkillBtns[i]->isDown());
        to_select->mark(!to_select->isMarked());
        update();
    }
}

void Dashboard::_onEquipSelectChanged()
{
    QSanSkillButton *btn = qobject_cast<QSanSkillButton *>(sender());
    if (btn != nullptr) {
        for (int i = 0; i < 5; i++) {
            if (_m_equipSkillBtns[i] == btn) {
                _setEquipBorderAnimation(i, btn->isDown());
                break;
            }
        }
    } else {
        CardItem *equip = qobject_cast<CardItem *>(sender());
        // Do not remove this assertion. If equip is NULL here, some other
        // sources that could select equip has not been considered and must
        // be implemented.
        Q_ASSERT(equip);
        for (int i = 0; i < 5; i++) {
            if (_m_equipCards[i] == equip) {
                _setEquipBorderAnimation(i, equip->isMarked());
                break;
            }
        }
    }
}

void Dashboard::_createEquipBorderAnimations()
{
    for (int i = 0; i < 5; i++) {
        _m_equipBorders[i] = new PixmapAnimation();
        _m_equipBorders[i]->setParentItem(_getEquipParent());
        _m_equipBorders[i]->setPath("image/system/emotion/equipborder/");

        if (!_m_equipBorders[i]->valid()) {
            delete _m_equipBorders[i];
            _m_equipBorders[i] = nullptr;
            continue;
        }

        _m_equipBorders[i]->setPos(_dlayout->m_equipBorderPos + _dlayout->m_equipSelectedOffset + _dlayout->m_equipAreas[i].topLeft());
        _m_equipBorders[i]->hide();
    }
}

void Dashboard::_setEquipBorderAnimation(int index, bool turnOn)
{
    _mutexEquipAnim.lock();
    if (_m_isEquipsAnimOn[index] == turnOn) {
        _mutexEquipAnim.unlock();
        return;
    }

    QPoint newPos;
    if (turnOn)
        newPos = _dlayout->m_equipSelectedOffset + _dlayout->m_equipAreas[index].topLeft();
    else
        newPos = _dlayout->m_equipAreas[index].topLeft();

    _m_equipAnim[index]->stop();
    _m_equipAnim[index]->clear();
    QPropertyAnimation *anim = new QPropertyAnimation(_m_equipRegions[index], "pos");
    anim->setEndValue(newPos);
    anim->setDuration(200);
    _m_equipAnim[index]->addAnimation(anim);
    anim = new QPropertyAnimation(_m_equipRegions[index], "opacity");
    anim->setEndValue(255);
    anim->setDuration(200);
    _m_equipAnim[index]->addAnimation(anim);
    _m_equipAnim[index]->start();

    Q_ASSERT(_m_equipBorders[index]);
    if (turnOn) {
        _m_equipBorders[index]->show();
        _m_equipBorders[index]->start();
    } else {
        _m_equipBorders[index]->hide();
        _m_equipBorders[index]->stop();
    }

    _m_isEquipsAnimOn[index] = turnOn;
    _mutexEquipAnim.unlock();
}

void Dashboard::adjustCards(bool playAnimation)
{
    _adjustCards();
    foreach (CardItem *card, m_handCards)
        card->goBack(playAnimation);
}

void Dashboard::_adjustCards()
{
    int maxCards = Config.MaxCards;

    int n = m_handCards.length();
    if (n == 0)
        return;

    if (maxCards >= n)
        maxCards = n;
    else if (maxCards <= (n - 1) / 2 + 1)
        maxCards = (n - 1) / 2 + 1;
    QList<CardItem *> row;
    QSanRoomSkin::DashboardLayout *layout = (QSanRoomSkin::DashboardLayout *)_m_layout;
    int leftWidth = layout->m_leftWidth;
    int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;
    int rwidth = (ServerInfo.Enable2ndGeneral) ? layout->m_rightWidthDouble : layout->m_rightWidth;
    int middleWidth = _m_width - layout->m_leftWidth - rwidth - getButtonWidgetWidth();
    QRect rowRect = QRect(leftWidth, layout->m_normalHeight - cardHeight - 3, middleWidth, cardHeight);
    for (int i = 0; i < maxCards; i++)
        row.push_back(m_handCards[i]);

    _m_highestZ = n;
    _disperseCards(row, rowRect, Qt::AlignLeft, true, true);

    row.clear();
    rowRect.translate(0, 1.5 * S_PENDING_OFFSET_Y);
    for (int i = maxCards; i < n; i++)
        row.push_back(m_handCards[i]);

    _m_highestZ = 0;
    _disperseCards(row, rowRect, Qt::AlignLeft, true, true);

    for (int i = 0; i < n; i++) {
        CardItem *card = m_handCards[i];
        if (card->isSelected()) {
            QPointF newPos = card->homePos();
            newPos.setY(newPos.y() + S_PENDING_OFFSET_Y);
            card->setHomePos(newPos);
        }
    }
}

int Dashboard::getMiddleWidth()
{
    int rwidth = (ServerInfo.Enable2ndGeneral) ? G_DASHBOARD_LAYOUT.m_rightWidthDouble : G_DASHBOARD_LAYOUT.m_rightWidth;
    return _m_width - G_DASHBOARD_LAYOUT.m_leftWidth - rwidth;
}

QList<CardItem *> Dashboard::cloneCardItems(QList<int> card_ids)
{
    QList<CardItem *> result;
    CardItem *card_item = nullptr;
    CardItem *new_card = nullptr;

    foreach (int card_id, card_ids) {
        card_item = CardItem::FindItem(m_handCards, card_id);
        new_card = _createCard(card_id);
        Q_ASSERT(card_item);
        if (card_item != nullptr) {
            new_card->setPos(card_item->pos());
            new_card->setHomePos(card_item->homePos());
        }
        result.append(new_card);
    }
    return result;
}

QList<CardItem *> Dashboard::removeHandCards(const QList<int> &card_ids)
{
    QList<CardItem *> result;
    CardItem *card_item = nullptr;
    foreach (int card_id, card_ids) {
        card_item = CardItem::FindItem(m_handCards, card_id);
        if (card_item == selected)
            selected = nullptr;
        Q_ASSERT(card_item);
        if (card_item != nullptr) {
            m_handCards.removeOne(card_item);
            card_item->hideFrame();
            card_item->disconnect(this);
            result.append(card_item);
        }
    }
    updateHandcardNum();
    return result;
}

QList<CardItem *> Dashboard::removeCardItems(const QList<int> &card_ids, Player::Place place)
{
    CardItem *card_item = nullptr;
    QList<CardItem *> result;
    bool pileNeedAdjust = false;
    if (place == Player::PlaceHand)
        result = removeHandCards(card_ids);
    else if (place == Player::PlaceEquip)
        result = removeEquips(card_ids);
    else if (place == Player::PlaceDelayedTrick)
        result = removeDelayedTricks(card_ids);
    else if (place == Player::PlaceSpecial) {
        foreach (int card_id, card_ids) {
            card_item = _createCard(card_id);
            card_item->setOpacity(0.0);
            result.push_back(card_item);

            foreach (const QList<int> &expanded, _m_pile_expanded) {
                if (expanded.contains(card_id)) {
                    QString key = _m_pile_expanded.key(expanded);
                    if (key.isEmpty())
                        continue;

                    _m_pile_expanded[key].removeOne(card_id);

                    CardItem *card_item = CardItem::FindItem(m_handCards, card_id);
                    if (card_item == selected)
                        selected = nullptr;
                    Q_ASSERT(card_item);
                    if (card_item != nullptr) {
                        m_handCards.removeOne(card_item);
                        card_item->disconnect(this);
                        delete card_item;
                        card_item = nullptr;
                    }
                    pileNeedAdjust = true;
                }
            }
        }
    } else
        Q_ASSERT(false);

    foreach (CardItem *card, result) {
        card->setAcceptedMouseButtons(nullptr);
    }

    Q_ASSERT(result.size() == card_ids.size());
    if (place == Player::PlaceHand)
        adjustCards();
    else if (result.size() > 1 || place == Player::PlaceSpecial) {
        QRect rect(0, 0, _dlayout->m_disperseWidth, 0);
        QPointF center(0, 0);
        if (place == Player::PlaceEquip || place == Player::PlaceDelayedTrick) {
            for (int i = 0; i < result.size(); i++)
                center += result[i]->pos();
            center = 1.0 / result.length() * center;
        } else if (place == Player::PlaceSpecial) {
            QPoint avatarArea_center = (ServerInfo.Enable2ndGeneral) ? _dlayout->m_avatarAreaDouble.center() : _dlayout->m_avatarArea.center();
            center = mapFromItem(_getAvatarParent(), avatarArea_center);
        } else
            Q_ASSERT(false);
        rect.moveCenter(center.toPoint());
        _disperseCards(result, rect, Qt::AlignCenter, false, false);

        if (place == Player::PlaceSpecial && pileNeedAdjust)
            adjustCards();
    }
    update();
    return result;
}

void Dashboard::updateAvatar()
{
    /*
        rewrite PlayerCardContainer::updateAvatar() ??
    */

    PlayerCardContainer::updateAvatar();
    if (_m_skillDock != nullptr)
        _m_skillDock->update();
    if (_m_rightSkillDock != nullptr)
        _m_rightSkillDock->update();
    _adjustComponentZValues();
}

void Dashboard::updateSmallAvatar()
{
    /*
    rewrite PlayerCardContainer::updateAvatar() ??
    */

    PlayerCardContainer::updateSmallAvatar();
    if (_m_skillDock != nullptr)
        _m_skillDock->update();
    if (_m_rightSkillDock != nullptr)
        _m_rightSkillDock->update();
    _adjustComponentZValues();
}

static bool CompareByNumber(const CardItem *a, const CardItem *b)
{
    return Card::CompareByNumber(a->getCard(), b->getCard());
}

static bool CompareBySuit(const CardItem *a, const CardItem *b)
{
    return Card::CompareBySuit(a->getCard(), b->getCard());
}

static bool CompareByType(const CardItem *a, const CardItem *b)
{
    return Card::CompareByType(a->getCard(), b->getCard());
}

void Dashboard::sortCards()
{
    if (m_handCards.length() == 0)
        return;

    QMenu *menu = _m_sort_menu;
    menu->clear();
    menu->setTitle(tr("Sort handcards"));

    QAction *action1 = menu->addAction(tr("Sort by type"));
    action1->setData((int)ByType);

    QAction *action2 = menu->addAction(tr("Sort by suit"));
    action2->setData((int)BySuit);

    QAction *action3 = menu->addAction(tr("Sort by number"));
    action3->setData((int)ByNumber);

    connect(action1, SIGNAL(triggered()), this, SLOT(beginSorting()));
    connect(action2, SIGNAL(triggered()), this, SLOT(beginSorting()));
    connect(action3, SIGNAL(triggered()), this, SLOT(beginSorting()));

    QPointF posf = QCursor::pos();
    menu->popup(QPoint(posf.x(), posf.y()));
}

void Dashboard::beginSorting()
{
    QAction *action = qobject_cast<QAction *>(sender());
    SortType type = ByType;
    if (action != nullptr)
        type = (SortType)(action->data().toInt());

    switch (type) {
    case ByType:
        std::sort(m_handCards.begin(), m_handCards.end(), CompareByType);
        break;
    case BySuit:
        std::sort(m_handCards.begin(), m_handCards.end(), CompareBySuit);
        break;
    case ByNumber:
        std::sort(m_handCards.begin(), m_handCards.end(), CompareByNumber);
        break;
    default:
        Q_ASSERT(false);
    }

    adjustCards();
}

void Dashboard::reverseSelection()
{
    if (view_as_skill == nullptr)
        return;

    QList<CardItem *> selected_items;
    foreach (CardItem *item, m_handCards)
        if (item->isSelected()) {
            item->clickItem();
            selected_items << item;
        }
    foreach (CardItem *item, m_handCards)
        if (item->isEnabled() && !selected_items.contains(item))
            item->clickItem();
    adjustCards();
}

void Dashboard::cancelNullification()
{
    ClientInstance->m_noNullificationThisTime = !ClientInstance->m_noNullificationThisTime;
    if (Sanguosha->getCurrentCardUseReason() == CardUseStruct::CARD_USE_REASON_RESPONSE_USE && Sanguosha->getCurrentCardUsePattern() == "nullification"
        && RoomSceneInstance->isCancelButtonEnabled()) {
        RoomSceneInstance->doCancelButton();
    }
}

void Dashboard::controlNullificationButton(bool show)
{
    if (ClientInstance->getReplayer() != nullptr)
        return;
    m_btnNoNullification->setState(QSanButton::S_STATE_UP);
    m_btnNoNullification->setVisible(show);
}

void Dashboard::disableAllCards()
{
    m_mutexEnableCards.lock();
    foreach (CardItem *card_item, m_handCards)
        card_item->setEnabled(false);
    m_mutexEnableCards.unlock();
}

void Dashboard::enableCards()
{
    m_mutexEnableCards.lock();
    foreach (const QString &pile, Self->getHandPileList(false))
        expandPileCards(pile);
    expandSpecialCard();

    foreach (CardItem *card_item, m_handCards)
        card_item->setEnabled(card_item->getCard()->isAvailable(Self));

    m_mutexEnableCards.unlock();
}

void Dashboard::enableAllCards()
{
    m_mutexEnableCards.lock();
    foreach (CardItem *card_item, m_handCards)
        card_item->setEnabled(true);
    m_mutexEnableCards.unlock();
}

void Dashboard::startPending(const ViewAsSkill *skill)
{
    m_mutexEnableCards.lock();
    view_as_skill = skill;
    pendings.clear();
    unselectAll();

    bool expand = ((skill != nullptr) && skill->isResponseOrUse());
    if (!expand && (skill != nullptr) && skill->inherits("ResponseSkill")) {
        const ResponseSkill *resp_skill = qobject_cast<const ResponseSkill *>(skill);
        if ((resp_skill != nullptr) && (resp_skill->getRequest() == Card::MethodResponse || resp_skill->getRequest() == Card::MethodUse))
            expand = true;
    }

    if (Self->hasFlag("Global_expandpileFailed"))
        expand = true;

    foreach (const QString &pileName, _m_pile_expanded.keys()) {
        if (!(pileName.startsWith("&") || pileName == "wooden_ox"))
            retractPileCards(pileName);
    }
    retractSpecialCard();

    if (expand) {
        foreach (const QString &pile, Self->getHandPileList(false))
            expandPileCards(pile);
        if (!((skill != nullptr) && skill->isResponseOrUse()))
            expandSpecialCard();
    } else {
        foreach (const QString &pile, Self->getPileNames()) {
            if (pile.startsWith("&") || pile == "wooden_ox")
                retractPileCards(pile);
        }
        retractSpecialCard();
        if ((skill != nullptr) && !skill->getExpandPile().isEmpty()) {
            foreach (const QString &pile_name, skill->getExpandPile().split(","))
                expandPileCards(pile_name);
        }
    }

    for (int i = 0; i < 5; i++) {
        if (_m_equipCards[i] != nullptr)
            connect(_m_equipCards[i], SIGNAL(mark_changed()), this, SLOT(onMarkChanged()));
    }

    updatePending();

    m_mutexEnableCards.unlock();
}

void Dashboard::stopPending()
{
    m_mutexEnableCards.lock();
    if (view_as_skill != nullptr) {
        if (!view_as_skill->getExpandPile().isEmpty()) {
            retractPileCards(view_as_skill->getExpandPile());
        }
    }

    view_as_skill = nullptr;
    pending_card = nullptr;
    foreach (const QString &pile, Self->getPileNames()) {
        if (pile.startsWith("&") || pile == "wooden_ox")
            retractPileCards(pile);
    }
    retractSpecialCard();
    emit card_selected(nullptr);

    foreach (CardItem *item, m_handCards) {
        item->setEnabled(false);
        animations->effectOut(item);
    }

    for (int i = 0; i < 5; i++) {
        CardItem *equip = _m_equipCards[i];
        if (equip != nullptr) {
            equip->mark(false);
            equip->setMarkable(false);
            _m_equipRegions[i]->setOpacity(1.0);
            equip->setEnabled(false);
            disconnect(equip, SIGNAL(mark_changed()));
        }
    }
    pendings.clear();
    adjustCards(true);
    m_mutexEnableCards.unlock();
}

void Dashboard::expandPileCards(const QString &pile_name)
{
    if (_m_pile_expanded.contains(pile_name))
        return;

    QList<int> pile;
    if (pile_name.startsWith("%") || pile_name.startsWith("+")) {
        QString new_name = pile_name.mid(1);
        if (pile_name.startsWith("+"))
            pile += Self->getPile(new_name);
        foreach (const Player *p, Self->getAliveSiblings())
            pile += p->getPile(new_name);
    } else if (pile_name == "#xiuye_temp") {
        foreach (const Card *c, ClientInstance->discarded_list) {
            if (c->getSuit() == Card::Club && (c->isNDTrick() || c->getTypeId() == Card::TypeBasic))
                pile << c->getEffectiveId();
        }
    } else if (pile_name == "#xiufu_temp") {
        foreach (const Card *c, ClientInstance->discarded_list) {
            if (c->getTypeId() == Card::TypeEquip)
                pile << c->getEffectiveId();
        }
    } else if (pile_name == "#shijie") {
        QString dyingPlayerName = Self->property("currentdying").toString();
        const Player *p = ClientInstance->findChild<const Player *>(dyingPlayerName);
        if (p != nullptr && p != Self) {
            foreach (const Card *c, p->getEquips())
                pile << c->getEffectiveId();
        }
    } else if (pile_name == "#judging_area") {
        pile = Self->getJudgingAreaID();
    } else if (pile_name.startsWith("*")) {
        pile = StringList2IntList(Self->property(pile_name.mid(1).toUtf8().constData()).toString().split("+"));
    } else {
        pile = Self->getPile(pile_name);
    }

    if (pile.isEmpty())
        return;

    QList<CardItem *> card_items = _createCards(pile);
    if (pile_name == "zhenli")
        std::sort(card_items.begin(), card_items.end(), CompareByNumber);
    foreach (CardItem *card_item, card_items) {
        card_item->setPos(mapFromScene(card_item->scenePos()));
        card_item->setParentItem(this);
    }

    foreach (CardItem *card_item, card_items) {
        QString pile_string = pile_name;
        if (pile_name == "%shown_card") {
            foreach (const Player *p, Self->getAliveSiblings()) {
                if (p->getPile("shown_card").contains(card_item->getId())) {
                    pile_string = ClientInstance->getPlayerName(p->objectName());
                    break;
                }
            }
        } else if (pile_name == "*mengxiang_temp") {
            QString target_name;
            foreach (const Player *p, Self->getAliveSiblings()) {
                if (p->hasFlag("mengxiangtarget")) {
                    target_name = p->objectName();
                    break;
                }
            }
            if (target_name.isEmpty())
                target_name = Self->objectName();
            pile_string = ClientInstance->getPlayerName(target_name);
        } else if (pile_name == "*chunhua") {
            bool isUse = false;
            if (card_items.indexOf(card_item) == 0)
                isUse = Self->property("chunhua_firstCardIsUsing").toString() == "1";
            if (isUse)
                pile_string = "use";
            else
                pile_string = "%shown_card";
        } else if ((pile_name.startsWith("%") || pile_name.startsWith("+") || pile_name.startsWith("*")) && pile_name != "%shown_card") {
            pile_string = pile_name.mid(1);
        }
        _addHandCard(card_item, true, Sanguosha->translate(pile_string));
    }

    adjustCards();
    _playMoveCardsAnimation(card_items, false);
    foreach (CardItem *card_item, card_items)
        card_item->setAcceptedMouseButtons(Qt::LeftButton);
    update();
    _m_pile_expanded[pile_name] = pile;
}

void Dashboard::expandSpecialCard()
{
    //delete first;
    retractSpecialCard();

    if (!m_player->hasSkill("chaoren"))
        return;
    // then expand
    bool ok = false;
    int id = Self->property("chaoren").toInt(&ok);
    if (ok && id > -1) {
        _m_id_expanded << id;

        QList<CardItem *> card_items;
        CardItem *card_item = _createCard(id);
        card_items << card_item;
        card_item->setPos(mapFromScene(card_item->scenePos()));
        card_item->setParentItem(this);

        _addHandCard(card_item, true, Sanguosha->translate("chaoren"));

        adjustCards();
        _playMoveCardsAnimation(card_items, false);
        card_item->setAcceptedMouseButtons(Qt::LeftButton);
        update();
    }
}

void Dashboard::retractPileCards(const QString &pile_name)
{
    if (!_m_pile_expanded.contains(pile_name))
        return;

    QList<int> pile = _m_pile_expanded.value(pile_name);
    _m_pile_expanded.remove(pile_name);

    if (pile.isEmpty())
        return;
    CardItem *card_item = nullptr;

    foreach (int card_id, pile) {
        card_item = CardItem::FindItem(m_handCards, card_id);
        if (card_item == selected)
            selected = nullptr;
        Q_ASSERT(card_item);
        if (card_item != nullptr) {
            m_handCards.removeOne(card_item);
            card_item->disconnect(this);
            delete card_item;
            card_item = nullptr;
        }
    }
    adjustCards();
    update();
}

void Dashboard::retractSpecialCard()
{
    CardItem *card_item = nullptr;
    foreach (int card_id, _m_id_expanded) {
        card_item = CardItem::FindItem(m_handCards, card_id);
        if (card_item == selected)
            selected = nullptr;
        Q_ASSERT(card_item);
        if (card_item != nullptr) {
            m_handCards.removeOne(card_item);
            card_item->disconnect(this);
            delete card_item;
            card_item = nullptr;
        }
    }
    adjustCards();
    update();
    _m_id_expanded = QList<int>();
}

void Dashboard::updateChaoren()
{
    expandSpecialCard();
}
void Dashboard::updateHandPile()
{
    WrappedCard *t = Self->getTreasure();
    if (t != nullptr) {
        if (Self->isBrokenEquip(t->getEffectiveId(), true))
            retractPileCards("wooden_ox");
    }
}

void Dashboard::selectLingshou()
{
    foreach (const QString &pile, Self->getPileNames()) {
        if (pile.startsWith("&") || pile == "wooden_ox")
            retractPileCards(pile);
    }
    retractSpecialCard();

    if (view_as_skill != nullptr) {
        unselectAll();
        bool ok = false;
        int selectedId = Self->property("lingshouSelected").toString().toInt(&ok);
        if (!ok)
            return;

        CardItem *handcardItem = nullptr;
        foreach (CardItem *card_item, m_handCards) {
            if (selectedId == card_item->getId()) {
                selectCard(card_item, true);
                pendings << card_item;
                handcardItem = card_item;
                break;
            }
        }
        if (handcardItem != nullptr) {
            for (CardItem *card_item : _m_equipCards) {
                if (card_item != nullptr) {
                    if (handcardItem->getCard()->getSuit() == card_item->getCard()->getSuit()) {
                        // selectCard(card_item, true);
                        card_item->mark();
                    }
                }
            }
        }
        updatePending();
    }
    adjustCards(true);
}

void Dashboard::selectWeiyi()
{
    foreach (const QString &pile, Self->getPileNames()) {
        if (pile.startsWith("&") || pile == "wooden_ox")
            retractPileCards(pile);
    }
    retractSpecialCard();

    if (view_as_skill != nullptr) {
        unselectAll();
        bool ok = false;
        int selectedId = Self->property("weiyiSelected").toString().toInt(&ok);
        if (!ok)
            return;

        foreach (CardItem *card_item, m_handCards) {
            if (selectedId == card_item->getId()) {
                selectCard(card_item, true);
                pendings << card_item;
                break;
            }
        }
        updatePending();
    }
    adjustCards(true);
}

void Dashboard::updateShown()
{
    updatePending();
    adjustCards();
    update();
}

void Dashboard::onCardItemClicked()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item == nullptr)
        return;

    if (view_as_skill != nullptr) {
        if (card_item->isSelected()) {
            selectCard(card_item, false);
            pendings.removeOne(card_item);
        } else {
            if (view_as_skill->inherits("OneCardViewAsSkill"))
                unselectAll();
            selectCard(card_item, true);
            pendings << card_item;
        }

        updatePending();
    } else {
        if (card_item->isSelected()) {
            unselectAll();
            emit card_selected(nullptr);
        } else {
            unselectAll();
            selectCard(card_item, true);
            selected = card_item;

            emit card_selected(selected->getCard());
        }
    }
}

//#include "wind.h"
void Dashboard::updatePending()
{
    foreach (CardItem *item, m_handCards) {
        if (item->getFootnote() == Sanguosha->translate("shown_card"))
            item->hideFootnote();
        if (Self->isShownHandcard(item->getCard()->getEffectiveId())) {
            item->setFootnote(Sanguosha->translate("shown_card"));
            item->showFootnote();
        }
    }

    if (view_as_skill == nullptr)
        return;
    QList<const Card *> cards;
    foreach (CardItem *item, pendings)
        cards.append(item->getCard());

    QList<const Card *> pended;
    if (!view_as_skill->inherits("OneCardViewAsSkill"))
        pended = cards;
    foreach (CardItem *item, m_handCards) {
        if (!item->isSelected() || pendings.isEmpty())
            item->setEnabled(view_as_skill->viewFilter(pended, item->getCard()));
        if (!item->isEnabled())
            animations->effectOut(item);
    }

    for (int i = 0; i < 5; i++) {
        CardItem *equip = _m_equipCards[i];
        if ((equip != nullptr) && !equip->isMarked())
            equip->setMarkable(view_as_skill->viewFilter(pended, equip->getCard()));
        if (equip != nullptr) {
            if (!equip->isMarkable() && ((_m_equipSkillBtns[i] == nullptr) || !_m_equipSkillBtns[i]->isEnabled()))
                _m_equipRegions[i]->setOpacity(0.7);
            else
                _m_equipRegions[i]->setOpacity(1.0);
        }
    }

    const Card *new_pending_card = view_as_skill->viewAs(cards);
    if (pending_card != new_pending_card) {
        if ((pending_card != nullptr) && (pending_card->parent() == nullptr) && pending_card->isVirtualCard()) {
            delete pending_card;
            pending_card = nullptr;
        }
        pending_card = new_pending_card;
        emit card_selected(pending_card);
    }
}

void Dashboard::onCardItemDoubleClicked()
{
    if (!Config.EnableDoubleClick)
        return;
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item != nullptr) {
        if (view_as_skill == nullptr)
            selected = card_item;
        animations->effectOut(card_item);
        emit card_to_use();
    }
}

void Dashboard::onCardItemThrown()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item != nullptr) {
        if (view_as_skill == nullptr)
            selected = card_item;
        emit card_to_use();
    }
}

void Dashboard::onCardItemHover()
{
    //QGraphicsItem *card_item = qobject_cast<QGraphicsItem *>(sender());
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item == nullptr)
        return;

    animations->emphasize(card_item);

    /*_adjustCards();
    foreach(CardItem *card, m_handCards)
        card->goBack(true, true);
    */
}

void Dashboard::onCardItemLeaveHover()
{
    //QGraphicsItem *card_item = qobject_cast<QGraphicsItem *>(sender());
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item == nullptr)
        return;

    animations->effectOut(card_item);

    /*_adjustCards();
    foreach(CardItem *card, m_handCards)
        card->goBack(true, true);
    */
}

void Dashboard::bringSenderToTop()
{
    CardItem *item = qobject_cast<CardItem *>(sender());

    Q_ASSERT(item);
    item->setData(CARDITEM_Z_DATA_KEY, item->zValue());
    item->setZValue(1000);
}

void Dashboard::resetSenderZValue()
{
    CardItem *item = qobject_cast<CardItem *>(sender());

    Q_ASSERT(item);
    const int z = item->data(CARDITEM_Z_DATA_KEY).toInt();
    item->setZValue(z);
}

void Dashboard::onMarkChanged()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());

    Q_ASSERT(card_item->isEquipped());

    if (card_item != nullptr) {
        if (card_item->isMarked()) {
            if (!pendings.contains(card_item)) {
                if ((view_as_skill != nullptr) && view_as_skill->inherits("OneCardViewAsSkill"))
                    unselectAll(card_item);
                pendings.append(card_item);
            }
        } else
            pendings.removeOne(card_item);

        updatePending();
    }
}

const ViewAsSkill *Dashboard::currentSkill() const
{
    return view_as_skill;
}

const Card *Dashboard::pendingCard() const
{
    return pending_card;
}

QPointF Dashboard::getHeroSkinContainerPosition() const
{
    QRectF avatarParentRect = _m_rightFrame->sceneBoundingRect();
    /*QRectF heroSkinContainerRect = (m_primaryHeroSkinContainer != NULL)
        ? m_primaryHeroSkinContainer->boundingRect()
        : m_secondaryHeroSkinContainer->boundingRect();;*/
    QRectF heroSkinContainerRect = m_primaryHeroSkinContainer->boundingRect();
    return QPointF(avatarParentRect.left() - heroSkinContainerRect.width() - 120, avatarParentRect.bottom() - heroSkinContainerRect.height() - 5);
}

bool Dashboard::isItemUnderMouse(QGraphicsItem *item) const
{
    return (item->isUnderMouse() && !_m_skillDock->isUnderMouse() && !_m_rightSkillDock->isUnderMouse())
        || ((_m_skillDock->isUnderMouse() || _m_rightSkillDock->isUnderMouse()) && _m_screenNameItem->isVisible());
}

void Dashboard::onAvatarHoverEnter()
{
    //hideControlButtons();
    _m_screenNameItem->show();

    PlayerCardContainer::onAvatarHoverEnter();
}

void Dashboard::onAnimationFinished()
{
    //while carditem went into handcard,   setAcceptedMouseButtons
    m_mutexCardItemsAnimationFinished.lock();

    foreach (CardItem *cardItem, _m_cardItemsAnimationFinished) {
        if (nullptr != cardItem) {
            cardItem->setAcceptedMouseButtons(Qt::LeftButton);
        }
    }
    _m_cardItemsAnimationFinished.clear();

    m_mutexCardItemsAnimationFinished.unlock();

    GenericCardContainer::onAnimationFinished();
}

/*void Dashboard::onCardItemContextMenu()
{
if (NULL != _m_carditem_context_menu) {
_m_carditem_context_menu->popup(QCursor::pos());
}
}*/

void Dashboard::_initializeRemovedEffect()
{
    _removedEffect = new QPropertyAnimation(this, "opacity", this);
    _removedEffect->setDuration(2000);
    _removedEffect->setEasingCurve(QEasingCurve::OutInBounce);
    _removedEffect->setEndValue(0.6);
    _removedEffect->setStartValue(1.0);
}

void Dashboard::showSeat()
{
    const QRect region = (ServerInfo.Enable2ndGeneral) ? G_DASHBOARD_LAYOUT.m_seatIconRegionDouble : G_DASHBOARD_LAYOUT.m_seatIconRegion;
    PixmapAnimation *pma = PixmapAnimation::GetPixmapAnimation(_m_rightFrame, "seat");
    if (pma != nullptr) {
        pma->setTransform(QTransform::fromTranslate(-pma->boundingRect().width() / 2, -pma->boundingRect().height() / 2));
        pma->setPos(region.x() + region.width() / 2, region.y() + region.height() / 2);
    }
    _paintPixmap(_m_seatItem, region, _getPixmap(QSanRoomSkin::S_SKIN_KEY_SEAT_NUMBER, QString::number(m_player->getSeat())), _m_rightFrame);
    //save the seat number for later use
    //m_player->setProperty("UI_Seat", m_player->getSeat()); //use InitalSeat instead
    _m_seatItem->setZValue(1.1);
}

//only for hegemony??
/*void Dashboard::_createRoleComboBox()
{
    _m_roleComboBox = new RoleComboBox(rightFrame, true);
}*/

// for battle array
/*void Dashboard::repaintAll()
{
    PlayerCardContainer::repaintAll();
    if (NULL != m_changeHeadHeroSkinButton) {
        m_changeHeadHeroSkinButton->setPos(layout->m_changeHeadHeroSkinButtonPos);
    }
    if (NULL != m_changeDeputyHeroSkinButton) {
        m_changeDeputyHeroSkinButton->setPos(layout->m_changeDeputyHeroSkinButtonPos);
    }

    QStringList kingdoms = Sanguosha->getKingdoms();
    kingdoms.removeAll("god");
    foreach(const QString &kingdom, kingdoms) {
        _m_frameBorders[kingdom]->setSize(QSize(_dlayout->m_avatarArea.width() * 2 * 1.1, _dlayout->m_normalHeight * 1.2));
        _m_frameBorders[kingdom]->setPos(-_dlayout->m_avatarArea.width() * 0.1, -_dlayout->m_normalHeight * 0.1);
        double scale = G_ROOM_LAYOUT.scale;
        QPixmap pix;
        pix.load("image/system/roles/careerist.png");
        int w = pix.width() * scale;
        int h = pix.height() * scale;
        _m_roleBorders[kingdom]->setPos(G_DASHBOARD_LAYOUT.m_roleComboBoxPos
            - QPoint((_m_roleBorders[kingdom]->boundingRect().width() - w) / 2, (_m_roleBorders[kingdom]->boundingRect().height() - h) / 2));
    }
}*/

void Dashboard::updateHiddenMark()
{
    if (!isHegemonyGameMode(ServerInfo.GameMode))
        return;
    if ((m_player != nullptr) && RoomSceneInstance->game_started && !m_player->hasShownGeneral()) {
        leftHiddenMark->setVisible(m_player->isHidden(true));
    }

    else
        leftHiddenMark->setVisible(false);
}

void Dashboard::updateRightHiddenMark()
{
    if (!isHegemonyGameMode(ServerInfo.GameMode))
        return;
    if (rightHiddenMark == nullptr)
        return;
    if ((m_player != nullptr) && RoomSceneInstance->game_started && !m_player->hasShownGeneral2())
        rightHiddenMark->setVisible(m_player->isHidden(false));
    else
        rightHiddenMark->setVisible(false);
}

void Dashboard::setPlayer(ClientPlayer *player)
{
    PlayerCardContainer::setPlayer(player);
    connect(player, &ClientPlayer::head_state_changed, this, &Dashboard::onHeadStateChanged);
    connect(player, &ClientPlayer::deputy_state_changed, this, &Dashboard::onDeputyStateChanged);
}

void Dashboard::onHeadStateChanged()
{
    if (!isHegemonyGameMode(ServerInfo.GameMode))
        return;
    if ((m_player != nullptr) && RoomSceneInstance->game_started && !m_player->hasShownGeneral())
        _m_shadow_layer1->setBrush(G_DASHBOARD_LAYOUT.m_generalShadowColor);
    else
        _m_shadow_layer1->setBrush(Qt::NoBrush);
    updateHiddenMark();
}

void Dashboard::onDeputyStateChanged()
{
    if (!isHegemonyGameMode(ServerInfo.GameMode))
        return;
    if ((m_player != nullptr) && RoomSceneInstance->game_started && !m_player->hasShownGeneral2())
        _m_shadow_layer2->setBrush(G_DASHBOARD_LAYOUT.m_generalShadowColor);
    else
        _m_shadow_layer2->setBrush(Qt::NoBrush);
    updateRightHiddenMark();
}

void Dashboard::refresh()
{
    PlayerCardContainer::refresh();
    if (!isHegemonyGameMode(ServerInfo.GameMode))
        return;
    if ((m_player == nullptr) || (m_player->getGeneral() == nullptr) || !m_player->isAlive()) {
        _m_shadow_layer1->setBrush(Qt::NoBrush);

        leftHiddenMark->setVisible(false);
        if (ServerInfo.Enable2ndGeneral) {
            _m_shadow_layer2->setBrush(Qt::NoBrush);
            rightHiddenMark->setVisible(false);
        }

    } else if (m_player != nullptr) {
        _m_shadow_layer1->setBrush(m_player->hasShownGeneral() ? Qt::transparent : G_DASHBOARD_LAYOUT.m_generalShadowColor);

        leftHiddenMark->setVisible(m_player->isHidden(true));
        if (ServerInfo.Enable2ndGeneral) {
            _m_shadow_layer2->setBrush(m_player->hasShownGeneral2() ? Qt::transparent : G_DASHBOARD_LAYOUT.m_generalShadowColor);
            rightHiddenMark->setVisible(m_player->isHidden(false));
        }
    }
}

void Dashboard::_createBattleArrayAnimations()
{
    QStringList kingdoms = Sanguosha->getHegemonyKingdoms();
    kingdoms.removeAll("god");
    foreach (const QString &kingdom, kingdoms) {
        _m_frameBorders[kingdom] = new PixmapAnimation();
        _m_frameBorders[kingdom]->setZValue(30000);
        _m_roleBorders[kingdom] = new PixmapAnimation();
        _m_roleBorders[kingdom]->setZValue(30000);
        _m_frameBorders[kingdom]->setParentItem(_getFocusFrameParent());
        _m_roleBorders[kingdom]->setParentItem(_m_rightFrame);
        _m_frameBorders[kingdom]->setSize(QSize(_dlayout->m_avatarArea.width() * 2 * 1.1, _dlayout->m_normalHeight * 1.2));
        _m_frameBorders[kingdom]->setPath(QString("image/kingdom/battlearray/big/%1/").arg(kingdom));
        _m_roleBorders[kingdom]->setPath(QString("image/kingdom/battlearray/roles/%1/").arg(kingdom));
        _m_frameBorders[kingdom]->setPlayTime(2000);
        _m_roleBorders[kingdom]->setPlayTime(2000);
        if (!_m_frameBorders[kingdom]->valid()) {
            delete _m_frameBorders[kingdom];
            delete _m_roleBorders[kingdom];
            _m_frameBorders[kingdom] = NULL;
            _m_roleBorders[kingdom] = NULL;
            continue;
        }
        _m_frameBorders[kingdom]->setPos(-_dlayout->m_avatarArea.width() * 0.1, -_dlayout->m_normalHeight * 0.1);

        double scale = G_ROOM_LAYOUT.scale;
        QPixmap pix;
        pix.load("image/system/roles/careerist.png");
        int w = pix.width() * scale;
        //int h = pix.height() * scale;
        _m_roleBorders[kingdom]->setPos(G_DASHBOARD_LAYOUT.m_roleComboBoxPos
                                        - QPoint((_m_roleBorders[kingdom]->boundingRect().width() - w) / 2, (_m_roleBorders[kingdom]->boundingRect().height()) / 2));
        _m_frameBorders[kingdom]->setHideonStop(true);
        _m_roleBorders[kingdom]->setHideonStop(true);
        _m_frameBorders[kingdom]->hide();
        _m_roleBorders[kingdom]->hide();
    }
}

void Dashboard::playBattleArrayAnimations()
{
    QString kingdom = getPlayer()->getKingdom();
    _m_frameBorders[kingdom]->show();
    _m_frameBorders[kingdom]->start(true, 30);
    _m_roleBorders[kingdom]->preStart();
}
