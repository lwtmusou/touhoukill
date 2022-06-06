#include "roomscene.h"
#include "CardFace.h"
#include "SkinBank.h"
#include "audio.h"
#include "bubblechatbox.h"
#include "button.h"
#include "cardcontainer.h"
#include "carditem.h"
#include "cardoverview.h"
#include "choosegeneralbox.h"
#include "choosegeneraldialog.h"
#include "chooseoptionsbox.h"
#include "choosetriggerorderbox.h"
#include "distanceviewdialog.h"
#include "engine.h"
#include "general.h"
#include "generaloverview.h"
#include "indicatoritem.h"
#include "legacyjson.h"
#include "legacyutil.h"
#include "lightboxanimation.h"
#include "mode.h"
#include "pixmapanimation.h"
#include "playercardbox.h"
#include "qsanbutton.h"
#include "recorder.h"
#include "serverinfowidget.h"
#include "settings.h"
#include "sgswindow.h"
#include "uiUtils.h"
#include "util.h"

#include <QApplication>
#include <QChar>
#include <QCheckBox>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsSceneMouseEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QRadioButton>
#include <QRandomGenerator>
#include <QSequentialAnimationGroup>
#include <QStandardPaths>
#include <QTimer>
#include <QTransform>
#include <QtMath>
#include <chrono>
#include <cmath>

using namespace std::chrono_literals;

using namespace QSanProtocol;

RoomScene *RoomSceneInstance;

void RoomScene::resetPiles()
{
    // @todo: fix this...
}

RoomScene::RoomScene(QMainWindow *main_window, Client *client)
    : m_skillButtonSank(false)
    , game_started(false)
    , main_window(main_window)
    , client(client)
{
    LordBGMConvertList = Sanguosha->configuration(QStringLiteral("bgm_convert_pairs")).toStringList();
    LordBackdropConvertList = Sanguosha->configuration(QStringLiteral("backdrop_convert_pairs")).toStringList();

    m_choiceDialog = nullptr;
    RoomSceneInstance = this;
    _m_last_front_item = nullptr;
    _m_last_front_ZValue = 0;
    int player_count = Sanguosha->getPlayerCount(ClientInstance->serverInfo()->GameModeStr);
    _m_roomSkin = &(QSanSkinFactory::getInstance().getCurrentSkinScheme().getRoomSkin());
    _m_roomLayout = &(G_ROOM_SKIN.getRoomLayout());
    _m_photoLayout = &(G_ROOM_SKIN.getPhotoLayout());
    _m_commonLayout = &(G_ROOM_SKIN.getCommonLayout());

    // create photos
    for (int i = 0; i < player_count - 1; i++) {
        Photo *photo = new Photo;
        photos << photo;
        addItem(photo);
        photo->setZValue(-0.5);
    }

    // create table pile
    m_tablePile = new TablePile;
    addItem(m_tablePile);
    connect(ClientInstance, SIGNAL(card_used()), m_tablePile, SLOT(clear()));

    // create dashboard
    dashboard = new Dashboard(createDashboardButtons());
    dashboard->setObjectName(QStringLiteral("dashboard"));
    dashboard->setZValue(0.8);
    addItem(dashboard);

    dashboard->setPlayer(Self);
    connect(Self, SIGNAL(general_changed()), dashboard, SLOT(updateAvatar()));
    connect(Self, SIGNAL(general2_changed()), dashboard, SLOT(updateSmallAvatar()));
    connect(dashboard, &Dashboard::card_selected, this, &RoomScene::enableTargets);
    connect(dashboard, &Dashboard::card_to_use, this, &RoomScene::doOkButton);
    // connect(Self, &Player::pile_changed, dashboard, &PlayerCardContainer::updatePile);

    // add role ComboBox
    // if (isHegemonyGameMode(ClientInstance->serverInfo()->GameMode))
    // connect(Self, &Player::kingdom_changed, dashboard, &Dashboard::updateKingdom);
    // else
    // connect(Self, &Player::role_changed, dashboard, &PlayerCardContainer::updateRole);

    m_replayControl = nullptr;
    if (ClientInstance->getReplayer() != nullptr) {
        dashboard->hideControlButtons();
        createReplayControlBar();
    }

    response_skill = new ResponseSkill;
    showorpindian_skill = new ShowOrPindianSkill;
    discard_skill = new DiscardSkill;
    yiji_skill = new YijiViewAsSkill;
    choose_skill = new ChoosePlayerSkill;

    miscellaneous_menu = new QMenu(main_window);

    change_general_menu = new QMenu(main_window);
    QAction *action = change_general_menu->addAction(tr("Change general ..."));
    FreeChooseDialog *general_changer = new FreeChooseDialog(main_window);
    connect(action, &QAction::triggered, general_changer, &QDialog::exec);
    connect(general_changer, &FreeChooseDialog::general_chosen, this, &RoomScene::changeGeneral);
    to_change = nullptr;

    // do signal-slot connections
    connect(ClientInstance, &Client::player_added, this, &RoomScene::addPlayer);
    connect(ClientInstance, &Client::player_removed, this, &RoomScene::removePlayer);
    connect(ClientInstance, &Client::generals_got, this, &RoomScene::chooseGeneral);
    connect(ClientInstance, &Client::generals_viewed, this, &RoomScene::viewGenerals);
    connect(ClientInstance, &Client::suits_got, this, &RoomScene::chooseSuit);
    connect(ClientInstance, &Client::options_got, this, &RoomScene::chooseOption);
    connect(ClientInstance, SIGNAL(cards_got(const Player *, QString, QString, bool, QSanguosha::HandlingMethod, QList<int>, bool)), this,
            SLOT(chooseCard(const Player *, QString, QString, bool, QSanguosha::HandlingMethod, QList<int>, bool)));
    connect(ClientInstance, &Client::roles_got, this, &RoomScene::chooseRole);
    connect(ClientInstance, &Client::directions_got, this, &RoomScene::chooseDirection);
    connect(ClientInstance, &Client::orders_got, this, &RoomScene::chooseOrder);
    connect(ClientInstance, &Client::kingdoms_got, this, &RoomScene::chooseKingdom);
    connect(ClientInstance, &Client::seats_arranged, this, &RoomScene::arrangeSeats);
    connect(ClientInstance, &Client::status_changed, this, &RoomScene::updateStatus);
    connect(ClientInstance, &Client::avatars_hiden, this, &RoomScene::hideAvatars);
    connect(ClientInstance, &Client::hp_changed, this, &RoomScene::changeHp);
    connect(ClientInstance, &Client::maxhp_changed, this, &RoomScene::changeMaxHp);
    connect(ClientInstance, &Client::pile_reset, this, &RoomScene::resetPiles);
    connect(ClientInstance, &Client::player_killed, this, &RoomScene::killPlayer);
    connect(ClientInstance, &Client::player_revived, this, &RoomScene::revivePlayer);
    connect(ClientInstance, &Client::dashboard_death, this, &RoomScene::setDashboardShadow);
    connect(ClientInstance, &Client::card_shown, this, &RoomScene::showCard);
    connect(ClientInstance, SIGNAL(gongxin(QList<int>, bool, QList<int>, QList<int>)), this, SLOT(doGongxin(QList<int>, bool, QList<int>, QList<int>)));
    connect(ClientInstance, &Client::focus_moved, this, &RoomScene::moveFocus);
    connect(ClientInstance, SIGNAL(emotion_set(QString, QString)), this, SLOT(setEmotion(QString, QString)));
    connect(ClientInstance, &Client::skill_invoked, this, &RoomScene::showSkillInvocation);
    connect(ClientInstance, &Client::skill_acquired, this, &RoomScene::acquireSkill);
    connect(ClientInstance, &Client::animated, this, &RoomScene::doAnimation);
    connect(ClientInstance, &Client::role_state_changed, this, &RoomScene::updateRoles);
    connect(ClientInstance, &Client::event_received, this, &RoomScene::handleGameEvent);

    connect(ClientInstance, &Client::game_started, this, &RoomScene::onGameStart);
    connect(ClientInstance, &Client::game_over, this, &RoomScene::onGameOver);
    connect(ClientInstance, &Client::standoff, this, &RoomScene::onStandoff);

    connect(ClientInstance, SIGNAL(move_cards_lost(int, QList<CardsMoveStruct>)), this, SLOT(loseCards(int, QList<CardsMoveStruct>)));
    connect(ClientInstance, SIGNAL(move_cards_got(int, QList<CardsMoveStruct>)), this, SLOT(getCards(int, QList<CardsMoveStruct>)));

    connect(ClientInstance, &Client::nullification_asked, dashboard, &Dashboard::controlNullificationButton);

    connect(ClientInstance, &Client::assign_asked, this, &RoomScene::startAssign);
    connect(ClientInstance, &Client::start_in_xs, this, &RoomScene::startInXs);

    connect(ClientInstance, &Client::triggers_got, this, &RoomScene::chooseTriggerOrder);
    connect(ClientInstance, &Client::skill_invalidity_changed, this, &RoomScene::skillInvalidityChange);

    guanxing_box = new GuanxingBox;
    guanxing_box->hide();
    addItem(guanxing_box);
    guanxing_box->setZValue(20000.0);

    connect(ClientInstance, &Client::guanxing, guanxing_box, &GuanxingBox::doGuanxing);
    guanxing_box->moveBy(-120, 0);

    m_chooseOptionsBox = new ChooseOptionsBox;
    m_chooseOptionsBox->hide();
    addItem(m_chooseOptionsBox);
    m_chooseOptionsBox->setZValue(30000.0);
    m_chooseOptionsBox->moveBy(-120, 0);

    time_label_widget = new TimeLabel;
    time_label_widget->setObjectName(QStringLiteral("time_label"));
    addItem(time_label_widget);
    time_label_widget->setZValue(10000);

    m_chooseTriggerOrderBox = new ChooseTriggerOrderBox;
    m_chooseTriggerOrderBox->hide();
    addItem(m_chooseTriggerOrderBox);
    m_chooseTriggerOrderBox->setZValue(30000.0);
    m_chooseTriggerOrderBox->moveBy(-120, 0);

    m_playerCardBox = new PlayerCardBox;
    m_playerCardBox->hide();
    addItem(m_playerCardBox);
    m_playerCardBox->setZValue(30000.0);
    m_playerCardBox->moveBy(-120, 0);

    card_container = new CardContainer;
    card_container->hide();
    addItem(card_container);
    card_container->setZValue(9.0);

    pileContainer = new CardContainer;
    pileContainer->hide();
    addItem(pileContainer);
    pileContainer->setZValue(9.0);

    m_chooseGeneralBox = new ChooseGeneralBox;
    m_chooseGeneralBox->hide();
    addItem(m_chooseGeneralBox);
    m_chooseGeneralBox->setZValue(30000.0);
    m_chooseGeneralBox->moveBy(-120, 0);

    connect(card_container, &CardContainer::item_chosen, ClientInstance, &Client::onPlayerChooseAG);
    connect(card_container, &CardContainer::item_gongxined, ClientInstance, &Client::onPlayerReplyGongxin);

    connect(ClientInstance, &Client::ag_filled, this, &RoomScene::fillCards);
    connect(ClientInstance, &Client::ag_taken, this, &RoomScene::takeAmazingGrace);
    connect(ClientInstance, &Client::ag_cleared, card_container, &CardContainer::clear);

    card_container->moveBy(-120, 0);

    connect(ClientInstance, &Client::skill_attached, this, &RoomScene::attachSkill);
    connect(ClientInstance, &Client::skill_detached, this, &RoomScene::detachSkill);

    enemy_box = nullptr;
    self_box = nullptr;

    if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3") || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1")
        || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_XMode")) {
        if (ClientInstance->serverInfo()->GameModeStr != QStringLiteral("06_XMode")) {
            connect(ClientInstance, &Client::generals_filled, this, &RoomScene::fillGenerals);
            connect(ClientInstance, &Client::general_asked, this, &RoomScene::startGeneralSelection);
            connect(ClientInstance, &Client::general_taken, this, &RoomScene::takeGeneral);
            connect(ClientInstance, &Client::general_recovered, this, &RoomScene::recoverGeneral);
        }
        connect(ClientInstance, &Client::arrange_started, this, &RoomScene::startArrange);

        arrange_button = nullptr;

        if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1")) {
            enemy_box = new KOFOrderBox(false, this);
            self_box = new KOFOrderBox(true, this);

            enemy_box->hide();
            self_box->hide();

            connect(ClientInstance, &Client::general_revealed, this, &RoomScene::revealGeneral);
        }
    }

    // chat box
    chat_box = new QTextEdit;
    chat_box->setObjectName(QStringLiteral("chat_box"));
    chat_box_widget = addWidget(chat_box);
    chat_box_widget->setZValue(-2.0);
    chat_box_widget->setObjectName(QStringLiteral("chat_box_widget"));
    chat_box->setReadOnly(true);
    chat_box->setTextColor(Config.TextEditColor);
    connect(ClientInstance, &Client::line_spoken, this, &RoomScene::appendChatBox);
    connect(ClientInstance, &Client::player_spoken, this, &RoomScene::showBubbleChatBox);

    // chat edit
    chat_edit = new QLineEdit;
    chat_edit->setObjectName(QStringLiteral("chat_edit"));
    chat_edit->setMaxLength(500);
    chat_edit_widget = addWidget(chat_edit);
    chat_edit_widget->setObjectName(QStringLiteral("chat_edit_widget"));
    chat_edit_widget->setZValue(-2.0);
    connect(chat_edit, &QLineEdit::returnPressed, this, &RoomScene::speak);
    chat_edit->setPlaceholderText(tr("Please enter text to chat ... "));

    chat_widget = new ChatWidget();
    chat_widget->setZValue(-0.1);
    addItem(chat_widget);
    connect(chat_widget, &ChatWidget::return_button_click, this, &RoomScene::speak);
    connect(chat_widget, &ChatWidget::chat_widget_msg, this, &RoomScene::appendChatEdit);

    if (ClientInstance->serverInfo()->DisableChat)
        chat_edit_widget->hide();

    // log box
    log_box = new ClientLogBox;
    log_box->setTextColor(Config.TextEditColor);
    log_box->setObjectName(QStringLiteral("log_box"));

    log_box_widget = addWidget(log_box);
    log_box_widget->setObjectName(QStringLiteral("log_box_widget"));
    log_box_widget->setZValue(-1.0);
    connect(ClientInstance, SIGNAL(log_received(QStringList)), log_box, SLOT(appendLog(QStringList)));

    prompt_box = new Window(tr("TouhouSatsu"), QSize(480, 200));
    prompt_box->setOpacity(0);
    prompt_box->setFlag(QGraphicsItem::ItemIsMovable);
    prompt_box->shift();
    prompt_box->setZValue(10);
    prompt_box->keepWhenDisappear();

    prompt_box_widget = new QGraphicsTextItem(prompt_box);
    prompt_box_widget->setParent(prompt_box);
    prompt_box_widget->setPos(40, 45);
    prompt_box_widget->setDefaultTextColor(Qt::white);

    QTextDocument *prompt_doc = ClientInstance->getPromptDoc();
    prompt_doc->setTextWidth(prompt_box->boundingRect().width() - 80);
    prompt_box_widget->setDocument(prompt_doc);

    QFont qf = Config.SmallFont;
    qf.setPixelSize(21);
    qf.setStyleStrategy(QFont::PreferAntialias);
    prompt_box_widget->setFont(qf);

    addItem(prompt_box);

    m_tableBg = new QGraphicsPixmapItem;
    m_tableBg->setZValue(-100000);
    addItem(m_tableBg);

    QHBoxLayout *skill_dock_layout = new QHBoxLayout;
    QMargins margins = skill_dock_layout->contentsMargins();
    margins.setTop(0);
    margins.setBottom(5);
    skill_dock_layout->setContentsMargins(margins);
    skill_dock_layout->addStretch();

    m_rolesBoxBackground.load(QStringLiteral("image/system/state.png"));
    m_rolesBox = new QGraphicsPixmapItem;
    addItem(m_rolesBox);
    QString roles = Sanguosha->getRoles(ClientInstance->serverInfo()->GameModeStr);
    m_pileCardNumInfoTextBox = addText(QString());
    m_pileCardNumInfoTextBox->setParentItem(m_rolesBox);
    m_pileCardNumInfoTextBox->setDocument(ClientInstance->getLinesDoc());
    m_pileCardNumInfoTextBox->setDefaultTextColor(Config.TextEditColor);
    updateRoles(roles);

    control_panel = addRect(0, 0, 500, 150, Qt::NoPen);
    control_panel->hide();

    add_robot = nullptr;
    fill_robots = nullptr;
    return_to_main_menu = nullptr;
    if (ClientInstance->serverInfo()->EnableAI) {
        add_robot = new Button(tr("Add a robot"));
        add_robot->setParentItem(control_panel);
        add_robot->setTransform(QTransform::fromTranslate(-add_robot->boundingRect().width() / 2, -add_robot->boundingRect().height() / 2), true);
        add_robot->setPos(0, -add_robot->boundingRect().height() - 10);
        add_robot->hide();

        fill_robots = new Button(tr("Fill robots"));
        fill_robots->setParentItem(control_panel);
        fill_robots->setTransform(QTransform::fromTranslate(-fill_robots->boundingRect().width() / 2, -fill_robots->boundingRect().height() / 2), true);
        fill_robots->setPos(0, 0);
        fill_robots->hide();

        connect(add_robot, &Button::clicked, ClientInstance, &Client::addRobot);
        connect(fill_robots, &Button::clicked, ClientInstance, &Client::fillRobots);
        // connect(Self, &Player::owner_changed, this, &RoomScene::showOwnerButtons);
    }

    return_to_main_menu = new Button(tr("Return to main menu"));
    return_to_main_menu->setParentItem(control_panel);
    return_to_main_menu->setTransform(QTransform::fromTranslate(-return_to_main_menu->boundingRect().width() / 2, -return_to_main_menu->boundingRect().height() / 2), true);
    return_to_main_menu->setPos(0, return_to_main_menu->boundingRect().height() + 10);
    return_to_main_menu->show();

    connect(return_to_main_menu, &Button::clicked, this, &RoomScene::return_to_start);
    control_panel->show();
    animations = new EffectAnimation(this);

    pausing_item = new QGraphicsRectItem;
    pausing_text = new QGraphicsSimpleTextItem(tr("Paused ..."));
    addItem(pausing_item);
    addItem(pausing_text);

    pausing_item->setOpacity(0.36);
    pausing_item->setZValue(1002.0);

    QFont font = Config.BigFont;
    font.setPixelSize(100);
    pausing_text->setFont(font);
    pausing_text->setBrush(Qt::white);
    pausing_text->setZValue(1002.1);

    pausing_item->hide();
    pausing_text->hide();

    pindian_box = new Window(tr("pindian"), QSize(255, 200), QStringLiteral("image/system/pindian.png"));
    pindian_box->setOpacity(0);
    pindian_box->setFlag(QGraphicsItem::ItemIsMovable);
    pindian_box->shift();
    pindian_box->setZValue(10);
    pindian_box->keepWhenDisappear();
    addItem(pindian_box);

    pindian_from_card = nullptr;
    pindian_to_card = nullptr;
}

RoomScene::~RoomScene()
{
    if (RoomSceneInstance == this)
        RoomSceneInstance = nullptr;
}

void RoomScene::handleGameEvent(const QVariant &args)
{
    QJsonArray arg = args.value<QJsonArray>();
    if (arg.isEmpty())
        return;

    GameEventType eventType = (GameEventType)arg[0].toInt();
    switch (eventType) {
    case S_GAME_EVENT_PLAYER_DYING: {
        Player *player = ClientInstance->findPlayer(arg[1].toString());
        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->setSaveMeIcon(true);
        Photo *photo = qobject_cast<Photo *>(container);
        if (photo != nullptr)
            photo->setFrame(Photo::S_FRAME_SOS);
        break;
    }
    case S_GAME_EVENT_PLAYER_QUITDYING: {
        Player *player = ClientInstance->findPlayer(arg[1].toString());
        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->setSaveMeIcon(false);
        Photo *photo = qobject_cast<Photo *>(container);
        if (photo != nullptr)
            photo->setFrame(Photo::S_FRAME_NO_FRAME);
        break;
    }
    case S_GAME_EVENT_HUASHEN: {
        Player *player = ClientInstance->findPlayer(arg[1].toString());
        QString huashenGeneral = arg[2].toString();
        QString huashenSkill = arg[3].toString();
        QString huashenGeneral2 = arg[4].toString();
        QString huashenSkill2 = arg[5].toString();

        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        if (huashenGeneral.isEmpty() && huashenGeneral2.isEmpty())
            container->stopHuaShen();
        else
            container->startHuaShen(huashenGeneral, huashenSkill, huashenGeneral2, huashenSkill2);

        break;
    }
    case S_GAME_EVENT_PLAY_EFFECT: {
        QString skillName = arg[1].toString();
        QString category;
        if (QSgsJsonUtils::isBool(arg[2])) {
            bool isMale = arg[2].toBool();
            category = isMale ? QStringLiteral("male") : QStringLiteral("female");
        } else if (QSgsJsonUtils::isString(arg[2]))
            category = arg[2].toString();
        int type = arg[3].toInt();
        Audio::playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(skillName, category, type));
        break;
    }
    case S_GAME_EVENT_JUDGE_RESULT: {
        int cardId = arg[1].toInt();
        bool takeEffect = arg[2].toBool();
        m_tablePile->showJudgeResult(cardId, takeEffect);
        break;
    }
    case S_GAME_EVENT_DETACH_SKILL: {
        QString player_name = arg[1].toString();
        QString skill_name = arg[2].toString();
        bool head = arg[3].toBool();

        Player *player = ClientInstance->findPlayer(player_name);
        player->detachSkill(skill_name);
        if (player == Self)
            detachSkill(skill_name, head);

        // stop huashen animation
        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->updateAvatarTooltip();
        break;
    }
    case S_GAME_EVENT_ACQUIRE_SKILL: {
        QString player_name = arg[1].toString();
        QString skill_name = arg[2].toString();
        bool head_skill = arg[3].toBool();
        const Skill *s = Sanguosha->skill(skill_name);
        ClientInstance->loadSkill(s);
        Player *player = ClientInstance->findPlayer(player_name);
        player->acquireSkill(skill_name);
        acquireSkill(player, skill_name, head_skill);

        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->updateAvatarTooltip();
        dashboard->expandSpecialCard(); //for chaoren
        if (skill_name == QStringLiteral("banling"))
            container->updateHp();
        break;
    }
    case S_GAME_EVENT_ADD_SKILL: {
        QString player_name = arg[1].toString();
        QString skill_name = arg[2].toString();
        bool head_skill = arg[3].toBool();

        const Skill *s = Sanguosha->skill(skill_name);
        ClientInstance->loadSkill(s);
        Player *player = ClientInstance->findPlayer(player_name);
        player->addSkill(skill_name, head_skill);

        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->updateAvatarTooltip();
        dashboard->expandSpecialCard(); //for chaoren
        if (skill_name == QStringLiteral("banling"))
            container->updateHp();
        break;
    }
    case S_GAME_EVENT_LOSE_SKILL: {
        QString player_name = arg[1].toString();
        QString skill_name = arg[2].toString();
        bool head = arg[3].toBool();
        Player *player = ClientInstance->findPlayer(player_name);
        player->loseSkill(skill_name, head);

        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->updateAvatarTooltip();
        dashboard->expandSpecialCard(); //for chaoren
        if (skill_name == QStringLiteral("banling"))
            container->updateHp();
        break;
    }
    case S_GAME_EVENT_PREPARE_SKILL:
    case S_GAME_EVENT_UPDATE_SKILL: {
        foreach (Photo *photo, photos)
            photo->updateAvatarTooltip();
        dashboard->updateAvatarTooltip();
        updateSkillButtons();
        dashboard->expandSpecialCard(); //for chaoren
        break;
    }
    case S_GAME_EVENT_UPDATE_PRESHOW: {
        QJsonObject preshow_map = arg[1].toObject();
        QStringList skill_names = preshow_map.keys();
        foreach (const QString &skill, skill_names) {
            bool showed = preshow_map[skill].toBool();

            Self->setSkillPreshowed(skill, showed);
            dashboard->updateHiddenMark();
            dashboard->updateRightHiddenMark();
        }
        break;
    }

    case S_GAME_EVENT_CHANGE_GENDER: {
        QString player_name = arg[1].toString();
        QSanguosha::Gender gender = (QSanguosha::Gender)arg[2].toInt();

        Player *player = ClientInstance->findPlayer(player_name);
        player->setGender(gender);

        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->updateAvatar(); // For Lu Boyan
        break;
    }
    case S_GAME_EVENT_CHANGE_HERO: {
        QString playerName = arg[1].toString();
        QString newHeroName = arg[2].toString();
        bool isSecondaryHero = arg[3].toBool();
        bool sendLog = arg[4].toBool();
        Player *player = ClientInstance->findPlayer(playerName);
        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        if (container != nullptr)
            container->refresh();
        if ((Sanguosha->general(newHeroName) != nullptr) && sendLog) {
            QString type = QStringLiteral("#Transfigure");
            QString arg2 = QString();
            if ((player->getGeneral2() != nullptr) && !isSecondaryHero) {
                type = QStringLiteral("#TransfigureDual");
                arg2 = QStringLiteral("GeneralA");
            } else if (isSecondaryHero) {
                type = QStringLiteral("#TransfigureDual");
                arg2 = QStringLiteral("GeneralB");
            }
            log_box->appendLog(type, player->objectName(), QStringList(), QString(), newHeroName, arg2);
        }

        //change bgm and backgroud
        if (!isHegemonyGameMode(ClientInstance->serverInfo()->GameModeStr) && player->isLord()) {
            ClientInstance->lord_name = newHeroName;
            setLordBGM(newHeroName);
            setLordBackdrop(newHeroName);
        }

        if (player != Self)
            break;
        const General *oldHero = isSecondaryHero ? player->getGeneral2() : player->general();
        const General *newHero = Sanguosha->general(newHeroName);
        if (oldHero != nullptr) {
            foreach (const Skill *skill, oldHero->skills(true, !isSecondaryHero))
                detachSkill(skill->name(), !isSecondaryHero);
            if (oldHero->hasSkill(QStringLiteral("pingyi")) && (container != nullptr))
                container->stopHuaShen();
        }

        if (newHero != nullptr) {
            foreach (const Skill *skill, newHero->skills(true, !isSecondaryHero)) {
                if (skill->isLordSkill() && !player->isLord())
                    continue;
                attachSkill(skill->name(), !isSecondaryHero);
            }
        }

        break;
    }
    case S_GAME_EVENT_PLAYER_REFORM: {
        Player *player = ClientInstance->findPlayer(arg[1].toString());
        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->updateReformState();
        break;
    }
    case S_GAME_EVENT_SKILL_INVOKED: {
        QString player_name = arg[1].toString();
        QString skill_name = arg[2].toString();
        const Skill *skill = Sanguosha->skill(skill_name);
        if ((skill != nullptr) && skill->isAttachedSkill())
            return;

        Player *player = ClientInstance->findPlayer(player_name);
        if (player == nullptr)
            return;

        bool display = player->hasValidSkill(skill_name);
        if (!display) {
            // for wuyu
            static QStringList bllmwuyu;
            if (bllmwuyu.isEmpty())
                bllmwuyu << QStringLiteral("bllmcaiyu") << QStringLiteral("bllmmingyu") << QStringLiteral("bllmseyu") << QStringLiteral("bllmshuiyu")
                         << QStringLiteral("bllmshiyu");
            if (bllmwuyu.contains(skill_name))
                display = true;
        }

        if (!display) {
            // for shenbao
            if (player->hasValidSkill(QStringLiteral("shenbao"))
                && (player->hasValidWeapon(skill_name) || player->hasValidArmor(skill_name) || player->hasValidTreasure(skill_name)))
                display = true;
        }

        if (display) {
            PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
            if (container != nullptr)
                container->showSkillName(skill_name, player == Self);
        }
        break;
    }
    case S_GAME_EVENT_PAUSE: {
        bool paused = arg[1].toBool();
        if (pausing_item->isVisible() != paused) {
            if (paused) {
                QBrush pausing_brush(QColor::fromRgb(QRandomGenerator::global()->generate()));
                pausing_item->setBrush(pausing_brush);
                bringToFront(pausing_item);
                bringToFront(pausing_text);
            }
            pausing_item->setVisible(paused);
            pausing_text->setVisible(paused);
        }
        break;
    }
    case S_GAME_EVENT_REVEAL_PINDIAN: {
        QString from_name = arg[1].toString();
        QString to_name = arg[3].toString();
        int from_id = arg[2].toInt();
        int to_id = arg[4].toInt();
        bool success = arg[5].toBool();
        pindian_success = success;
        QString reason = arg[6].toString();

        if (Config.value(QStringLiteral("EnablePindianBox"), true).toBool())
            showPindianBox(from_name, from_id, to_name, to_id, reason);
        else
            setEmotion(from_name, success ? QStringLiteral("success") : QStringLiteral("no-success"));
        break;
    }
    case S_GAME_EVENT_SKIN_CHANGED: {
        QString player_name = arg[1].toString();
        QString general_name = arg[2].toString();
        int skinIndex = arg[3].toInt();
        QString unique_general = general_name;
        if (unique_general.endsWith(QStringLiteral("_hegemony")))
            unique_general = unique_general.replace(QStringLiteral("_hegemony"), QString());

        int old_skin_index = Config.value(QStringLiteral("HeroSkin/%1").arg(unique_general), 0).toInt();
        if (skinIndex == old_skin_index)
            break;
        bool head = arg[4].toBool();

        Player *player = ClientInstance->findPlayer(player_name);

        QList<PlayerCardContainer *> playerCardContainers;
        foreach (Photo *photo, photos) {
            playerCardContainers.append(photo);
        }
        playerCardContainers.append(dashboard);

        bool noSkin = false;
        foreach (PlayerCardContainer *playerCardContainer, playerCardContainers) {
            if (noSkin)
                break;
            if (general_name == playerCardContainer->getPlayer()->generalName()
                || general_name == playerCardContainer->getPlayer()->getGeneral2Name()) { // check container which changed skin
                if ((player->generalName() == general_name || player->getGeneral2Name() == general_name)
                    && Self != player) { // check this roomscene instance of the players who need notify
                    QString generalIconPath;
                    QRect clipRegion;
                    G_ROOM_SKIN.getHeroSkinContainerGeneralIconPathAndClipRegion(general_name, skinIndex, generalIconPath, clipRegion);
                    if (!QFile::exists(generalIconPath)) {
                        noSkin = true;
                        continue;
                    }

                    Config.beginGroup(QStringLiteral("HeroSkin"));
                    (0 == skinIndex) ? Config.remove(unique_general) : Config.setValue(unique_general, skinIndex);
                    Config.endGroup();

                    foreach (HeroSkinContainer *heroSkinContainer, getHeroSkinContainers()) {
                        if (heroSkinContainer->getGeneralName() == general_name) {
                            heroSkinContainer->swapWithSkinItemUsed(skinIndex);
                        }
                    }
                    if (head)
                        playerCardContainer->getAvartarItem()->startChangeHeroSkinAnimation(general_name);
                    else
                        playerCardContainer->getSmallAvartarItem()->startChangeHeroSkinAnimation(general_name);
                }
            }
        }
        break;
    }
    case S_GAME_ROLE_STATUS_CHANGED: {
        QString player_name = arg[1].toString();
        bool shown = arg[2].toBool();

        Player *player = ClientInstance->findPlayer(player_name);
        QList<PlayerCardContainer *> playerCardContainers;
        foreach (Photo *photo, photos) {
            playerCardContainers.append(photo);
        }
        playerCardContainers.append(dashboard);
        foreach (PlayerCardContainer *playerCardContainer, playerCardContainers) {
            if (player->generalName() == playerCardContainer->getPlayer()->generalName()) {
                playerCardContainer->setRoleShown(shown);
                break;
            }
        }
        break;
    }
    default:
        break;
    }
}

QGraphicsItem *RoomScene::createDashboardButtons()
{
    QGraphicsItem *widget
        = new QGraphicsPixmapItem(G_ROOM_SKIN.getPixmap(QString::fromUtf8(QSanRoomSkin::S_SKIN_KEY_DASHBOARD_BUTTON_SET_BG)).scaled(G_DASHBOARD_LAYOUT.m_buttonSetSize));

    ok_button = new QSanButton(QStringLiteral("platter"), QStringLiteral("confirm"), widget);
    ok_button->setRect(G_DASHBOARD_LAYOUT.m_confirmButtonArea);
    cancel_button = new QSanButton(QStringLiteral("platter"), QStringLiteral("cancel"), widget);
    cancel_button->setRect(G_DASHBOARD_LAYOUT.m_cancelButtonArea);
    discard_button = new QSanButton(QStringLiteral("platter"), QStringLiteral("discard"), widget);
    discard_button->setRect(G_DASHBOARD_LAYOUT.m_discardButtonArea);
    connect(ok_button, &QSanButton::clicked, this, &RoomScene::doOkButton);
    connect(cancel_button, &QSanButton::clicked, this, &RoomScene::doCancelButton);
    connect(discard_button, &QSanButton::clicked, this, &RoomScene::doDiscardButton);

    trust_button = new QSanButton(QStringLiteral("platter"), QStringLiteral("trust"), widget);
    trust_button->setStyle(QSanButton::S_STYLE_TOGGLE);
    trust_button->setRect(G_DASHBOARD_LAYOUT.m_trustButtonArea);
    connect(trust_button, &QSanButton::clicked, this, &RoomScene::trust);
    // connect(Self, &Player::state_changed, this, &RoomScene::updateTrustButton);

    // set them all disabled
    ok_button->setEnabled(false);
    cancel_button->setEnabled(false);
    discard_button->setEnabled(false);
    trust_button->setEnabled(false);
    return widget;
}

QRectF ReplayerControlBar::boundingRect() const
{
    return QRectF(0, 0, S_BUTTON_WIDTH * 4 + S_BUTTON_GAP * 3, S_BUTTON_HEIGHT);
}

void ReplayerControlBar::paint(QPainter * /*painter*/, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
}

ReplayerControlBar::ReplayerControlBar(Dashboard *dashboard)
{
    QSanButton *play = nullptr;
    QSanButton *uniform = nullptr;
    QSanButton *slow_down = nullptr;
    QSanButton *speed_up = nullptr;

    uniform = new QSanButton(QStringLiteral("replay"), QStringLiteral("uniform"), this);
    slow_down = new QSanButton(QStringLiteral("replay"), QStringLiteral("slow-down"), this);
    play = new QSanButton(QStringLiteral("replay"), QStringLiteral("pause"), this);
    speed_up = new QSanButton(QStringLiteral("replay"), QStringLiteral("speed-up"), this);
    play->setStyle(QSanButton::S_STYLE_TOGGLE);
    uniform->setStyle(QSanButton::S_STYLE_TOGGLE);

    int step = S_BUTTON_GAP + S_BUTTON_WIDTH;
    uniform->setPos(0, 0);
    slow_down->setPos(step, 0);
    play->setPos(step * 2, 0);
    speed_up->setPos(step * 3, 0);

    time_label = new QLabel;
    time_label->setAttribute(Qt::WA_NoSystemBackground);
    time_label->setText(QStringLiteral("-----------------------------------------------------"));
    QPalette palette;
    palette.setColor(QPalette::WindowText, Config.TextEditColor);
    time_label->setPalette(palette);

    QGraphicsProxyWidget *widget = new QGraphicsProxyWidget(this);
    widget->setWidget(time_label);
    widget->setPos(step * 4, 0);

    Replayer *replayer = ClientInstance->getReplayer();
    connect(play, &QSanButton::clicked, replayer, &Replayer::toggle);
    connect(uniform, &QSanButton::clicked, replayer, &Replayer::uniform);
    connect(slow_down, &QSanButton::clicked, replayer, &Replayer::slowDown);
    connect(speed_up, &QSanButton::clicked, replayer, &Replayer::speedUp);
    connect(replayer, &Replayer::elasped, this, &ReplayerControlBar::setTime);
    connect(replayer, &Replayer::speed_changed, this, &ReplayerControlBar::setSpeed);

    speed = replayer->getSpeed();
    setParentItem(dashboard);
    setPos(S_BUTTON_GAP, -S_BUTTON_GAP - S_BUTTON_HEIGHT);

    duration_str = FormatTime(replayer->getDuration());
}

QString ReplayerControlBar::FormatTime(int secs)
{
    int minutes = secs / 60;
    int remainder = secs % 60;

    return QStringLiteral("%1:%2").arg(minutes, 2, 10, QLatin1Char('0')).arg(remainder, 2, 10, QLatin1Char('0'));
}

void ReplayerControlBar::setSpeed(qreal speed)
{
    this->speed = speed;
}

void ReplayerControlBar::setTime(int secs)
{
    time_label->setText(QStringLiteral("<b>x%1 </b> [%2/%3]").arg(QString::number(speed), FormatTime(secs), duration_str));
}

TimeLabel::TimeLabel()
{
    time_label = new QLabel(QStringLiteral("00:00:00"));
    time_label->setAttribute(Qt::WA_NoSystemBackground);
    QPalette palette;
    palette.setColor(QPalette::WindowText, Qt::yellow);
    time_label->setPalette(palette);
    timer = new QTimer(this);

    connect(timer, &QTimer::timeout, this, &TimeLabel::updateTimerLabel);

    QGraphicsProxyWidget *widget = new QGraphicsProxyWidget(this);
    widget->setWidget(time_label);
    widget->setPos(0, 0);
}

QRectF TimeLabel::boundingRect() const
{
    return QRectF(0, 0, time_label->width(), time_label->height());
}

void TimeLabel::paint(QPainter * /*painter*/, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
}

void TimeLabel::updateTimerLabel()
{
    QTime current = QTime::fromString(time_label->text(), QStringLiteral("hh:mm:ss")).addSecs(1);
    time_label->setText(current.toString(QStringLiteral("hh:mm:ss")));
    //add 1 sec each call, update time label
}

void TimeLabel::initializeLabel()
{
    time_label->setText(QStringLiteral("00:00:00"));
    timer->stop();
}

void TimeLabel::startCounting()
{
    timer->start(1s);
}

void RoomScene::createReplayControlBar()
{
    m_replayControl = new ReplayerControlBar(dashboard);
}

void RoomScene::_getSceneSizes(QSize &minSize, QSize &maxSize)
{
    if (photos.size() >= 8) {
        minSize = _m_roomLayout->m_minimumSceneSize10Player;
        maxSize = _m_roomLayout->m_maximumSceneSize10Player;
    } else {
        minSize = _m_roomLayout->m_minimumSceneSize;
        maxSize = _m_roomLayout->m_maximumSceneSize;
    }
}

void RoomScene::adjustItems()
{
    QRectF displayRegion = sceneRect();

    // switch between default & compact skin depending on scene size
    QSanSkinFactory &factory = QSanSkinFactory::getInstance();
    QString skinName = factory.getCurrentSkinName();

    QSize minSize;
    QSize maxSize;
    _getSceneSizes(minSize, maxSize);
    if (skinName == factory.S_DEFAULT_SKIN_NAME) {
        if (displayRegion.width() < minSize.width() || displayRegion.height() < minSize.height()) {
            QThread *thread = QCoreApplication::instance()->thread();
            thread->blockSignals(true);
            factory.switchSkin(factory.S_COMPACT_SKIN_NAME);
            thread->blockSignals(false);
            foreach (Photo *photo, photos)
                photo->repaintAll();
            dashboard->repaintAll();
        }
    } else if (skinName == factory.S_COMPACT_SKIN_NAME) {
        if (displayRegion.width() > maxSize.width() && displayRegion.height() > maxSize.height()) {
            QThread *thread = QCoreApplication::instance()->thread();
            thread->blockSignals(true);
            factory.switchSkin(factory.S_DEFAULT_SKIN_NAME);
            thread->blockSignals(false);
            foreach (Photo *photo, photos)
                photo->repaintAll();
            dashboard->repaintAll();
        }
    }

    // update the sizes since we have reloaded the skin.
    _getSceneSizes(minSize, maxSize);

    if (displayRegion.left() != 0 || displayRegion.top() != 0 || displayRegion.bottom() < minSize.height() || displayRegion.right() < minSize.width()) {
        displayRegion.setLeft(0);
        displayRegion.setTop(0);
        double sy = minSize.height() / displayRegion.height();
        double sx = minSize.width() / displayRegion.width();
        double scale = qMax(sx, sy);
        displayRegion.setBottom(scale * displayRegion.height());
        displayRegion.setRight(scale * displayRegion.width());
        setSceneRect(displayRegion);
    }

    int padding = _m_roomLayout->m_scenePadding;
    displayRegion.moveLeft(displayRegion.x() + padding);
    displayRegion.moveTop(displayRegion.y() + padding);
    displayRegion.setWidth(displayRegion.width() - padding * 2);
    displayRegion.setHeight(displayRegion.height() - padding * 2);

    // set dashboard
    dashboard->setX(displayRegion.x());
    dashboard->setWidth(displayRegion.width());
    dashboard->setY(displayRegion.height() - dashboard->boundingRect().height());

    // set infoplane
    _m_infoPlane.setWidth(displayRegion.width() * _m_roomLayout->m_infoPlaneWidthPercentage);
    _m_infoPlane.moveRight(displayRegion.right());
    _m_infoPlane.setTop(displayRegion.top() + _m_roomLayout->m_roleBoxHeight);
    _m_infoPlane.setBottom(dashboard->y() - _m_roomLayout->m_chatTextBoxHeight - 35);
    m_rolesBoxBackground = m_rolesBoxBackground.scaled(_m_infoPlane.width(), _m_roomLayout->m_roleBoxHeight);
    m_rolesBox->setPixmap(m_rolesBoxBackground);
    m_rolesBox->setPos(_m_infoPlane.left(), displayRegion.top());
    m_rolesBox->setPos(_m_infoPlane.left(), displayRegion.top());

    log_box_widget->setPos(_m_infoPlane.topLeft());
    log_box->resize(_m_infoPlane.width(), _m_infoPlane.height() * _m_roomLayout->m_logBoxHeightPercentage);
    chat_box_widget->setPos(_m_infoPlane.left(), _m_infoPlane.bottom() - _m_infoPlane.height() * _m_roomLayout->m_chatBoxHeightPercentage);
    chat_box->resize(_m_infoPlane.width(), _m_infoPlane.bottom() - chat_box_widget->y());
    chat_edit_widget->setPos(_m_infoPlane.left(), _m_infoPlane.bottom());
    chat_edit->resize(_m_infoPlane.width() - chat_widget->boundingRect().width(), _m_roomLayout->m_chatTextBoxHeight);
    chat_widget->setPos(_m_infoPlane.right() - chat_widget->boundingRect().width(),
                        chat_edit_widget->y() + (_m_roomLayout->m_chatTextBoxHeight - chat_widget->boundingRect().height()) / 2);

    padding += _m_roomLayout->m_photoRoomPadding;
    if (self_box != nullptr)
        self_box->setPos(_m_infoPlane.left() - padding - self_box->boundingRect().width(),
                         sceneRect().height() - padding - self_box->boundingRect().height() - G_DASHBOARD_LAYOUT.m_normalHeight - G_DASHBOARD_LAYOUT.m_floatingAreaHeight);
    if (enemy_box != nullptr)
        enemy_box->setPos(padding * 2, padding * 2);

    padding -= _m_roomLayout->m_photoRoomPadding;
    m_tablew = displayRegion.width();
    m_tableh = displayRegion.height();

    if (image_path.isNull() || !QFile::exists(image_path)) {
        image_path = Config.TableBgImage;
    }
    changeTableBg(image_path);

    updateRolesBox();
    setChatBoxVisible(chat_box_widget->isVisible());
    QMapIterator<QString, BubbleChatBox *> iter(m_bubbleChatBoxs);
    while (iter.hasNext()) {
        iter.next();
        iter.value()->setArea(getBubbleChatBoxShowArea(iter.key()));
    }
}

void RoomScene::_dispersePhotos(QList<Photo *> &photos, const QRectF &fillRegion, Qt::Orientation orientation, Qt::Alignment align)
{
    double photoWidth = _m_photoLayout->m_normalWidth;
    double photoHeight = _m_photoLayout->m_normalHeight;
    int numPhotos = photos.size();
    if (numPhotos == 0)
        return;
    Qt::Alignment hAlign = align & Qt::AlignHorizontal_Mask;
    Qt::Alignment vAlign = align & Qt::AlignVertical_Mask;

    double startX = 0;
    double startY = 0;
    double stepX = NAN;
    double stepY = NAN;

    if (orientation == Qt::Horizontal) {
        double maxWidth = fillRegion.width();
        stepX = qMax(photoWidth + G_ROOM_LAYOUT.m_photoHDistance, maxWidth / numPhotos);
        stepY = 0;
    } else {
        stepX = 0;
        stepY = G_ROOM_LAYOUT.m_photoVDistance + photoHeight;
    }

    switch (vAlign) {
    case Qt::AlignTop:
        startY = fillRegion.top() + photoHeight / 2;
        break;
    case Qt::AlignBottom:
        startY = fillRegion.bottom() - photoHeight / 2 - stepY * (numPhotos - 1);
        break;
    case Qt::AlignVCenter:
        startY = fillRegion.center().y() - stepY * (numPhotos - 1) / 2.0;
        break;
    default:
        Q_ASSERT(false);
    }
    switch (hAlign) {
    case Qt::AlignLeft:
        startX = fillRegion.left() + photoWidth / 2;
        break;
    case Qt::AlignRight:
        startX = fillRegion.right() - photoWidth / 2 - stepX * (numPhotos - 1);
        break;
    case Qt::AlignHCenter:
        startX = fillRegion.center().x() - stepX * (numPhotos - 1) / 2.0;
        break;
    default:
        Q_ASSERT(false);
    }

    for (int i = 0; i < numPhotos; i++) {
        Photo *photo = photos[i];
        QPointF newPos = QPointF(startX + stepX * i, startY + stepY * i);
        photo->setPos(newPos);
    }
}

void RoomScene::updateTable()
{
    int pad = _m_roomLayout->m_scenePadding + _m_roomLayout->m_photoRoomPadding;
    int tablew = log_box_widget->x() - pad * 2;
    int tableh = sceneRect().height() - pad * 2 - dashboard->boundingRect().height();
    if ((ClientInstance->serverInfo()->GameModeStr == QStringLiteral("04_1v3") || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3")) && game_started)
        tableh -= _m_roomLayout->m_photoVDistance;
    int photow = _m_photoLayout->m_normalWidth;
    int photoh = _m_photoLayout->m_normalHeight;

    // Layout:
    //    col1           col2
    // _______________________
    // |_2_|______1_______|_0_| row1
    // |   |              |   |
    // | 4 |    table     | 3 |
    // |___|______________|___|
    // |      dashboard       |
    // ------------------------
    // region 5 = 0 + 3, region 6 = 2 + 4, region 7 = 0 + 1 + 2

    static int regularSeatIndex[][9]
        = {{1}, {5, 6}, {5, 1, 6}, {3, 1, 1, 4}, {3, 1, 1, 1, 4}, {5, 5, 1, 1, 6, 6}, {5, 5, 1, 1, 1, 6, 6}, {3, 3, 7, 7, 7, 7, 4, 4}, {3, 3, 7, 7, 7, 7, 7, 4, 4}};
    static int hulaoSeatIndex[][3] = {{1, 1, 1}, // if self is shenlvbu
                                      {3, 3, 1},
                                      {3, 1, 4},
                                      {1, 4, 4}};
    static int kof3v3SeatIndex[][5] = {
        {3, 1, 1, 1, 4}, // lord
        {1, 1, 1, 4, 4}, // rebel (left), same with loyalist (left)
        {3, 3, 1, 1, 1} // loyalist (right), same with rebel (right)
    };

    double hGap = _m_roomLayout->m_photoHDistance;
    double vGap = _m_roomLayout->m_photoVDistance;
    double col1 = photow + hGap;
    double col2 = tablew - col1;
    double row1 = photoh + vGap;
    double row2 = tableh;

    const int C_NUM_REGIONS = 8;
    QRectF seatRegions[] = {QRectF(col2, pad, col1, row1),        QRectF(col1, pad, col2 - col1, row1), QRectF(pad, pad, col1, row1), QRectF(col2, row1, col1, row2 - row1),
                            QRectF(pad, row1, col1, row2 - row1), QRectF(col2, pad, col1, row2),        QRectF(pad, pad, col1, row2), QRectF(pad, pad, col1 + col2, row1)};

    static Qt::Alignment aligns[] = {
        Qt::AlignRight | Qt::AlignTop,    Qt::AlignHCenter | Qt::AlignTop,   Qt::AlignLeft | Qt::AlignTop,     Qt::AlignRight | Qt::AlignVCenter,
        Qt::AlignLeft | Qt::AlignVCenter, Qt::AlignRight | Qt::AlignVCenter, Qt::AlignLeft | Qt::AlignVCenter, Qt::AlignHCenter | Qt::AlignTop,
    };

    static Qt::Alignment kofAligns[] = {
        Qt::AlignRight | Qt::AlignTop,   Qt::AlignHCenter | Qt::AlignTop,  Qt::AlignLeft | Qt::AlignTop,    Qt::AlignRight | Qt::AlignBottom,
        Qt::AlignLeft | Qt::AlignBottom, Qt::AlignRight | Qt::AlignBottom, Qt::AlignLeft | Qt::AlignBottom, Qt::AlignHCenter | Qt::AlignTop,
    };

    Qt::Orientation orients[] = {Qt::Horizontal, Qt::Horizontal, Qt::Horizontal, Qt::Vertical, Qt::Vertical, Qt::Vertical, Qt::Vertical, Qt::Horizontal};

    QRectF tableRect(col1, row1, col2 - col1, row2 - row1);

    QRect tableBottomBar(0, 0, log_box_widget->x() - col1, G_DASHBOARD_LAYOUT.m_floatingAreaHeight);
    tableBottomBar.moveBottomLeft(QPoint((int)tableRect.left(), 0));
    dashboard->setFloatingArea(tableBottomBar);

    m_tableCenterPos = tableRect.center();
    control_panel->setPos(m_tableCenterPos);
    m_tablePile->setPos(m_tableCenterPos);
    m_tablePile->setSize(qMax((int)tableRect.width() - _m_roomLayout->m_discardPilePadding * 2, _m_roomLayout->m_discardPileMinWidth), _m_commonLayout->m_cardNormalHeight);

    m_tablePile->adjustCards();
    card_container->setPos(m_tableCenterPos);
    pileContainer->setPos(m_tableCenterPos);
    guanxing_box->setPos(m_tableCenterPos);
    m_chooseGeneralBox->setPos(m_tableCenterPos - QPointF(m_chooseGeneralBox->boundingRect().width() / 2, m_chooseGeneralBox->boundingRect().height() / 2));
    m_chooseOptionsBox->setPos(m_tableCenterPos - QPointF(m_chooseOptionsBox->boundingRect().width() / 2, m_chooseOptionsBox->boundingRect().height() / 2));
    m_chooseTriggerOrderBox->setPos(m_tableCenterPos - QPointF(m_chooseTriggerOrderBox->boundingRect().width() / 2, m_chooseTriggerOrderBox->boundingRect().height() / 2));
    m_playerCardBox->setPos(m_tableCenterPos - QPointF(m_playerCardBox->boundingRect().width() / 2, m_playerCardBox->boundingRect().height() / 2));
    prompt_box->setPos(m_tableCenterPos);
    pausing_text->setPos(m_tableCenterPos - pausing_text->boundingRect().center());
    pausing_item->setRect(sceneRect());
    pausing_item->setPos(0, 0);

    int *seatToRegion = regularSeatIndex[photos.length() - 1];
    bool pkMode = false;
    if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("04_1v3") && game_started) {
        seatToRegion = hulaoSeatIndex[Self->seat() - 1];
        pkMode = true;
    } else if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3") && game_started) {
        seatToRegion = kof3v3SeatIndex[(Self->seat() - 1) % 3];
        pkMode = true;
    }
    QList<Photo *> photosInRegion[C_NUM_REGIONS];
    // TODO: out of bounds when regionIndex == 9
    int n = photos.length();
    for (int i = 0; i < n; i++) {
        int regionIndex = seatToRegion[i];
        if (regionIndex == 4 || regionIndex == 6 || regionIndex == 9)
            photosInRegion[regionIndex].append(photos[i]);
        else
            photosInRegion[regionIndex].prepend(photos[i]);
    }
    for (int i = 0; i < C_NUM_REGIONS; i++) {
        if (photosInRegion[i].isEmpty())
            continue;
        Qt::Alignment align;
        if (pkMode)
            align = kofAligns[i];
        else
            align = aligns[i];
        Qt::Orientation orient = orients[i];

        int hDist = G_ROOM_LAYOUT.m_photoHDistance;
        QRect floatingArea(0, 0, hDist, G_PHOTO_LAYOUT.m_normalHeight);
        // if the photo is on the right edge of table
        if (i == 0 || i == 3 || i == 5)
            floatingArea.moveRight(0);
        else
            floatingArea.moveLeft(G_PHOTO_LAYOUT.m_normalWidth);

        foreach (Photo *photo, photosInRegion[i])
            photo->setFloatingArea(floatingArea);
        _dispersePhotos(photosInRegion[i], seatRegions[i], orient, align);
    }
}

void RoomScene::addPlayer(Player *player)
{
    for (int i = 0; i < photos.length(); i++) {
        Photo *photo = photos[i];
        if (photo->getPlayer() == nullptr) {
            photo->setPlayer(player);
            name2photo[player->objectName()] = photo;

            if (!Self->hasFlag(QStringLiteral("marshalling")))
                Audio::playSystemAudioEffect(QStringLiteral("add-player"));

            return;
        }
    }
}

void RoomScene::removePlayer(const QString &player_name)
{
    Photo *photo = name2photo[player_name];
    if (photo != nullptr) {
        photo->setPlayer(nullptr);
        name2photo.remove(player_name);
        Audio::playSystemAudioEffect(QStringLiteral("remove-player"));
    }
}

void RoomScene::arrangeSeats(const QList<const Player *> &seats)
{
    // rearrange the photos
    Q_ASSERT(seats.length() == photos.length());

    for (int i = 0; i < seats.length(); i++) {
        const Player *player = seats.at(i);
        for (int j = i; j < photos.length(); j++) {
            if (photos.at(j)->getPlayer() == player) {
                photos.swapItemsAt(i, j);
                break;
            }
        }
    }
    game_started = true;
    QParallelAnimationGroup *group = new QParallelAnimationGroup(this);
    updateTable();

    group->start(QAbstractAnimation::DeleteWhenStopped);

    // set item to player mapping
    if (item2player.isEmpty()) {
        item2player.insert(dashboard, Self);
        connect(dashboard, &PlayerCardContainer::selected_changed, this, &RoomScene::updateSelectedTargets);
        foreach (Photo *photo, photos) {
            item2player.insert(photo, photo->getPlayer());
            connect(photo, &PlayerCardContainer::selected_changed, this, &RoomScene::updateSelectedTargets);
            connect(photo, &PlayerCardContainer::enable_changed, this, &RoomScene::onEnabledChange);
        }
    }

    QStringList names = name2photo.keys();
    foreach (const QString &who, names) {
        if (m_bubbleChatBoxs.contains(who)) {
            m_bubbleChatBoxs[who]->setArea(getBubbleChatBoxShowArea(who));
        }
    }

    bool all_robot = true;
    foreach (const Player *p, ClientInstance->players()) {
        if (p != Self && p->getState() != QStringLiteral("robot")) {
            all_robot = false;
            break;
        }
    }
    if (all_robot)
        setChatBoxVisible(false);
}

// @todo: The following 3 fuctions are for drag & use feature. Currently they are very buggy and
// cause a lot of major problems. We should look into this later.
void RoomScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);

    if (isHegemonyGameMode(ClientInstance->serverInfo()->GameModeStr)) {
        bool changed = false;
        QPoint point(event->pos().x(), event->pos().y());
        foreach (Photo *photo, photos) {
            HegemonyRoleComboBox *box = photo->getHegemonyRoleComboBox();
            if (!box->boundingRect().contains(point) && box->isExpanding())
                changed = true;
        }
        HegemonyRoleComboBox *box = dashboard->getHegemonyRoleComboBox();
        if (!box->boundingRect().contains(point) && box->isExpanding())
            changed = true;
        if (changed)
            emit cancel_role_box_expanding();
    }
}

void RoomScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
}

void RoomScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseMoveEvent(event);
}

void RoomScene::enableTargets(const Card *card)
{
    bool enabled = true;
    if (card != nullptr) {
        Client::Status status = ClientInstance->getStatus();
        if (status == Client::Playing && !card->face()->isAvailable(Self, card))
            enabled = false;
        if (status == Client::Responding || status == Client::RespondingUse) {
            QSanguosha::HandlingMethod method = card->handleMethod();
            if (status == Client::Responding && method == QSanguosha::MethodUse)
                method = QSanguosha::MethodResponse;
            if (Self->isCardLimited(card, method))
                enabled = false;
        }
        if (status == Client::RespondingForDiscard && Self->isCardLimited(card, QSanguosha::MethodDiscard))
            enabled = false;
    }
    if (!enabled) {
        ok_button->setEnabled(false);
        return;
    }

    selected_targets.clear();

    // unset avatar and all photo
    foreach (PlayerCardContainer *item, item2player.keys())
        item->setSelected(false);

    if (card == nullptr) {
        foreach (PlayerCardContainer *item, item2player.keys()) {
            QGraphicsItem *animationTarget = item->getMouseClickReceiver();
            animations->effectOut(animationTarget);
            item->setFlag(QGraphicsItem::ItemIsSelectable, false);
            item->setEnabled(true);
        }

        ok_button->setEnabled(false);
        return;
    }

    Client::Status status = ClientInstance->getStatus();

    if (card->face()->targetFixed(Self, card)
        || ((status & Client::ClientStatusBasicMask) == Client::Responding
            && (status == Client::Responding || (card->face()->type() != QSanguosha::TypeSkill && status != Client::RespondingUse)))
        || ClientInstance->getStatus() == Client::AskForShowOrPindian) {
        foreach (PlayerCardContainer *item, item2player.keys()) {
            QGraphicsItem *animationTarget = item->getMouseClickReceiver();
            animations->effectOut(animationTarget);
            item->setFlag(QGraphicsItem::ItemIsSelectable, false);
        }
        if (card->face()->isKindOf(QStringLiteral("AOE")) && status == Client::RespondingUse)
            ok_button->setEnabled(card->face()->isAvailable(Self, card));
        else if (card->face()->isKindOf(QStringLiteral("Peach")) && status == Client::RespondingUse)
            ok_button->setEnabled(card->face()->isAvailable(Self, card));
        else if (card->face()->isKindOf(QStringLiteral("QirenCard")) && status == Client::RespondingUse)
            ok_button->setEnabled(card->face()->isAvailable(Self, card));
        else
            ok_button->setEnabled(true);
        return;
    }

    updateTargetsEnablity(card);

    if (selected_targets.isEmpty()) {
        if (card->face()->isKindOf(QStringLiteral("Slash")) && Self->hasFlag(QStringLiteral("slashTargetFixToOne"))) {
            unselectAllTargets();
            foreach (Photo *photo, photos) {
                if ((photo->flags() & QGraphicsItem::ItemIsSelectable) != 0)
                    if (!photo->isSelected()) {
                        photo->setSelected(true);
                        break;
                    }
            }
        } else if (Config.EnableAutoTarget) {
            if (!card->face()->targetsFeasible(selected_targets, Self, card)) {
                unselectAllTargets();
                int count = 0;
                foreach (Photo *photo, photos)
                    if ((photo->flags() & QGraphicsItem::ItemIsSelectable) != 0)
                        count++;
                if ((dashboard->flags() & QGraphicsItem::ItemIsSelectable) != 0)
                    count++;
                if (count == 1)
                    selectNextTarget(false);
            }
        }
    }

    ok_button->setEnabled(card->face()->targetsFeasible(selected_targets, Self, card));
}

void RoomScene::updateTargetsEnablity(const Card *card)
{
    QMapIterator<PlayerCardContainer *, const Player *> itor(item2player);
    while (itor.hasNext()) {
        itor.next();

        PlayerCardContainer *item = itor.key();
        const Player *player = itor.value();
        int maxVotes = 0;

        if (card != nullptr) {
            maxVotes = card->face()->targetFilter(selected_targets, player, Self, card);
            item->setMaxVotes(maxVotes);
        }

        if (item->isSelected())
            continue;

        bool enabled = (card == nullptr) || ((ClientInstance->isProhibited(Self, player, card, selected_targets) == nullptr) && maxVotes > 0);

        QGraphicsItem *animationTarget = item->getMouseClickReceiver();
        if (enabled)
            animations->effectOut(animationTarget);
        else if ((animationTarget->graphicsEffect() == nullptr) || !animationTarget->graphicsEffect()->inherits("SentbackEffect"))
            animations->sendBack(animationTarget);

        if (card != nullptr)
            item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
    }
}

void RoomScene::updateSelectedTargets()
{
    PlayerCardContainer *item = qobject_cast<PlayerCardContainer *>(sender());
    if (item == nullptr)
        return;

    const Card *card = dashboard->getSelected();
    if (card != nullptr) {
        const Player *player = item2player.value(item, NULL);
        if (item->isSelected())
            selected_targets.append(player);
        else {
            selected_targets.removeAll(player);
            foreach (const Player *cp, selected_targets) {
                QList<const Player *> tempPlayers = QList<const Player *>(selected_targets);
                tempPlayers.removeAll(cp);
                if (card->face()->targetFilter(tempPlayers, cp, Self, card) == 0 || (ClientInstance->isProhibited(Self, cp, card, selected_targets) != nullptr)) {
                    selected_targets.clear();
                    unselectAllTargets();
                    return;
                }
            }
        }
        ok_button->setEnabled(card->face()->targetsFeasible(selected_targets, Self, card));
    } else {
        selected_targets.clear();
    }

    updateTargetsEnablity(card);
}

void RoomScene::keyReleaseEvent(QKeyEvent *event)
{
    if (!Config.EnableHotKey)
        return;
    if (chat_edit->hasFocus())
        return;

    bool control_is_down = (event->modifiers() & Qt::ControlModifier) != 0;
    bool alt_is_down = (event->modifiers() & Qt::AltModifier) != 0;

    switch (event->key()) {
    case Qt::Key_F1:
        break;
    case Qt::Key_F2:
        chooseSkillButton();
        break;
    case Qt::Key_F3:
        dashboard->beginSorting();
        break;
    case Qt::Key_F4:
        dashboard->reverseSelection();
        break;
    case Qt::Key_F5: {
        adjustItems();
        break;
    }
    case Qt::Key_F6: {
        if (!Self || /*!Self->isOwner() || */ ClientInstance->players().length() < Sanguosha->getPlayerCount(ClientInstance->serverInfo()->GameModeStr))
            break;
        foreach (const Player *p, ClientInstance->players()) {
            if (p != Self && p->isAlive() && p->getState() != QStringLiteral("robot"))
                break;
        }
        bool paused = pausing_text->isVisible();
        ClientInstance->notifyServer(S_COMMAND_PAUSE, !paused);
        break;
    }
    case Qt::Key_F7: {
        if (control_is_down) {
            if ((add_robot != nullptr) && add_robot->isVisible())
                ClientInstance->addRobot();
        } else if ((fill_robots != nullptr) && fill_robots->isVisible())
            ClientInstance->fillRobots();
        break;
    }
    case Qt::Key_F8: {
        setChatBoxVisible(!chat_box_widget->isVisible());
        break;
    }
#if 0
    case Qt::Key_S:
        dashboard->selectCard(QStringLiteral("slash"));
        break;
    case Qt::Key_J:
        dashboard->selectCard(QStringLiteral("jink"));
        break;
    case Qt::Key_P:
        dashboard->selectCard(QStringLiteral("peach"));
        break;
    case Qt::Key_O:
        dashboard->selectCard(QStringLiteral("analeptic"));
        break;

    case Qt::Key_E:
        dashboard->selectCard(QStringLiteral("equip"));
        break;
    case Qt::Key_W:
        dashboard->selectCard(QStringLiteral("weapon"));
        break;
    case Qt::Key_F:
        dashboard->selectCard(QStringLiteral("armor"));
        break;
    case Qt::Key_H:
        dashboard->selectCard(QStringLiteral("defensive_horse+offensive_horse"));
        break;

    case Qt::Key_T:
        dashboard->selectCard(QStringLiteral("trick"));
        break;
    case Qt::Key_A:
        dashboard->selectCard(QStringLiteral("aoe"));
        break;
    case Qt::Key_N:
        dashboard->selectCard(QStringLiteral("nullification"));
        break;
    case Qt::Key_Q:
        dashboard->selectCard(QStringLiteral("snatch"));
        break;
    case Qt::Key_C:
        dashboard->selectCard(QStringLiteral("dismantlement"));
        break;
    case Qt::Key_U:
        dashboard->selectCard(QStringLiteral("duel"));
        break;
    case Qt::Key_L:
        dashboard->selectCard(QStringLiteral("lightning"));
        break;
    case Qt::Key_I:
        dashboard->selectCard(QStringLiteral("indulgence"));
        break;
    case Qt::Key_B:
        dashboard->selectCard(QStringLiteral("supply_shortage"));
        break;

    case Qt::Key_Left:
        dashboard->selectCard(QStringLiteral("."), false, control_is_down);
        break;
    case Qt::Key_Right:
        dashboard->selectCard(QStringLiteral("."), true, control_is_down);
        break; // iterate all cards
#endif
    case Qt::Key_Return: {
        if (ok_button->isEnabled())
            doOkButton();
        break;
    }
    case Qt::Key_Escape: {
        if (ClientInstance->getStatus() == Client::Playing) {
            dashboard->unselectAll();
            enableTargets(nullptr);
        } else
            dashboard->unselectAll();
        break;
    }
    case Qt::Key_Space: {
        if (cancel_button->isEnabled())
            doCancelButton();
        else if (discard_button->isEnabled())
            doDiscardButton();
    }

    case Qt::Key_0:
    case Qt::Key_1:
    case Qt::Key_2:
    case Qt::Key_3:
    case Qt::Key_4: {
        int position = event->key() - Qt::Key_0;
        if (position != 0 && alt_is_down) {
            dashboard->selectEquip(position);
            break;
        }
    }
    case Qt::Key_5:
    case Qt::Key_6:
    case Qt::Key_7:
    case Qt::Key_8:
    case Qt::Key_9: {
        int order = event->key() - Qt::Key_0;
        selectTarget(order, control_is_down);
        break;
    }

    case Qt::Key_D: {
        if (Self == nullptr)
            return;
        foreach (Photo *photo, photos) {
            if ((photo->getPlayer() != nullptr) && photo->getPlayer()->isAlive())
                photo->showDistance();
        }
        break;
    }
    }
}

void RoomScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    QGraphicsScene::contextMenuEvent(event);
    QTransform transform;
    QGraphicsItem *item = itemAt(event->scenePos(), transform);
    if (item->zValue() < -99999) { // @todo_P: tableBg?
        QMenu *menu = miscellaneous_menu;
        menu->clear();
        menu->setTitle(tr("Miscellaneous"));

        QMenu *private_pile = menu->addMenu(tr("Private Piles"));

        bool enabled = false;
        foreach (PlayerCardContainer *container, item2player.keys()) {
            const Player *player = item2player.value(container, NULL);
            QStringList piles = player->pileNames();
            if (!piles.isEmpty()) {
                foreach (QString pile_name, piles) {
                    bool add = false;
                    foreach (int id, player->pile(pile_name)) {
                        if (id != Card::S_UNKNOWN_CARD_ID) {
                            add = true;
                            break;
                        }
                    }
                    if (add) {
                        enabled = true;
                        QAction *action
                            = private_pile->addAction(QStringLiteral("%1 %2").arg(ClientInstance->getPlayerName(player->objectName()), Sanguosha->translate(pile_name)));
                        action->setData(QStringLiteral("%1.%2").arg(player->objectName(), pile_name));
                        connect(action, &QAction::triggered, this, &RoomScene::showPlayerCards);
                    }
                }
            }
        }
        private_pile->setEnabled(enabled);
        menu->addSeparator();

        if (ClientInstance->serverInfo()->EnableCheat) {
            QMenu *known_cards = menu->addMenu(tr("Known cards"));

            foreach (PlayerCardContainer *container, item2player.keys()) {
                const Player *player = item2player.value(container, NULL);
                if (player == Self)
                    continue;
                QList<const Card *> known = player->handCards();
                if (known.isEmpty()) {
                    known_cards->addAction(ClientInstance->getPlayerName(player->objectName()))->setEnabled(false);
                } else {
                    QMenu *submenu = known_cards->addMenu(ClientInstance->getPlayerName(player->objectName()));
                    QAction *action = submenu->addAction(tr("View in new dialog"));
                    action->setData(player->objectName());
                    connect(action, &QAction::triggered, this, &RoomScene::showPlayerCards);

                    submenu->addSeparator();
                    foreach (const Card *card, known) {
                        const CardDescriptor &engine_card = Sanguosha->cardDescriptor(card->id());
                        submenu->addAction(G_ROOM_SKIN.getCardSuitPixmap(engine_card.suit), engine_card.fullName());
                    }
                }
            }
            menu->addSeparator();
        }

        QAction *distance = menu->addAction(tr("View distance"));
        connect(distance, &QAction::triggered, this, &RoomScene::viewDistance);
        QAction *discard = menu->addAction(tr("View Discard pile"));
        connect(discard, &QAction::triggered, this, &RoomScene::toggleDiscards);

        menu->popup(event->screenPos());
    } else if (ClientInstance->serverInfo()->FreeChoose && (arrange_button != nullptr)) {
        QGraphicsObject *obj = item->toGraphicsObject();
        if ((obj != nullptr) && (Sanguosha->general(obj->objectName()) != nullptr)) {
            to_change = qobject_cast<CardItem *>(obj);
            change_general_menu->popup(event->screenPos());
        }
    }
}

void RoomScene::chooseGeneral(const QStringList &generals, bool single_result, bool can_convert)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Audio::playSystemAudioEffect(QStringLiteral("prelude"));

    if (ClientInstance->serverInfo()->GameMode->category() == QSanguosha::ModeHegemony && ClientInstance->serverInfo()->GeneralsPerPlayer >= 2
        && !Self->hasFlag(QStringLiteral("Pingyi_Choose")) && !generals.isEmpty()) {
        m_chooseGeneralBox->chooseGeneral(generals, false, single_result, QString(), nullptr, can_convert);
    } else {
        QDialog *dialog = nullptr;
        if (generals.isEmpty())
            dialog = new FreeChooseDialog(main_window);
        else {
            dialog = new ChooseGeneralDialog(generals, main_window);
        }

        delete m_choiceDialog;
        m_choiceDialog = dialog;
    }
}

void RoomScene::chooseSuit(const QStringList &suits)
{
    QDialog *dialog = new QDialog;
    QVBoxLayout *layout = new QVBoxLayout;

    foreach (QString suit, suits) {
        CommandLinkDoubleClickButton *button = new CommandLinkDoubleClickButton;
        button->setIcon(QIcon(QStringLiteral("image/system/suit/%1.png").arg(suit)));
        button->setText(Sanguosha->translate(suit));
        button->setObjectName(suit);

        layout->addWidget(button);

        connect(button, SIGNAL(double_clicked()), ClientInstance, SLOT(onPlayerChooseSuit()));
        connect(button, &CommandLinkDoubleClickButton::double_clicked, dialog, &QDialog::accept);
    }

    connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseSuit()));

    dialog->setObjectName(QStringLiteral("."));
    dialog->setWindowTitle(tr("Please choose a suit"));
    dialog->setLayout(layout);
    delete m_choiceDialog;
    m_choiceDialog = dialog;
}

namespace {
int preferredColumnCount(int x)
{
    int x2 = sqrt(x);
    return sqrt(x + x2);
}
} // namespace

void RoomScene::chooseKingdom(const QStringList &kingdoms)
{
    QDialog *dialog = new QDialog;
    // QVBoxLayout *layout = new QVBoxLayout;
    QGridLayout *layout = new QGridLayout;
    int columnCount = preferredColumnCount(kingdoms.length());
    int currentColumn = 0;
    int currentRow = 0;

    foreach (QString kingdom, kingdoms) {
        CommandLinkDoubleClickButton *button = new CommandLinkDoubleClickButton;
        QPixmap kingdom_pixmap(QStringLiteral("image/kingdom/icon/%1.png").arg(kingdom));
        QIcon kingdom_icon(kingdom_pixmap);

        button->setIcon(kingdom_icon);
        button->setIconSize(kingdom_pixmap.size());
        button->setText(Sanguosha->translate(kingdom));
        button->setObjectName(kingdom);

        layout->addWidget(button, currentRow, currentColumn++);
        if (currentColumn >= columnCount) {
            currentColumn = 0;
            currentRow += 1;
        }

        connect(button, SIGNAL(double_clicked()), ClientInstance, SLOT(onPlayerChooseKingdom()));
        connect(button, &CommandLinkDoubleClickButton::double_clicked, dialog, &QDialog::accept);
    }

    connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseKingdom()));

    dialog->setObjectName(QStringLiteral("."));
    dialog->setWindowTitle(tr("Please choose a kingdom"));
    dialog->setLayout(layout);
    delete m_choiceDialog;
    m_choiceDialog = dialog;
}

void RoomScene::chooseOption(const QString &skillName, const QStringList &options)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Audio::playSystemAudioEffect(QStringLiteral("pop-up"));

    m_chooseOptionsBox->setSkillName(skillName);
    m_chooseOptionsBox->chooseOption(options);
}

void RoomScene::chooseCard(const Player *player, const QString &flags, const QString &reason, bool handcard_visible, QSanguosha::HandlingMethod method,
                           const QList<int> &disabled_ids, bool enableEmptyCard)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Audio::playSystemAudioEffect(QStringLiteral("pop-up"));

    m_playerCardBox->chooseCard(Sanguosha->translate(reason), player, flags, handcard_visible, method, disabled_ids, enableEmptyCard);
}

void RoomScene::chooseOrder(QSanProtocol::Game3v3ChooseOrderCommand reason)
{
    QDialog *dialog = new QDialog;
    if (reason == S_REASON_CHOOSE_ORDER_SELECT)
        dialog->setWindowTitle(tr("The order who first choose general"));
    else if (reason == S_REASON_CHOOSE_ORDER_TURN)
        dialog->setWindowTitle(tr("The order who first in turn"));

    QLabel *prompt = new QLabel(tr("Please select the order"));
    OptionButton *warm_button = new OptionButton(QStringLiteral("image/system/3v3/warm.png"), tr("Warm"));
    warm_button->setObjectName(QStringLiteral("warm"));
    OptionButton *cool_button = new OptionButton(QStringLiteral("image/system/3v3/cool.png"), tr("Cool"));
    cool_button->setObjectName(QStringLiteral("cool"));

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(warm_button);
    hlayout->addWidget(cool_button);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(prompt);
    layout->addLayout(hlayout);
    dialog->setLayout(layout);

    connect(warm_button, SIGNAL(double_clicked()), ClientInstance, SLOT(onPlayerChooseOrder()));
    connect(cool_button, SIGNAL(double_clicked()), ClientInstance, SLOT(onPlayerChooseOrder()));
    connect(warm_button, &OptionButton::double_clicked, dialog, &QDialog::accept);
    connect(cool_button, &OptionButton::double_clicked, dialog, &QDialog::accept);
    connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseOrder()));
    delete m_choiceDialog;
    m_choiceDialog = dialog;
}

void RoomScene::chooseRole(const QString &scheme, const QStringList &roles)
{
    QDialog *dialog = new QDialog;
    dialog->setWindowTitle(tr("Select role in 3v3 mode"));

    QLabel *prompt = new QLabel(tr("Please select a role"));
    QVBoxLayout *layout = new QVBoxLayout;

    layout->addWidget(prompt);

    static QMap<QString, QString> jargon;
    if (jargon.isEmpty()) {
        jargon[QStringLiteral("lord")] = tr("Warm leader");
        jargon[QStringLiteral("loyalist")] = tr("Warm guard");
        jargon[QStringLiteral("renegade")] = tr("Cool leader");
        jargon[QStringLiteral("rebel")] = tr("Cool guard");

        jargon[QStringLiteral("leader1")] = tr("Leader of Team 1");
        jargon[QStringLiteral("guard1")] = tr("Guard of Team 1");
        jargon[QStringLiteral("leader2")] = tr("Leader of Team 2");
        jargon[QStringLiteral("guard2")] = tr("Guard of Team 2");
    }

    foreach (QString role, roles) {
        CommandLinkDoubleClickButton *button = new CommandLinkDoubleClickButton(jargon[role]);
        if (scheme == QStringLiteral("AllRoles"))
            button->setIcon(QIcon(QStringLiteral("image/system/roles/%1.png").arg(role)));
        layout->addWidget(button);
        button->setObjectName(role);
        connect(button, SIGNAL(double_clicked()), ClientInstance, SLOT(onPlayerChooseRole3v3()));
        connect(button, &CommandLinkDoubleClickButton::double_clicked, dialog, &QDialog::accept);
    }

    CommandLinkDoubleClickButton *abstain_button = new CommandLinkDoubleClickButton(tr("Abstain"));
    connect(abstain_button, &CommandLinkDoubleClickButton::double_clicked, dialog, &QDialog::reject);
    layout->addWidget(abstain_button);

    dialog->setObjectName(QStringLiteral("abstain"));
    connect(dialog, SIGNAL(rejected()), ClientInstance, SLOT(onPlayerChooseRole3v3()));

    dialog->setLayout(layout);
    delete m_choiceDialog;
    m_choiceDialog = dialog;
}

void RoomScene::chooseDirection()
{
    QDialog *dialog = new QDialog;
    dialog->setWindowTitle(tr("Please select the direction"));

    QLabel *prompt = new QLabel(dialog->windowTitle());

    OptionButton *cw_button = new OptionButton(QStringLiteral("image/system/3v3/cw.png"), tr("CW"));
    cw_button->setObjectName(QStringLiteral("cw"));

    OptionButton *ccw_button = new OptionButton(QStringLiteral("image/system/3v3/ccw.png"), tr("CCW"));
    ccw_button->setObjectName(QStringLiteral("ccw"));

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addWidget(cw_button);
    hlayout->addWidget(ccw_button);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(prompt);
    layout->addLayout(hlayout);
    dialog->setLayout(layout);

    dialog->setObjectName(QStringLiteral("ccw"));
    connect(ccw_button, &OptionButton::double_clicked, ClientInstance, &Client::onPlayerMakeChoice);
    connect(ccw_button, &OptionButton::double_clicked, dialog, &QDialog::accept);
    connect(cw_button, &OptionButton::double_clicked, ClientInstance, &Client::onPlayerMakeChoice);
    connect(cw_button, &OptionButton::double_clicked, dialog, &QDialog::accept);
    connect(dialog, &QDialog::rejected, ClientInstance, &Client::onPlayerMakeChoice);
    delete m_choiceDialog;
    m_choiceDialog = dialog;
}

void RoomScene::chooseTriggerOrder(const QVariantList &options, bool optional)
{
    QApplication::alert(main_window);
    if (!main_window->isActiveWindow())
        Audio::playSystemAudioEffect(QStringLiteral("pop-up"));

    m_chooseTriggerOrderBox->chooseOption(options, optional);
}

void RoomScene::toggleDiscards()
{
    CardOverview *overview = new CardOverview;
    overview->setWindowTitle(tr("Discarded pile"));
    overview->loadFromList(ClientInstance->discardPile());
    overview->show();
}

GenericCardContainer *RoomScene::_getGenericCardContainer(QSanguosha::Place place, Player *player)
{
    if (place == QSanguosha::PlaceDiscardPile || place == QSanguosha::PlaceJudge || place == QSanguosha::PlaceDrawPile || place == QSanguosha::PlaceTable)
        return m_tablePile;
    // @todo: AG must be a pile with name rather than simply using the name special...
    else if (player == nullptr && place == QSanguosha::PlaceSpecial)
        return pileContainer;
    else if (player == Self)
        return dashboard;
    else if (player != nullptr)
        return name2photo.value(player->objectName(), NULL);
    else
        Q_ASSERT(false);
    return nullptr;
}

bool RoomScene::_shouldIgnoreDisplayMove(LegacyCardsMoveStruct &movement)
{
    QSanguosha::Place from = movement.from_place;
    QSanguosha::Place to = movement.to_place;
    QString from_pile = movement.from_pile_name;
    QString to_pile = movement.to_pile_name;
    if ((from == QSanguosha::PlaceSpecial && !from_pile.isEmpty() && from_pile.startsWith(QLatin1Char('#')))
        || (to == QSanguosha::PlaceSpecial && !to_pile.isEmpty() && to_pile.startsWith(QLatin1Char('#'))))
        return true;
    else {
        static QList<QSanguosha::Place> ignore_place;
        if (ignore_place.isEmpty())
            ignore_place << QSanguosha::PlaceDiscardPile << QSanguosha::PlaceTable << QSanguosha::PlaceJudge;
        return movement.reason.m_skillName != QStringLiteral("manjuan") && ignore_place.contains(from) && ignore_place.contains(to);
    }
}

bool RoomScene::_processCardsMove(LegacyCardsMoveStruct &move, bool isLost)
{
    _MoveCardsClassifier cls(move);
    // delayed trick processed;
    if (move.from_place == QSanguosha::PlaceDelayedTrick && move.to_place == QSanguosha::PlaceTable) {
        if (isLost)
            m_move_cache[cls] = move;
        return true;
    }
    LegacyCardsMoveStruct tmpMove = m_move_cache.value(cls, LegacyCardsMoveStruct());
    if (tmpMove.from_place != QSanguosha::PlaceUnknown) {
        move.from = tmpMove.from;
        move.from_place = tmpMove.from_place;
        move.from_pile_name = tmpMove.from_pile_name;
    }
    if (!isLost)
        m_move_cache.remove(cls);
    return false;
}

void RoomScene::getCards(int moveId, QList<LegacyCardsMoveStruct> card_moves)
{
    int count = 0;
    for (int i = 0; i < card_moves.size(); i++) {
        LegacyCardsMoveStruct &movement = card_moves[i];
        bool skipMove = _processCardsMove(movement, false);
        if (skipMove)
            continue;
        if (_shouldIgnoreDisplayMove(movement))
            continue;
        card_container->m_currentPlayer = (Player *)movement.to;
        GenericCardContainer *to_container = _getGenericCardContainer(movement.to_place, movement.to);
        QList<CardItem *> cards = _m_cardsMoveStash[moveId][count];
        count++;
        for (int j = 0; j < cards.size(); j++) {
            CardItem *card = cards[j];
            card->setFlag(QGraphicsItem::ItemIsMovable, false);
            CardMoveReason reason = movement.reason;
            if (!reason.m_skillName.isEmpty() && (movement.from != nullptr) && movement.to_place != QSanguosha::PlaceHand && movement.to_place != QSanguosha::PlaceSpecial
                && movement.to_place != QSanguosha::PlaceEquip && movement.to_place != QSanguosha::PlaceDelayedTrick) {
                Player *target = ClientInstance->findPlayer(movement.from->objectName());
                if (!reason.m_playerId.isEmpty() && reason.m_playerId != movement.from->objectName())
                    target = ClientInstance->findPlayer(reason.m_playerId);

                if ((target != nullptr) && target->haveShownSkill(reason.m_skillName))
                    card->showAvatar(target->general());
            }

            int card_id = card->getId();
            if (!card_moves[i].card_ids.contains(card_id)) {
                card->setVisible(false);
                card->deleteLater();
                cards.removeAt(j);
                j--;
            } else
                card->setEnabled(true);
            card->setFootnote(_translateMovement(movement));
            card->hideFootnote();
        }
        bringToFront(to_container);
        to_container->addCardItems(cards, movement);
        keepGetCardLog(movement);
    }
    _m_cardsMoveStash[moveId].clear();
}

void RoomScene::loseCards(int moveId, QList<LegacyCardsMoveStruct> card_moves)
{
    for (int i = 0; i < card_moves.size(); i++) {
        LegacyCardsMoveStruct &movement = card_moves[i];
        bool skipMove = _processCardsMove(movement, true);
        if (skipMove)
            continue;
        if (_shouldIgnoreDisplayMove(movement))
            continue;
        card_container->m_currentPlayer = (Player *)movement.to;
        GenericCardContainer *from_container = _getGenericCardContainer(movement.from_place, movement.from);
        QList<CardItem *> cards = from_container->removeCardItems(movement.card_ids, movement.from_place);
        foreach (CardItem *card, cards)
            card->setEnabled(false);

        _m_cardsMoveStash[moveId].append(cards);
        keepLoseCardLog(movement);
    }
}

QString RoomScene::_translateMovement(const LegacyCardsMoveStruct &move)
{
    CardMoveReason reason = move.reason;
    if (reason.m_reason == QSanguosha::MoveReasonUnknown)
        return QString();

    Photo *srcPhoto = name2photo[reason.m_playerId];
    Photo *dstPhoto = name2photo[reason.m_targetId];
    QString playerName;
    QString targetName;

    if (srcPhoto != nullptr)
        playerName = ClientInstance->getPlayerFootNoteName(srcPhoto->getPlayer());
    else if (reason.m_playerId == Self->objectName())
        playerName = QStringLiteral("%1(%2)").arg(ClientInstance->getPlayerFootNoteName(Self), Sanguosha->translate(QStringLiteral("yourself")));

    if (dstPhoto != nullptr)
        targetName = Sanguosha->translate(QStringLiteral("use upon")).append(ClientInstance->getPlayerFootNoteName(dstPhoto->getPlayer()));
    else if (reason.m_targetId == Self->objectName())
        targetName = QStringLiteral("%1%2(%3)")
                         .arg(Sanguosha->translate(QStringLiteral("use upon")))
                         .arg(ClientInstance->getPlayerFootNoteName(Self))
                         .arg(Sanguosha->translate(QStringLiteral("yourself")));

    QString result(playerName + targetName);
    result.append(Sanguosha->translate(reason.m_eventName));
    result.append(Sanguosha->translate(reason.m_skillName));
    if ((reason.m_reason & QSanguosha::MoveReasonBasicMask) == QSanguosha::MoveReasonUse && reason.m_skillName.isEmpty()) {
        result.append(Sanguosha->translate(QStringLiteral("use")));
    } else if ((reason.m_reason & QSanguosha::MoveReasonBasicMask) == QSanguosha::MoveReasonResponse) {
        if (reason.m_reason == QSanguosha::MoveReasonRetrial)
            result.append(Sanguosha->translate(QStringLiteral("retrial")));
        else if (reason.m_skillName.isEmpty())
            result.append(Sanguosha->translate(QStringLiteral("response")));
    } else if ((reason.m_reason & QSanguosha::MoveReasonBasicMask) == QSanguosha::MoveReasonDiscard) {
        if (reason.m_reason == QSanguosha::MoveReasonRuleDiscard)
            result.append(Sanguosha->translate(QStringLiteral("discard")));
        else if (reason.m_reason == QSanguosha::MoveReasonThrow)
            result.append(Sanguosha->translate(QStringLiteral("throw")));
        else if (reason.m_reason == QSanguosha::MoveReasonChangeEquip)
            result.append(Sanguosha->translate(QStringLiteral("change equip")));
        else if (reason.m_reason == QSanguosha::MoveReasonDismantle)
            result.append(Sanguosha->translate(QStringLiteral("throw")));
    } else if (reason.m_reason == QSanguosha::MoveReasonRecast) {
        result.append(Sanguosha->translate(QStringLiteral("recast")));
    } else if (reason.m_reason == QSanguosha::MoveReasonPindian) {
        result.append(Sanguosha->translate(QStringLiteral("pindian")));
    } else if ((reason.m_reason & QSanguosha::MoveReasonBasicMask) == QSanguosha::MoveReasonShow) {
        if (reason.m_reason == QSanguosha::MoveReasonJudge)
            result.append(Sanguosha->translate(QStringLiteral("judge")));
        else if (reason.m_reason == QSanguosha::MoveReasonTurnover) //ignore turnover from bottom...
            result.append(Sanguosha->translate(QStringLiteral("turnover")));
        else if (reason.m_reason == QSanguosha::MoveReasonDemonstrate)
            result.append(Sanguosha->translate(QStringLiteral("show")));
        else if (reason.m_reason == QSanguosha::MoveReasonPreview)
            result.append(Sanguosha->translate(QStringLiteral("preview")));
    } else if ((reason.m_reason & QSanguosha::MoveReasonBasicMask) == QSanguosha::MoveReasonPut) {
        if (reason.m_reason == QSanguosha::MoveReasonPut) {
            result.append(Sanguosha->translate(QStringLiteral("put")));
            if (move.to_place == QSanguosha::PlaceDiscardPile)
                result.append(Sanguosha->translate(QStringLiteral("discardPile")));
            else if (move.to_place == QSanguosha::PlaceDrawPile)
                result.append(Sanguosha->translate(QStringLiteral("drawPileTop")));
        } else if (reason.m_reason == QSanguosha::MoveReasonNaturalEnter) {
            result.append(Sanguosha->translate(QStringLiteral("enter")));
            if (move.to_place == QSanguosha::PlaceDiscardPile)
                result.append(Sanguosha->translate(QStringLiteral("discardPile")));
            else if (move.to_place == QSanguosha::PlaceDrawPile)
                result.append(Sanguosha->translate(QStringLiteral("drawPileTop")));
        } else if (reason.m_reason == QSanguosha::MoveReasonJudgeDone) {
            result.append(Sanguosha->translate(QStringLiteral("judgedone")));
        } else if (reason.m_reason == QSanguosha::MoveReasonRemoveFromPile) {
            result.append(Sanguosha->translate(QStringLiteral("backinto")));
        }
    }
    return result;
}

void RoomScene::keepLoseCardLog(const LegacyCardsMoveStruct &move)
{
    if ((move.from != nullptr) && move.to_place == QSanguosha::PlaceDrawPile) {
        if (move.reason.m_reason == QSanguosha::MoveReasonPut && move.reason.m_skillName == QStringLiteral("luck_card"))
            return;
        QString from_general = move.from->objectName();
        QList<int> open_ids = move.card_ids;
        open_ids.removeAll(Card::S_UNKNOWN_CARD_ID);
        int hidden_num = move.card_ids.length() - open_ids.length();
        if (hidden_num == move.card_ids.length())
            log_box->appendLog(QStringLiteral("#PutCard"), from_general, QStringList(), QString(), QString::number(hidden_num));
        else if (hidden_num == 0)
            log_box->appendLog(QStringLiteral("$PutCard"), from_general, QStringList(), IntList2StringList(open_ids).join(QStringLiteral("+")));
        else
            log_box->appendLog(QStringLiteral("$PutNCards"), from_general, QStringList(), IntList2StringList(open_ids).join(QStringLiteral("+")), QString::number(hidden_num));
    }
}

void RoomScene::keepGetCardLog(const LegacyCardsMoveStruct &move)
{
    if (move.card_ids.isEmpty())
        return;
    if ((move.to != nullptr) && (move.to_place == QSanguosha::PlaceHand || move.to_place == QSanguosha::PlaceEquip || move.to_place == QSanguosha::PlaceSpecial)
        && move.from_place != QSanguosha::PlaceDrawPile) {
        foreach (QString flag, move.to->flagList())
            if (flag.endsWith(QStringLiteral("_InTempMoving")))
                return;
    }

    // private pile
    if (move.to_place == QSanguosha::PlaceSpecial && !move.to_pile_name.isNull() && !move.to_pile_name.startsWith(QLatin1Char('#'))) {
        QList<int> open_ids = move.card_ids;
        open_ids.removeAll(Card::S_UNKNOWN_CARD_ID);
        int hidden_num = move.card_ids.length() - open_ids.length();
        if (hidden_num == move.card_ids.length())
            log_box->appendLog(QStringLiteral("#RemoveFromGame"), QString(), QStringList(), QString(), move.to_pile_name, QString::number(hidden_num));
        else if (hidden_num == 0)
            log_box->appendLog(QStringLiteral("$AddToPile"), QString(), QStringList(), IntList2StringList(open_ids).join(QStringLiteral("+")), move.to_pile_name);
        else
            log_box->appendLog(QStringLiteral("$RemoveNCardsFromGame"), QString(), QStringList(), IntList2StringList(open_ids).join(QStringLiteral("+")), move.to_pile_name,
                               QString::number(hidden_num));
    }
    if (move.from_place == QSanguosha::PlaceSpecial && (move.to != nullptr) && move.reason.m_reason == QSanguosha::MoveReasonExchangeFromPile) {
        bool hidden = (move.card_ids.contains(Card::S_UNKNOWN_CARD_ID));
        if (!hidden)
            log_box->appendLog(QStringLiteral("$GotCardFromPile"), move.to->objectName(), QStringList(), IntList2StringList(move.card_ids).join(QStringLiteral("+")),
                               move.from_pile_name);
        else
            log_box->appendLog(QStringLiteral("#GotNCardFromPile"), move.to->objectName(), QStringList(), QString(), move.from_pile_name, QString::number(move.card_ids.length()));
    }
    //DrawNCards
    if (move.from_place == QSanguosha::PlaceDrawPile && move.to_place == QSanguosha::PlaceHand) {
        QString to_general = move.to->objectName();
        bool hidden = (move.card_ids.contains(Card::S_UNKNOWN_CARD_ID));
        if (!hidden)
            log_box->appendLog(QStringLiteral("$DrawCards"), to_general, QStringList(), IntList2StringList(move.card_ids).join(QStringLiteral("+")),
                               QString::number(move.card_ids.length()));
        else
            log_box->appendLog(QStringLiteral("#DrawNCards"), to_general, QStringList(), QString(), QString::number(move.card_ids.length()));
    }
    if ((move.from_place == QSanguosha::PlaceTable || move.from_place == QSanguosha::PlaceJudge) && move.to_place == QSanguosha::PlaceHand
        && move.reason.m_reason != QSanguosha::MoveReasonPreview) {
        QString to_general = move.to->objectName();
        QList<int> ids = move.card_ids;
        ids.removeAll(Card::S_UNKNOWN_CARD_ID);
        if (!ids.isEmpty()) {
            QString card_str = IntList2StringList(ids).join(QStringLiteral("+"));
            log_box->appendLog(QStringLiteral("$GotCardBack"), to_general, QStringList(), card_str);
        }
    }
    if (move.from_place == QSanguosha::PlaceDiscardPile && move.to_place == QSanguosha::PlaceHand) {
        QString to_general = move.to->objectName();
        QString card_str = IntList2StringList(move.card_ids).join(QStringLiteral("+"));
        log_box->appendLog(QStringLiteral("$RecycleCard"), to_general, QStringList(), card_str);
    }
    if ((move.from != nullptr) && move.from_place != QSanguosha::PlaceHand && move.from_place != QSanguosha::PlaceJudge && move.to_place != QSanguosha::PlaceDelayedTrick
        && move.to_place != QSanguosha::PlaceJudge && (move.to != nullptr) && move.from != move.to) {
        QString from_general = move.from->objectName();
        QStringList tos;
        tos << move.to->objectName();
        QList<int> open_ids = move.card_ids;
        open_ids.removeAll(Card::S_UNKNOWN_CARD_ID);
        int hidden_num = move.card_ids.length() - open_ids.length();
        if (hidden_num == move.card_ids.length())
            log_box->appendLog(QStringLiteral("#MoveNCards"), from_general, tos, QString(), QString::number(hidden_num));
        else if (hidden_num == 0)
            log_box->appendLog(QStringLiteral("$MoveCard"), from_general, tos, IntList2StringList(open_ids).join(QStringLiteral("+")));
        else
            log_box->appendLog(QStringLiteral("$MoveNCards"), from_general, tos, IntList2StringList(open_ids).join(QStringLiteral("+")), QString::number(hidden_num));
    }
    if (move.from_place == QSanguosha::PlaceHand && move.to_place == QSanguosha::PlaceHand) {
        QString from_general = move.from->objectName();
        QStringList tos;
        tos << move.to->objectName();
        QList<int> open_ids = move.card_ids;
        open_ids.removeAll(Card::S_UNKNOWN_CARD_ID);
        int hidden_num = move.card_ids.length() - open_ids.length();

        if (hidden_num == move.card_ids.length())
            log_box->appendLog(QStringLiteral("#MoveNCards"), from_general, tos, QString(), QString::number(hidden_num));
        else if (hidden_num == 0)
            log_box->appendLog(QStringLiteral("$MoveCard"), from_general, tos, IntList2StringList(open_ids).join(QStringLiteral("+")));
        else
            log_box->appendLog(QStringLiteral("$MoveNCards"), from_general, tos, IntList2StringList(open_ids).join(QStringLiteral("+")), QString::number(hidden_num));
    }
    if ((move.from != nullptr) && (move.to != nullptr)) {
        // both src and dest are player
        QString type;
        if (move.to_place == QSanguosha::PlaceDelayedTrick) {
            if (move.from_place == QSanguosha::PlaceDelayedTrick && move.from != move.to)
                type = QStringLiteral("$LightningMove");
            else
                type = QStringLiteral("$PasteCard");
        }
        if (!type.isNull()) {
            QString from_general = move.from->objectName();
            QStringList tos;
            tos << move.to->objectName();
            log_box->appendLog(type, from_general, tos, QString::number(move.card_ids.first()));
        }
    }
    if ((move.from != nullptr) && (move.to != nullptr) && move.from_place == QSanguosha::PlaceEquip && move.to_place == QSanguosha::PlaceEquip) {
        QString type = QStringLiteral("$Install");
        QString to_general = move.to->objectName();
        foreach (int card_id, move.card_ids)
            log_box->appendLog(type, to_general, QStringList(), QString::number(card_id));
    }
    if (move.reason.m_reason == QSanguosha::MoveReasonTurnover) {
        QString type = (move.reason.m_skillName == QStringLiteral("xunbao")) ? QStringLiteral("$TurnOverBottom") : QStringLiteral("$TurnOver");
        log_box->appendLog(type, move.reason.m_playerId, QStringList(), IntList2StringList(move.card_ids).join(QStringLiteral("+")));
    }
}

void RoomScene::addSelection(const ViewAsSkill *skill, QMenu *menu, const ViewAsSkillSelection *selection, const QStringList &currentChain)
{
    foreach (const ViewAsSkillSelection *nexts, selection->next) {
        QStringList appendedChain = QStringList(currentChain) << nexts->name;
        bool enabled = skill->isSelectionEnabled(appendedChain, Self);
        if (nexts->next.isEmpty()) {
            QAction *action = menu->addAction(Sanguosha->translate(nexts->name));
            action->setObjectName(appendedChain.join(QStringLiteral(".")));
            action->setProperty("skillname", skill->name());
            action->setEnabled(enabled);

            connect(action, &QAction::triggered, action, [this, action]() -> void {
                setCurrentViewAsSkillSelectionChain(action->objectName().split(QStringLiteral(".")));
            });
            connect(action, &QAction::triggered, this, &RoomScene::onSkillActivated);
        } else {
            QMenu *subMenu = menu->addMenu(Sanguosha->translate(nexts->name));
            subMenu->setEnabled(enabled);
            subMenu->setObjectName(appendedChain.join(QStringLiteral(".")));
            if (enabled)
                addSelection(skill, subMenu, nexts, appendedChain);
        }
    }
}

void RoomScene::addSkillButton(const Skill *skill, bool head)
{
    // check duplication
    QSanSkillButton *btn = dashboard->addSkillButton(skill->name(), head);

    if (btn == nullptr)
        return;

    if (btn->getViewAsSkill() != nullptr && (m_replayControl == nullptr)) {
        connect(btn, SIGNAL(skill_activated()), dashboard, SLOT(skillButtonActivated()));
        connect(btn, SIGNAL(skill_activated()), this, SLOT(onSkillActivated()));
        connect(btn, SIGNAL(skill_deactivated()), dashboard, SLOT(skillButtonDeactivated()));
        connect(btn, SIGNAL(skill_deactivated()), this, SLOT(onSkillDeactivated()));
        if (btn->getViewAsSkill()->name() == QStringLiteral("fsu0413fei2zhai"))
            connect(btn, SIGNAL(skill_activated()), dashboard, SLOT(selectAll()));

        const ViewAsSkillSelection *selection = btn->getViewAsSkill()->selections(Self);
        if (selection != nullptr && !selection->next.isEmpty() && (m_replayControl == nullptr)) {
            connect(btn, (void(QSanSkillButton ::*)())(&QSanSkillButton::skill_activated), btn, [this, btn, selection]() -> void {
                // QMenu *menu = new QMenu(this);
                setCurrentViewAsSkillSelectionChain(QStringList());
                QMenu *menu = mainWindow()->findChild<QMenu *>(btn->getViewAsSkill()->name());
                if (menu == nullptr) {
                    menu = new QMenu;
                    menu->setObjectName(btn->getViewAsSkill()->name());
                    menu->setParent(mainWindow());
                } else {
                    menu->clear();
                }

                addSelection(btn->getViewAsSkill(), menu, selection, {selection->name});

                connect(menu, &QMenu::aboutToHide, btn, [this, btn]() -> void {
                    if (currentViewAsSkillSelectionChain().isEmpty()) {
                        dashboard->skillButtonDeactivated();
                        return;
                    }

                    if (btn->getViewAsSkill()->name() == QStringLiteral("qiji"))
                        dashboard->selectAll();
                    if (btn->getViewAsSkill()->name() == QStringLiteral("anyun"))
                        anyunSelectSkill();
                });
                menu->exec();
            });
            disconnect(btn, SIGNAL(skill_activated()), this, SLOT(onSkillActivated()));
        }
    }

    m_skillButtons.append(btn);
}

void RoomScene::acquireSkill(const Player *player, const QString &skill_name, bool head)
{
    QString type = QStringLiteral("#AcquireSkill");
    QString from_general = player->objectName();
    const QString &arg = skill_name;
    log_box->appendLog(type, from_general, QStringList(), QString(), arg);

    if (player == Self)
        addSkillButton(Sanguosha->skill(skill_name), head);
}

void RoomScene::updateSkillButtons()
{
    //check duanchang?
    foreach (const Skill *skill, Self->getHeadSkillList()) {
        if (skill->isLordSkill()
            && (Self->roleString() != QStringLiteral("lord") || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3")
                || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_XMode") || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1")
                || Config.value(QStringLiteral("WithoutLordskill"), false).toBool()))
            continue;

        addSkillButton(skill, true);
    }
    foreach (const Skill *skill, Self->getDeputySkillList()) {
        if (skill->isLordSkill()
            && (Self->roleString() != QStringLiteral("lord") || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3")
                || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_XMode") || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1")
                || Config.value(QStringLiteral("WithoutLordskill"), false).toBool()))
            continue;

        addSkillButton(skill, false);
    }

    if (isHegemonyGameMode(ClientInstance->serverInfo()->GameModeStr)) {
        foreach (QSanSkillButton *button, m_skillButtons) {
            const Skill *skill = button->getSkill();
            button->setEnabled(skill->canPreshow() && !Self->haveShownSkill(skill));
            if (skill->canPreshow() && Self->hasGeneralCardSkill(skill) && !Self->haveShownGeneral()) {
                if (Self->havePreshownSkill(skill->name()))
                    button->setState(QSanButton::S_STATE_DISABLED);
                else
                    button->setState(QSanButton::S_STATE_CANPRESHOW);
            }
        }
    } else {
        // disable all skill buttons
        foreach (QSanSkillButton *button, m_skillButtons)
            button->setEnabled(false);
    }
}

void RoomScene::useSelectedCard()
{
    switch (ClientInstance->getStatus() & Client::ClientStatusBasicMask) {
    case Client::Playing: {
        const Card *card = dashboard->getSelected();
        if (card != nullptr)
            useCard(card);
        break;
    }
    case Client::Responding: {
        const Card *card = dashboard->getSelected();
        if (card != nullptr) {
            if (ClientInstance->getStatus() == Client::Responding) {
                Q_ASSERT(selected_targets.isEmpty());
                selected_targets.clear();
            }
            ClientInstance->onPlayerResponseCard(card, selected_targets);
            prompt_box->disappear();
        }

        dashboard->unselectAll();
        break;
    }
    case Client::AskForShowOrPindian: {
        const Card *card = dashboard->getSelected();
        if (card != nullptr) {
            ClientInstance->onPlayerResponseCard(card);
            prompt_box->disappear();
        }
        dashboard->unselectAll();
        break;
    }
    case Client::Discarding:
    case Client::Exchanging: {
        const Card *card = dashboard->pendingCard();
        if (card != nullptr) {
            ClientInstance->onPlayerDiscardCards(card);
            dashboard->stopPending();
            prompt_box->disappear();
        }
        dashboard->unselectAll();
        break;
    }
    case Client::NotActive: {
        QMessageBox::warning(main_window, tr("Warning"), tr("The OK button should be disabled when client is not active!"));
        return;
    }
    case Client::AskForAG: {
        ClientInstance->onPlayerChooseAG(-1);
        return;
    }
    case Client::ExecDialog: {
        QMessageBox::warning(main_window, tr("Warning"), tr("The OK button should be disabled when client is in executing dialog"));
        return;
    }
    case Client::AskForSkillInvoke: {
        prompt_box->disappear();
        QString skill_name = ClientInstance->getSkillNameToInvoke();
        dashboard->highlightEquip(skill_name, false);
        ClientInstance->onPlayerInvokeSkill(true);
        break;
    }
    case Client::AskForPlayerChoose: {
        ClientInstance->onPlayerChoosePlayer(selected_targets.first());
        prompt_box->disappear();
        break;
    }
    case Client::AskForYiji: {
        const Card *card = dashboard->pendingCard();
        if (card != nullptr) {
            ClientInstance->onPlayerReplyYiji(card, selected_targets.first());
            dashboard->stopPending();
            prompt_box->disappear();
        }
        break;
    }
    case Client::AskForGuanxing: {
        guanxing_box->reply();
        break;
    }
    default:
        break;
    }

    const ViewAsSkill *skill = dashboard->currentSkill();
    if (skill != nullptr)
        dashboard->stopPending();
    else {
        dashboard->retractPileCards(QStringLiteral("wooden_ox"));
        dashboard->retractPileCards(QStringLiteral("chaoren"));
        foreach (const QString &pile, Self->pileNames()) {
            if (pile.startsWith(QStringLiteral("&")))
                dashboard->retractPileCards(pile);
        }
    }
}

void RoomScene::onEnabledChange()
{
    QGraphicsItem *photo = qobject_cast<QGraphicsItem *>(sender());
    if (photo == nullptr)
        return;
    if (photo->isEnabled())
        animations->effectOut(photo);
    else
        animations->sendBack(photo);
}

void RoomScene::useCard(const Card *card)
{
    if (card->face()->targetFixed(Self, card) || card->face()->targetsFeasible(selected_targets, Self, card))
        ClientInstance->onPlayerResponseCard(card, selected_targets);
    enableTargets(nullptr);
}

void RoomScene::callViewAsSkill()
{
    const Card *card = dashboard->pendingCard();
    if (card == nullptr)
        return;

    if (card->face()->isAvailable(Self, card)) {
        // use card
        dashboard->stopPending();
        useCard(card);
    }
}

void RoomScene::cancelViewAsSkill()
{
    dashboard->stopPending();
    Client::Status status = ClientInstance->getStatus();
    updateStatus(status, status);
}

void RoomScene::selectTarget(int order, bool multiple)
{
    if (!multiple)
        unselectAllTargets();

    QGraphicsItem *to_select = nullptr;
    if (order == 0)
        to_select = dashboard;
    else if (order > 0 && order <= photos.length())
        to_select = photos.at(order - 1);

    if (to_select == nullptr)
        return;
    if (!(to_select->isSelected() || ((to_select->flags() & QGraphicsItem::ItemIsSelectable) != 0)))
        return;

    to_select->setSelected(!to_select->isSelected());
}

void RoomScene::selectNextTarget(bool multiple)
{
    if (!multiple)
        unselectAllTargets();

    QList<QGraphicsItem *> targets;
    foreach (Photo *photo, photos) {
        if ((photo->flags() & QGraphicsItem::ItemIsSelectable) != 0)
            targets << photo;
    }

    if ((dashboard->flags() & QGraphicsItem::ItemIsSelectable) != 0)
        targets << dashboard;

    for (int i = 0; i < targets.length(); i++) {
        if (targets.at(i)->isSelected()) {
            for (int j = i + 1; j < targets.length(); j++) {
                if (!targets.at(j)->isSelected()) {
                    targets.at(j)->setSelected(true);
                    return;
                }
            }
        }
    }

    foreach (QGraphicsItem *target, targets) {
        if (!target->isSelected()) {
            target->setSelected(true);
            break;
        }
    }
}

void RoomScene::unselectAllTargets(const QGraphicsItem *except)
{
    if (dashboard != except)
        dashboard->setSelected(false);

    foreach (Photo *photo, photos) {
        if (photo != except && photo->isSelected())
            photo->setSelected(false);
    }
}

void RoomScene::doTimeout()
{
    switch (ClientInstance->getStatus() & Client::ClientStatusBasicMask) {
    case Client::Playing: {
        discard_button->click();
        break;
    }
    case Client::Responding:
    case Client::Discarding:
    case Client::Exchanging:
    case Client::ExecDialog:
    case Client::AskForShowOrPindian: {
        doCancelButton();
        break;
    }
    case Client::AskForPlayerChoose: {
        ClientInstance->onPlayerChoosePlayer(nullptr);
        dashboard->stopPending();
        prompt_box->disappear();
        break;
    }
    case Client::AskForAG: {
        int card_id = card_container->getFirstEnabled();
        if (card_id != -1)
            ClientInstance->onPlayerChooseAG(card_id);
        break;
    }
    case Client::AskForSkillInvoke: {
        cancel_button->click();
        break;
    }
    case Client::AskForYiji: {
        if (cancel_button->isEnabled())
            cancel_button->click();
        else {
            prompt_box->disappear();
            doCancelButton();
        }
        break;
    }
    case Client::AskForGuanxing: {
        ok_button->click();
        break;
    }
    case Client::AskForGongxin: {
        cancel_button->click();
        break;
    }
    case Client::AskForGeneralTaken: {
        break;
    }
    case Client::AskForArrangement: {
        arrange_items << down_generals.mid(0, 3 - arrange_items.length());
        finishArrange();
    }
    default:
        break;
    }
}

void RoomScene::showPromptBox()
{
    bringToFront(prompt_box);
    prompt_box->appear();
}

void RoomScene::updateStatus(Client::Status oldStatus, Client::Status newStatus)
{
    foreach (QSanSkillButton *button, m_skillButtons) {
        Q_ASSERT(button != nullptr);
        const ViewAsSkill *vsSkill = button->getViewAsSkill();
        if (vsSkill != nullptr) {
            QString pattern = ClientInstance->currentCardUsePattern();
            QRegularExpression rx(QRegularExpression::anchoredPattern(QStringLiteral("@@?([_A-Za-z]+)(\\d+)?!?")));
            QSanguosha::CardUseReason reason = QSanguosha::CardUseReasonUnknown;
            if ((newStatus & Client::ClientStatusBasicMask) == Client::Responding) {
                if (newStatus == Client::RespondingUse)
                    reason = QSanguosha::CardUseReasonResponseUse;
                else if (newStatus == Client::Responding || rx.match(pattern).hasMatch())
                    reason = QSanguosha::CardUseReasonResponse;
            } else if (newStatus == Client::Playing)
                reason = QSanguosha::CardUseReasonPlay;
            button->setEnabled(vsSkill->isAvailable(Self, reason, pattern) && !pattern.endsWith(QStringLiteral("!")));
        } else {
            const Skill *skill = button->getSkill();
            if (skill->isLimited() && skill->isCompulsory()) {
                button->setEnabled(Self->mark(skill->name()) > 0);
            } else
                button->setEnabled(false);
        }
    }

    QString highlight_skill_name = ClientInstance->highlight_skill_name;
    if (isHighlightStatus(newStatus) && !highlight_skill_name.isNull())
        highlightSkillButton(highlight_skill_name, true);
    else if (!isHighlightStatus(newStatus) && !highlight_skill_name.isNull()) {
        highlightSkillButton(highlight_skill_name, false);
        ClientInstance->clearHighlightSkillName();
    }

    if (oldStatus == Client::AskForChoice) //regardless of newStatus
        m_chooseOptionsBox->clear();

    switch (newStatus & Client::ClientStatusBasicMask) {
    case Client::NotActive: {
        if (oldStatus == Client::ExecDialog) {
            if (m_choiceDialog != nullptr && m_choiceDialog->isVisible()) {
                m_choiceDialog->hide();
            }
        } else if (oldStatus == Client::AskForGuanxing || oldStatus == Client::AskForGongxin) {
            guanxing_box->clear();
            //Do not clear AG of AmazingGrace after operating Guanxing. such case as Ruizhi and Fengshui
        } else if (oldStatus == Client::AskForTriggerOrder) {
            m_chooseTriggerOrderBox->clear();
        } else if (oldStatus == Client::AskForCardChosen) {
            m_playerCardBox->clear();
        } else if (oldStatus == Client::AskForGeneralTaken) {
            if (m_chooseGeneralBox != nullptr)
                m_chooseGeneralBox->clear();
        }

        prompt_box->disappear();
        ClientInstance->getPromptDoc()->clear();

        dashboard->disableAllCards();
        selected_targets.clear();

        ok_button->setEnabled(false);
        cancel_button->setEnabled(false);
        discard_button->setEnabled(false);

        if (dashboard->currentSkill() != nullptr)
            dashboard->stopPending();

        dashboard->hideProgressBar();

        break;
    }
    case Client::Responding: {
        showPromptBox();

        ok_button->setEnabled(false);
        cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
        discard_button->setEnabled(false);

        QString pattern = ClientInstance->currentCardUsePattern();
        QRegularExpression rx(QRegularExpression::anchoredPattern(QStringLiteral("@@?(\\w+)(-card)?(\\d+)?!?")));
        QRegularExpressionMatch match;
        if ((match = rx.match(pattern)).hasMatch()) {
            QString skill_name = match.capturedTexts().at(1);
            const ViewAsSkill *skill = dynamic_cast<const ViewAsSkill *>(Sanguosha->skill(skill_name));
            if (skill != nullptr) {
                QSanguosha::CardUseReason reason = QSanguosha::CardUseReasonResponse;
                if (newStatus == Client::RespondingUse)
                    reason = QSanguosha::CardUseReasonResponseUse;
                if (!Self->hasFlag(skill_name))
                    Self->setFlag(skill_name);
                bool available = skill->isAvailable(Self, reason, pattern);
                Self->setFlag(QStringLiteral("-") + skill_name);
                if (!available) {
                    ClientInstance->onPlayerResponseCard(nullptr);
                    return;
                }
                if (Self->hasValidSkill(skill_name, true)) {
                    foreach (QSanSkillButton *button, m_skillButtons) {
                        Q_ASSERT(button != nullptr);
                        const ViewAsSkill *vsSkill = button->getViewAsSkill();
                        if (vsSkill != nullptr && vsSkill->name() == skill_name && vsSkill->isAvailable(Self, reason, pattern)) {
                            button->click();
                            break;
                        }
                    }
                }
                dashboard->startPending(skill);
                if ((dynamic_cast<const OneCardViewAsSkill *>(skill) != nullptr) && Config.EnableIntellectualSelection)
                    dashboard->selectOnlyCard();
                else if (skill->name() == QStringLiteral("LingshouOtherVS"))
                    dashboard->selectLingshou();
            }
        } else {
            if (pattern.endsWith(QStringLiteral("!")))
                pattern = pattern.mid(0, pattern.length() - 1);
            response_skill->setPattern(pattern);
            if (newStatus == Client::RespondingForDiscard)
                response_skill->setRequest(QSanguosha::MethodDiscard);
            else if (newStatus == Client::RespondingNonTrigger)
                response_skill->setRequest(QSanguosha::MethodNone);
            else if (newStatus == Client::RespondingUse)
                response_skill->setRequest(QSanguosha::MethodUse);
            else
                response_skill->setRequest(QSanguosha::MethodResponse);
            dashboard->startPending(response_skill);
            if (Config.EnableIntellectualSelection)
                dashboard->selectOnlyCard();
        }
        break;
    }
    case Client::AskForShowOrPindian: {
        showPromptBox();

        ok_button->setEnabled(false);
        cancel_button->setEnabled(false);
        discard_button->setEnabled(false);

        QString pattern = ClientInstance->currentCardUsePattern();
        showorpindian_skill->setPattern(pattern);
        dashboard->startPending(showorpindian_skill);

        break;
    }
    case Client::Playing: {
        dashboard->enableCards();
        bringToFront(dashboard);
        ok_button->setEnabled(false);
        cancel_button->setEnabled(false);
        discard_button->setEnabled(true);
        break;
    }
    case Client::Discarding:
    case Client::Exchanging: {
        showPromptBox();

        ok_button->setEnabled(false);
        cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
        discard_button->setEnabled(false);

        discard_skill->setNum(ClientInstance->discard_num);
        discard_skill->setMinNum(ClientInstance->min_num);
        discard_skill->setIncludeEquip(ClientInstance->m_canDiscardEquip);
        discard_skill->setIsDiscard(newStatus != Client::Exchanging);
        dashboard->startPending(discard_skill);
        break;
    }
    case Client::ExecDialog: {
        if (m_choiceDialog != nullptr) {
            m_choiceDialog->setParent(main_window, Qt::Dialog);
            m_choiceDialog->show();
            ok_button->setEnabled(false);
            cancel_button->setEnabled(true);
            discard_button->setEnabled(false);
        }
        break;
    }
    case Client::AskForSkillInvoke: {
        QString skill_name = ClientInstance->getSkillNameToInvoke();
        dashboard->highlightEquip(skill_name, true);
        foreach (QSanSkillButton *button, m_skillButtons) {
            if (button->getSkill()->name() == skill_name) {
                if (button->getStyle() == QSanSkillButton::S_STYLE_TOGGLE && button->isEnabled()) {
                    if (button->isDown()) {
                        ClientInstance->onPlayerInvokeSkill(true);
                        return;
                    }
                }
            }
        }

        showPromptBox();
        ok_button->setEnabled(true);
        cancel_button->setEnabled(true);
        discard_button->setEnabled(false);
        break;
    }
    case Client::AskForPlayerChoose: {
        showPromptBox();
        ok_button->setEnabled(false);
        cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
        discard_button->setEnabled(false);

        choose_skill->setPlayerNames(ClientInstance->players_to_choose);
        dashboard->startPending(choose_skill);

        break;
    }
    case Client::AskForAG: {
        dashboard->disableAllCards();

        ok_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
        cancel_button->setEnabled(false);
        discard_button->setEnabled(false);

        card_container->startChoose();

        break;
    }
    case Client::AskForYiji: {
        ok_button->setEnabled(false);
        cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
        discard_button->setEnabled(false);

        QStringList yiji_info = ClientInstance->currentCardUsePattern().split(QStringLiteral("="));
        yiji_skill->setCards(yiji_info.at(1));
        yiji_skill->setMaxNum(yiji_info.first().toInt());
        yiji_skill->setPlayerNames(yiji_info.last().split(QStringLiteral("+")));
        dashboard->startPending(yiji_skill);

        showPromptBox();

        break;
    }
    case Client::AskForGuanxing: {
        ok_button->setEnabled(true);
        cancel_button->setEnabled(false);
        discard_button->setEnabled(false);

        break;
    }
    case Client::AskForGongxin: {
        ok_button->setEnabled(false);
        cancel_button->setEnabled(ClientInstance->m_isDiscardActionRefusable);
        discard_button->setEnabled(false);

        break;
    }
    case Client::AskForGeneralTaken:
    case Client::AskForChoice:
    case Client::AskForTriggerOrder:
    case Client::AskForCardChosen:
    case Client::AskForArrangement: {
        ok_button->setEnabled(false);
        cancel_button->setEnabled(false);
        discard_button->setEnabled(false);

        break;
    }
    }
    if (newStatus != oldStatus && newStatus != Client::Playing && newStatus != Client::NotActive)
        QApplication::alert(QApplication::focusWidget());

    if (ClientInstance->serverInfo()->OperationTimeout == 0)
        return;

    // do timeout
    if (newStatus != Client::NotActive && newStatus != oldStatus) {
        QApplication::alert(main_window);
        connect(dashboard, &Dashboard::progressBarTimedOut, this, &RoomScene::doTimeout);
        dashboard->showProgressBar(ClientInstance->getCountdown());
    }
}

void RoomScene::onSkillDeactivated()
{
    const ViewAsSkill *current = dashboard->currentSkill();
    if (current != nullptr)
        cancel_button->click();
}

void RoomScene::onSkillActivated()
{
    QSanSkillButton *button = qobject_cast<QSanSkillButton *>(sender());
    const ViewAsSkill *skill = nullptr;

    if (button != nullptr)
        skill = button->getViewAsSkill();
    else {
        QAction *action = qobject_cast<QAction *>(sender());

        if (action != nullptr)
            skill = dynamic_cast<const ViewAsSkill *>(Sanguosha->skill(action->property("skillname").toString()));
        else {
            QDialog *dialog = qobject_cast<QDialog *>(sender());
            if (dialog != nullptr)
                skill = dynamic_cast<const ViewAsSkill *>(Sanguosha->skill(dialog->objectName()));
        }
    }

    if (skill != nullptr) {
        dashboard->startPending(skill);
        cancel_button->setEnabled(true);

        const Card *card = dashboard->pendingCard();
        if ((card != nullptr) && card->face()->targetFixed(Self, card) && card->face()->isAvailable(Self, card) && !Self->hasFlag(QStringLiteral("Global_InstanceUse_Failed"))) {
            bool instance_use = (dynamic_cast<const ZeroCardViewAsSkill *>(skill) != nullptr);
            if (!instance_use) {
                QList<const Card *> cards;
                cards << Self->handCards() << Self->equipCards();

                foreach (const QString &name, dashboard->getPileExpanded()) {
                    IdSet pile = Self->pile(name);
                    foreach (int id, pile)
                        cards << ClientInstance->card(id);
                }

                foreach (const Card *c, cards) {
                    if (skill->viewFilter(QList<const Card *>(), c, Self, currentViewAsSkillSelectionChain()))
                        return;
                }
                instance_use = true;
            }
            if (instance_use)
                useSelectedCard();
        } else if ((dynamic_cast<const OneCardViewAsSkill *>(skill) != nullptr) && Config.EnableIntellectualSelection)
            dashboard->selectOnlyCard(ClientInstance->getStatus() == Client::Playing);
    }
}

void RoomScene::updateTrustButton()
{
    if (ClientInstance->getReplayer() == nullptr) {
        bool trusting = Self->getState() == QStringLiteral("trust");
        trust_button->update();
        dashboard->setTrust(trusting);
    }
}

void RoomScene::doOkButton()
{
    if (!ok_button->isEnabled())
        return;

    //for highlighting skill button
    QString skill_name = ClientInstance->highlight_skill_name;
    if (!skill_name.isNull() && isHighlightStatus(ClientInstance->getStatus())) {
        highlightSkillButton(skill_name, false);
        ClientInstance->clearHighlightSkillName();
    }

    if (card_container->retained())
        card_container->clear();
    useSelectedCard();
}

void RoomScene::doCancelButton()
{
    //for highlignting skill button
    QString h_skill_name = ClientInstance->highlight_skill_name;
    if (!h_skill_name.isNull() && isHighlightStatus(ClientInstance->getStatus())) {
        highlightSkillButton(h_skill_name, false);
        ClientInstance->clearHighlightSkillName();
    }

    if (card_container->retained())
        card_container->clear();
    switch (ClientInstance->getStatus() & Client::ClientStatusBasicMask) {
    case Client::Playing: {
        dashboard->skillButtonDeactivated();
        const ViewAsSkill *skill = dashboard->currentSkill();
        dashboard->unselectAll();
        if (skill != nullptr)
            cancelViewAsSkill();
        else
            dashboard->stopPending();
        dashboard->enableCards();
        break;
    }
    case Client::Responding: {
        dashboard->skillButtonDeactivated();
        QString pattern = ClientInstance->currentCardUsePattern();
        if (pattern.isEmpty())
            return;

        dashboard->unselectAll();

        if (!pattern.startsWith(QStringLiteral("@"))) {
            const ViewAsSkill *skill = dashboard->currentSkill();
            if (dynamic_cast<const ResponseSkill *>(skill) == nullptr) {
                cancelViewAsSkill();
                break;
            }
        }

        ClientInstance->onPlayerResponseCard(nullptr);
        prompt_box->disappear();
        dashboard->stopPending();
        break;
    }
    case Client::AskForShowOrPindian: {
        dashboard->unselectAll();
        ClientInstance->onPlayerResponseCard(nullptr);
        prompt_box->disappear();
        dashboard->stopPending();
        break;
    }
    case Client::Discarding:
    case Client::Exchanging: {
        dashboard->unselectAll();
        dashboard->stopPending();
        ClientInstance->onPlayerDiscardCards(nullptr);
        prompt_box->disappear();
        break;
    }
    case Client::ExecDialog: {
        m_choiceDialog->reject();
        break;
    }
    case Client::AskForSkillInvoke: {
        QString skill_name = ClientInstance->getSkillNameToInvoke();
        dashboard->highlightEquip(skill_name, false);
        ClientInstance->onPlayerInvokeSkill(false);
        prompt_box->disappear();

        break;
    }
    case Client::AskForYiji: {
        dashboard->stopPending();
        ClientInstance->onPlayerReplyYiji(nullptr, nullptr);
        prompt_box->disappear();
        break;
    }
    case Client::AskForPlayerChoose: {
        dashboard->stopPending();
        ClientInstance->onPlayerChoosePlayer(nullptr);
        prompt_box->disappear();
        break;
    }
    case Client::AskForGongxin: {
        ClientInstance->onPlayerReplyGongxin();
        card_container->clear();
        break;
    }
    default:
        break;
    }
}

void RoomScene::doDiscardButton()
{
    dashboard->stopPending();
    dashboard->unselectAll();

    if (card_container->retained())
        card_container->clear();
    if (ClientInstance->getStatus() == Client::Playing)
        ClientInstance->onPlayerResponseCard(nullptr);
}

void RoomScene::hideAvatars()
{
    if (control_panel != nullptr)
        control_panel->hide();
}

void RoomScene::startInXs()
{
    if (add_robot != nullptr)
        add_robot->hide();
    if (fill_robots != nullptr)
        fill_robots->hide();
    if (return_to_main_menu != nullptr)
        return_to_main_menu->hide();
    time_label_widget->startCounting();
}

void RoomScene::changeTableBg()
{
    QRectF displayRegion = sceneRect();
    QPixmap tableBg = QPixmap(Config.TableBgImage).scaled(displayRegion.width(), displayRegion.height() + 5, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_tableh -= _m_roomLayout->m_photoDashboardPadding;
    m_tableBg->setPos(0, 0);
    m_tableBg->setPixmap(tableBg);
    updateTable();
}

void RoomScene::changeTableBg(const QString &tableBgImage_path)
{
    QRectF displayRegion = sceneRect();

    QPixmap tableBg = QPixmap(tableBgImage_path).scaled(displayRegion.width(), displayRegion.height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    m_tableh -= _m_roomLayout->m_photoDashboardPadding;
    m_tableBg->setPos(0, 0);
    m_tableBg->setPixmap(tableBg);
    updateTable();
}

void RoomScene::changeHp(const QString &who, int delta, QSanguosha::DamageNature nature, bool losthp)
{
    // update
    Photo *photo = name2photo.value(who, NULL);
    if (photo != nullptr)
        photo->updateHp();
    else
        dashboard->update();

    if (delta < 0) {
        if (losthp) {
            Audio::playSystemAudioEffect(QStringLiteral("hplost"));
            QString from_general = ClientInstance->findPlayer(who)->objectName();
            log_box->appendLog(QStringLiteral("#GetHp"), from_general, QStringList(), QString(), QString::number(ClientInstance->findPlayer(who)->hp()),
                               QString::number(ClientInstance->findPlayer(who)->maxHp()));
            return;
        }

        QString damage_effect;
        QString from_general = ClientInstance->findPlayer(who)->objectName();
        log_box->appendLog(QStringLiteral("#GetHp"), from_general, QStringList(), QString(), QString::number(ClientInstance->findPlayer(who)->hp()),
                           QString::number(ClientInstance->findPlayer(who)->maxHp()));
        switch (delta) {
        case -1:
            damage_effect = QStringLiteral("injure1");
            break;
        case -2:
            damage_effect = QStringLiteral("injure2");
            break;
        case -3:
        default:
            damage_effect = QStringLiteral("injure3");
            break;
        }

        Audio::playSystemAudioEffect(damage_effect);

        if (photo != nullptr) {
            setEmotion(who, QStringLiteral("damage"));
            photo->tremble();
        }

        if (nature == QSanguosha::DamageFire)
            doAnimation(S_ANIMATE_FIRE, QStringList() << who);
        else if (nature == QSanguosha::DamageThunder)
            doAnimation(S_ANIMATE_LIGHTNING, QStringList() << who);
    } else {
        QString type = QStringLiteral("#Recover");
        QString from_general = ClientInstance->findPlayer(who)->objectName();
        QString n = QString::number(delta);

        log_box->appendLog(type, from_general, QStringList(), QString(), n);
        log_box->appendLog(QStringLiteral("#GetHp"), from_general, QStringList(), QString(), QString::number(ClientInstance->findPlayer(who)->hp()),
                           QString::number(ClientInstance->findPlayer(who)->maxHp()));
    }
}

void RoomScene::changeMaxHp(const QString & /*unused*/, int delta)
{
    if (delta < 0)
        Audio::playSystemAudioEffect(QStringLiteral("maxhplost"));
}

void RoomScene::onStandoff()
{
    log_box->append(QString(tr("<font color='%1'>---------- Game Finish ----------</font>").arg(Config.TextEditColor.name())));

    freeze();
    Audio::playSystemAudioEffect(QStringLiteral("standoff"));

    QDialog *dialog = new QDialog(main_window);
    dialog->resize(500, 600);
    dialog->setWindowTitle(tr("Standoff"));

    QVBoxLayout *layout = new QVBoxLayout;

    QTableWidget *table = new QTableWidget;
    fillTable(table, ConstClientInstance->players());

    layout->addWidget(table);
    dialog->setLayout(layout);

    addRestartButton(dialog);

    dialog->exec();
}

void RoomScene::onGameOver()
{
    log_box->append(QString(tr("<font color='%1'>---------- Game Finish ----------</font>").arg(Config.TextEditColor.name())));

    m_roomMutex.lock();
    freeze();
    time_label_widget->initializeLabel();
    bool victory = Self->property("win").toBool();

    QString win_effect;
    if (victory)
        win_effect = QStringLiteral("win");
    else
        win_effect = QStringLiteral("lose");

    Audio::playSystemAudioEffect(win_effect);

    QDialog *dialog = new QDialog(main_window);
    dialog->resize(800, 600);
    dialog->setWindowTitle(victory ? tr("Victory") : tr("Failure"));

    QGroupBox *winner_box = new QGroupBox(tr("Winner(s)"));
    QGroupBox *loser_box = new QGroupBox(tr("Loser(s)"));

    QTableWidget *winner_table = new QTableWidget;
    QTableWidget *loser_table = new QTableWidget;

    QVBoxLayout *winner_layout = new QVBoxLayout;
    winner_layout->addWidget(winner_table);
    winner_box->setLayout(winner_layout);

    QVBoxLayout *loser_layout = new QVBoxLayout;
    loser_layout->addWidget(loser_table);
    loser_box->setLayout(loser_layout);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(winner_box);
    layout->addWidget(loser_box);
    dialog->setLayout(layout);

    QList<const Player *> winner_list;
    QList<const Player *> loser_list;
    foreach (const Player *player, ClientInstance->players()) {
        bool win = player->property("win").toBool();
        if (win)
            winner_list << player;
        else
            loser_list << player;
    }

    fillTable(winner_table, winner_list);
    fillTable(loser_table, loser_list);

    if (!ClientInstance->getReplayer() && Config.value(QStringLiteral("EnableAutoSaveRecord"), false).toBool())
        saveReplayRecord(true, Config.value(QStringLiteral("NetworkOnly"), false).toBool());

    addRestartButton(dialog);
    m_roomMutex.unlock();
    dialog->exec();
}

void RoomScene::addRestartButton(QDialog *dialog)
{
    dialog->resize(main_window->width() / 2, dialog->height());

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();

    if (ClientInstance->findChild<Replayer *>() == NULL && QUrl(Config.HostAddress).path().isEmpty()) {
        QPushButton *restart_button = new QPushButton(tr("Restart Game"));
        connect(restart_button, &QAbstractButton::clicked, dialog, &QDialog::accept);
        connect(restart_button, &QAbstractButton::clicked, this, &RoomScene::restart);
        hlayout->addWidget(restart_button);
        QPushButton *save_button = new QPushButton(tr("Save record"));
        connect(save_button, SIGNAL(clicked()), this, SLOT(saveReplayRecord()));
        hlayout->addWidget(save_button);
    }
    QPushButton *return_button = new QPushButton(tr("Return to main menu"));
    connect(return_button, &QAbstractButton::clicked, dialog, &QDialog::accept);
    connect(return_button, &QAbstractButton::clicked, this, &RoomScene::return_to_start);
    hlayout->addWidget(return_button);

    QVBoxLayout *layout = qobject_cast<QVBoxLayout *>(dialog->layout());
    if (layout != nullptr)
        layout->addLayout(hlayout);
}

void RoomScene::saveReplayRecord()
{
    saveReplayRecord(false);
}

void RoomScene::saveReplayRecord(bool auto_save, bool network_only)
{
    if (auto_save) {
        int human = 0;
        foreach (const Player *player, ClientInstance->players()) {
            if (player == Self) {
                human++;
                continue;
            }
            if (player->getState() != QStringLiteral("robot"))
                human++;
        }
        bool is_network = (human >= 2);
        if (network_only && !is_network)
            return;
        QString location = Config.value(QStringLiteral("RecordSavePaths"), QStringLiteral("records/")).toString();
        if (!location.startsWith(QStringLiteral(":"))) {
            location.replace(QStringLiteral("\\"), QStringLiteral("/"));
            if (!location.endsWith(QStringLiteral("/")))
                location.append(QStringLiteral("/"));
            if (!QDir(location).exists())
                QDir().mkdir(location);
            QString general_name = Sanguosha->translate(Self->generalName());
            if (ClientInstance->serverInfo()->isMultiGeneralEnabled())
                general_name.append(QStringLiteral("_") + Sanguosha->translate(Self->getGeneral2Name()));
            location.append(QStringLiteral("%1 %2(%3)-").arg(Sanguosha->versionNumber().toString(), general_name, Sanguosha->translate(Self->roleString())));
            location.append(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss")));
            location.append(QStringLiteral(".txt"));
            ClientInstance->save(location);
        }
        return;
    }

    QString location = Config.value(QStringLiteral("LastReplayDir")).toString();
    if (location.isEmpty())
        location = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);

    QString filename = QFileDialog::getSaveFileName(main_window, tr("Save replay record"), location, tr("Pure text replay file (*.txt);; Image replay file (*.png)"));

    if (!filename.isEmpty()) {
        ClientInstance->save(filename);

        QFileInfo file_info(filename);
        QString last_dir = file_info.absoluteDir().path();
        Config.setValue(QStringLiteral("LastReplayDir"), last_dir);
    }
}

ScriptExecutor::ScriptExecutor(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Script execution"));
    QVBoxLayout *vlayout = new QVBoxLayout;
    vlayout->addWidget(new QLabel(tr("Please input the script that should be executed at server side:\n P = you, R = your room")));

    QTextEdit *box = new QTextEdit;
    box->setObjectName(QStringLiteral("scriptBox"));
    vlayout->addWidget(box);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();

    QPushButton *ok_button = new QPushButton(tr("OK"));
    hlayout->addWidget(ok_button);

    vlayout->addLayout(hlayout);

    connect(ok_button, &QAbstractButton::clicked, this, &QDialog::accept);
    connect(this, &QDialog::accepted, this, &ScriptExecutor::doScript);

    setLayout(vlayout);
}

void ScriptExecutor::doScript()
{
    QTextEdit *box = findChild<QTextEdit *>(QStringLiteral("scriptBox"));
    if (box == nullptr)
        return;

    QString script = box->toPlainText();
    QByteArray data = script.toLatin1();
    data = qCompress(data);
    script = QString::fromUtf8(data.toBase64());

    ClientInstance->requestCheatRunScript(script);
}

DeathNoteDialog::DeathNoteDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Death note"));

    killer = new QComboBox;
    RoomScene::FillPlayerNames(killer, true);

    victim = new QComboBox;
    RoomScene::FillPlayerNames(victim, false);

    QPushButton *ok_button = new QPushButton(tr("OK"));
    connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));

    QFormLayout *layout = new QFormLayout;
    layout->addRow(tr("Killer"), killer);
    layout->addRow(tr("Victim"), victim);

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(ok_button);
    layout->addRow(hlayout);

    setLayout(layout);
}

void DeathNoteDialog::accept()
{
    QDialog::accept();
    ClientInstance->requestCheatKill(killer->itemData(killer->currentIndex()).toString(), victim->itemData(victim->currentIndex()).toString());
}

DamageMakerDialog::DamageMakerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Damage maker"));

    damage_source = new QComboBox;
    RoomScene::FillPlayerNames(damage_source, true);

    damage_target = new QComboBox;
    RoomScene::FillPlayerNames(damage_target, false);

    damage_nature = new QComboBox;
    damage_nature->addItem(tr("Normal"), S_CHEAT_NORMAL_DAMAGE);
    damage_nature->addItem(tr("Thunder"), S_CHEAT_THUNDER_DAMAGE);
    damage_nature->addItem(tr("Fire"), S_CHEAT_FIRE_DAMAGE);
    damage_nature->addItem(tr("Recover HP"), S_CHEAT_HP_RECOVER);
    damage_nature->addItem(tr("Lose HP"), S_CHEAT_HP_LOSE);
    damage_nature->addItem(tr("Lose Max HP"), S_CHEAT_MAX_HP_LOSE);
    damage_nature->addItem(tr("Reset Max HP"), S_CHEAT_MAX_HP_RESET);

    damage_point = new QSpinBox;
    damage_point->setRange(1, INT_MAX);
    damage_point->setValue(1);

    QPushButton *ok_button = new QPushButton(tr("OK"));
    connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(ok_button);

    QFormLayout *layout = new QFormLayout;

    layout->addRow(tr("Damage source"), damage_source);
    layout->addRow(tr("Damage target"), damage_target);
    layout->addRow(tr("Damage nature"), damage_nature);
    layout->addRow(tr("Damage point"), damage_point);
    layout->addRow(hlayout);

    setLayout(layout);

    connect(damage_nature, &QComboBox::currentIndexChanged, this, &DamageMakerDialog::disableSource);
}

void DamageMakerDialog::disableSource()
{
    QString nature = damage_nature->itemData(damage_nature->currentIndex()).toString();
    damage_source->setEnabled(nature != QStringLiteral("L"));
}

void RoomScene::FillPlayerNames(QComboBox *ComboBox, bool add_none)
{
    if (add_none)
        ComboBox->addItem(tr("None"), QStringLiteral("."));
    ComboBox->setIconSize(G_COMMON_LAYOUT.m_tinyAvatarSize);
    foreach (const Player *player, ClientInstance->players()) {
        QString general_name = Sanguosha->translate(player->generalName());
        if (player->general() == nullptr)
            continue;
        QPixmap pixmap = G_ROOM_SKIN.getGeneralPixmap(player->generalName(), QSanRoomSkin::S_GENERAL_ICON_SIZE_TINY, false);
        ComboBox->addItem(QIcon(pixmap), QStringLiteral("%1 [%2]").arg(general_name, player->screenName()), player->objectName());
    }
}

void DamageMakerDialog::accept()
{
    QDialog::accept();

    ClientInstance->requestCheatDamage(damage_source->itemData(damage_source->currentIndex()).toString(), damage_target->itemData(damage_target->currentIndex()).toString(),
                                       (QSanguosha::DamageNature)damage_nature->itemData(damage_nature->currentIndex()).toInt(), damage_point->value());
}

void RoomScene::makeDamage()
{
    if (ClientInstance->getStatus() != Client::Playing) {
        QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
        return;
    }

    DamageMakerDialog *damage_maker = new DamageMakerDialog(main_window);
    damage_maker->exec();
}

void RoomScene::makeKilling()
{
    if (ClientInstance->getStatus() != Client::Playing) {
        QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
        return;
    }

    DeathNoteDialog *dialog = new DeathNoteDialog(main_window);
    dialog->exec();
}

void RoomScene::makeReviving()
{
    if (ClientInstance->getStatus() != Client::Playing) {
        QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
        return;
    }

    QStringList items;
    QList<const Player *> victims;
    foreach (const Player *player, ClientInstance->players()) {
        if (player->isDead()) {
            QString general_name = Sanguosha->translate(player->generalName());
            items << QStringLiteral("%1 [%2]").arg(player->screenName(), general_name);
            victims << player;
        }
    }

    if (items.isEmpty()) {
        QMessageBox::warning(main_window, tr("Warning"), tr("No victims now!"));
        return;
    }

    bool ok = false;
    QString item = QInputDialog::getItem(main_window, tr("Reviving wand"), tr("Please select a player to revive"), items, 0, false, &ok);
    if (ok) {
        int index = items.indexOf(item);
        ClientInstance->requestCheatRevive(victims.at(index)->objectName());
    }
}

void RoomScene::doScript()
{
    if (ClientInstance->getStatus() != Client::Playing) {
        QMessageBox::warning(main_window, tr("Warning"), tr("This function is only allowed at your play phase!"));
        return;
    }

    ScriptExecutor *dialog = new ScriptExecutor(main_window);
    dialog->exec();
}

void RoomScene::viewGenerals(const QString &reason, const QStringList &names)
{
    QDialog *dialog = new ChooseGeneralDialog(names, main_window, true, Sanguosha->translate(reason));
    connect(dialog, &QDialog::rejected, dialog, &QObject::deleteLater);
    dialog->setParent(main_window, Qt::Dialog);
    dialog->show();
}

void RoomScene::fillTable(QTableWidget *table, const QList<const Player *> &players)
{
    table->setColumnCount(10);
    table->setRowCount(players.length());
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    static QStringList labels;
    if (labels.isEmpty()) {
        labels << tr("General") << tr("Name") << tr("Alive");
        labels << tr("Role");
        labels << tr("Handcards");
    }
    table->setHorizontalHeaderLabels(labels);
    table->setSelectionBehavior(QTableWidget::SelectRows);

    for (int i = 0; i < players.length(); i++) {
        const Player *player = players[i];

        QTableWidgetItem *item = new QTableWidgetItem;
        item->setText(ClientInstance->getPlayerName(player->objectName()));
        table->setItem(i, 0, item);

        item = new QTableWidgetItem;
        item->setText(player->screenName());
        table->setItem(i, 1, item);

        item = new QTableWidgetItem;
        if (player->isAlive())
            item->setText(tr("Alive"));
        else
            item->setText(tr("Dead"));
        table->setItem(i, 2, item);

        item = new QTableWidgetItem;

        QIcon icon(QStringLiteral("image/system/roles/%1.png").arg(player->roleString()));
        item->setIcon(icon);
        QString role = player->roleString();
        if (ClientInstance->serverInfo()->GameModeStr.startsWith(QStringLiteral("06_"))) {
            if (role == QStringLiteral("lord") || role == QStringLiteral("renegade"))
                role = QStringLiteral("leader");
            else
                role = QStringLiteral("guard");
        } else if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("04_1v3")) {
            int seat = player->seat();
            switch (seat) {
            case 1:
                role = QStringLiteral("lvbu");
                break;
            case 2:
                role = QStringLiteral("vanguard");
                break;
            case 3:
                role = QStringLiteral("mainstay");
                break;
            case 4:
                role = QStringLiteral("general");
                break;
            }
        } else if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1")) {
            if (role == QStringLiteral("lord"))
                role = QStringLiteral("defensive");
            else
                role = QStringLiteral("offensive");
        }
        item->setText(Sanguosha->translate(role));

        if (!player->isAlive())
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        table->setItem(i, 3, item);

        item = new QTableWidgetItem;
        QString handcards = QString::fromUtf8(QByteArray::fromBase64(player->property("last_handcards").toString().toLatin1()));
        handcards.replace(QStringLiteral("<img src='image/system/log/spade.png' height = 12/>"), tr("Spade"));
        handcards.replace(QStringLiteral("<img src='image/system/log/heart.png' height = 12/>"), tr("Heart"));
        handcards.replace(QStringLiteral("<img src='image/system/log/club.png' height = 12/>"), tr("Club"));
        handcards.replace(QStringLiteral("<img src='image/system/log/diamond.png' height = 12/>"), tr("Diamond"));
        item->setText(handcards);
        table->setItem(i, 4, item);
    }

    for (int i = 2; i <= 4; i++)
        table->resizeColumnToContents(i);
}

void RoomScene::killPlayer(const QString &who)
{
    const General *general = nullptr;
    m_roomMutex.lock();

    Player *player = ClientInstance->findPlayer(who);
    if (player != nullptr) {
        PlayerCardContainer *container = (PlayerCardContainer *)_getGenericCardContainer(QSanguosha::PlaceHand, player);
        container->stopHuaShen();
    }

    if (who == Self->objectName()) {
        dashboard->killPlayer();
        dashboard->update();
        general = Self->general();
        item2player.remove(dashboard);
        if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1"))
            self_box->killPlayer(general->name());
    } else {
        Photo *photo = name2photo[who];
        photo->killPlayer();
        photo->setFrame(Photo::S_FRAME_NO_FRAME);
        photo->update();
        item2player.remove(photo);
        general = photo->getPlayer()->general();
        if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1"))
            enemy_box->killPlayer(general->name());
    }

    if (Config.EnableEffects && Config.EnableLastWord && !Self->hasFlag(QStringLiteral("marshalling")))
        Audio::GeneralLastWord(general->name());
    m_roomMutex.unlock();
}

void RoomScene::revivePlayer(const QString &who)
{
    if (who == Self->objectName()) {
        dashboard->revivePlayer();
        item2player.insert(dashboard, Self);
        updateSkillButtons();
    } else {
        Photo *photo = name2photo[who];
        photo->revivePlayer();
        item2player.insert(photo, photo->getPlayer());
    }
}

void RoomScene::setDashboardShadow(const QString &who)
{
    if (who != Self->objectName())
        return;
    dashboard->setDeathColor();
}

void RoomScene::takeAmazingGrace(Player *taker, int card_id, bool move_cards)
{
    QList<int> card_ids;
    card_ids.append(card_id);
    m_tablePile->clear();

    card_container->m_currentPlayer = taker;
    CardItem *copy = card_container->removeCardItems(card_ids, QSanguosha::PlaceHand).first();
    if (copy == nullptr)
        return;

    QList<CardItem *> items;
    items << copy;

    if (taker != nullptr) {
        GenericCardContainer *container = _getGenericCardContainer(QSanguosha::PlaceHand, taker);
        bringToFront(container);
        if (move_cards) {
            QString type = QStringLiteral("$TakeAG");
            QString from_general = taker->objectName();
            QString card_str = QString::number(card_id);
            log_box->appendLog(type, from_general, QStringList(), card_str);
            LegacyCardsMoveStruct move;
            move.card_ids.append(card_id);
            move.from_place = QSanguosha::PlaceTable;
            move.to_place = QSanguosha::PlaceHand;
            move.to = taker;
            container->addCardItems(items, move);
        } else {
            delete copy;
        }
    } else
        delete copy;
}

void RoomScene::showCard(const QString &player_name, int card_id)
{
    QList<int> card_ids;
    card_ids << card_id;
    Player *player = ClientInstance->findPlayer(player_name);

    GenericCardContainer *container = _getGenericCardContainer(QSanguosha::PlaceHand, player);
    QList<CardItem *> card_items = container->cloneCardItems(card_ids);
    CardMoveReason reason(QSanguosha::MoveReasonDemonstrate, player->objectName());
    bringToFront(m_tablePile);
    LegacyCardsMoveStruct move;
    move.from_place = QSanguosha::PlaceHand;
    move.to_place = QSanguosha::PlaceTable;
    move.reason = reason;
    card_items[0]->setFootnote(_translateMovement(move));
    m_tablePile->addCardItems(card_items, move);

    QString card_str = QString::number(card_id);
    log_box->appendLog(QStringLiteral("$ShowCard"), player->objectName(), QStringList(), card_str);
}

void RoomScene::chooseSkillButton()
{
    QList<QSanSkillButton *> enabled_buttons;
    foreach (QSanSkillButton *btn, m_skillButtons) {
        if (btn->isEnabled())
            enabled_buttons << btn;
    }

    if (enabled_buttons.isEmpty())
        return;

    QDialog *dialog = new QDialog(main_window);
    dialog->setWindowTitle(tr("Select skill"));

    QVBoxLayout *layout = new QVBoxLayout;

    foreach (QSanSkillButton *btn, enabled_buttons) {
        Q_ASSERT(btn->getSkill());
        CommandLinkDoubleClickButton *button = new CommandLinkDoubleClickButton(Sanguosha->translate(btn->getSkill()->name()));
        connect(button, &CommandLinkDoubleClickButton::double_clicked, btn, &QSanButton::click);
        connect(button, &CommandLinkDoubleClickButton::double_clicked, dialog, &QDialog::accept);
        layout->addWidget(button);
    }

    dialog->setLayout(layout);
    dialog->exec();
}

void RoomScene::attachSkill(const QString &skill_name, bool from_left)
{
    const Skill *skill = Sanguosha->skill(skill_name);
    if (skill != nullptr)
        addSkillButton(skill, from_left);
}

void RoomScene::detachSkill(const QString &skill_name, bool head)
{
    QSanSkillButton *btn = dashboard->removeSkillButton(skill_name, head);
    if (btn == nullptr)
        return; //be care LordSkill
    m_skillButtons.removeAll(btn);
    btn->deleteLater();
}

void RoomScene::viewDistance()
{
    DistanceViewDialog *dialog = new DistanceViewDialog(main_window);
    dialog->show();
}

void RoomScene::speak()
{
    bool broadcast = true;
    QString text = chat_edit->text();
    if (text == QStringLiteral(".StartBgMusic")) {
        broadcast = false;
        Config.EnableBgMusic = true;
        Config.setValue(QStringLiteral("EnableBgMusic"), true);

        Audio::stopBGM();
        Audio::playBGM(Audio::getBgmFileNames(QString(), false));
        Audio::setBGMVolume(Config.BGMVolume);

    } else if (text.startsWith(QStringLiteral(".StartBgMusic="))) {
        broadcast = false;
        Config.EnableBgMusic = true;
        Config.setValue(QStringLiteral("EnableBgMusic"), true);
        QString path = text.mid(14);
        if (path.startsWith(QStringLiteral("|"))) {
            path = path.mid(1);
            Config.setValue(QStringLiteral("BackgroundMusic"), path);
        }

        Audio::stopBGM();
        Audio::playBGM(QStringList(path));
        Audio::setBGMVolume(Config.BGMVolume);

    } else if (text == QStringLiteral(".StopBgMusic")) {
        broadcast = false;
        Config.EnableBgMusic = false;
        Config.setValue(QStringLiteral("EnableBgMusic"), false);

        Audio::stopBGM();
    }
    if (broadcast) {
        if (game_started && ClientInstance->serverInfo()->DisableChat)
            chat_box->append(tr("This room does not allow chatting!"));
        else
            ClientInstance->speakToServer(text);
    } else {
        QString title;
        if (Self != nullptr) {
            title = Self->generalName();
            title = Sanguosha->translate(title);
            title.append(QStringLiteral("(%1)").arg(Self->screenName()));
            title = QStringLiteral("<b>%1</b>").arg(title);
        }
        QString line = tr("<font color='%1'>[%2] said: %3 </font>").arg(Config.TextEditColor.name(), title, text);
        appendChatBox(QStringLiteral("<p style=\"margin:3px 2px;\">%1</p>").arg(line));
    }
    chat_edit->clear();
}

void RoomScene::fillCards(const QList<int> &card_ids, const QList<int> &disabled_ids, const QList<int> &shownHandcard_ids)
{
    bringToFront(card_container);
    card_container->fillCards(card_ids, disabled_ids, shownHandcard_ids);
    card_container->show();
}

void RoomScene::doGongxin(const QList<int> &card_ids, bool enable_heart, const QList<int> &enabled_ids, const QList<int> &shownHandcard_ids)
{
    fillCards(card_ids, QList<int>(), shownHandcard_ids);
    if (enable_heart)
        card_container->startGongxin(enabled_ids);
    else
        card_container->addCloseButton();
}

void RoomScene::showOwnerButtons(bool owner)
{
    if ((add_robot != nullptr) && (fill_robots != nullptr) && !game_started && ClientInstance->serverInfo()->EnableAI) {
        add_robot->setVisible(owner);
        fill_robots->setVisible(owner);
    }
}

void RoomScene::showPlayerCards()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action != nullptr) {
        QStringList names = action->data().toString().split(QStringLiteral("."));
        QString player_name = names.first();
        const Player *player = ClientInstance->findPlayer(player_name);
        if (names.length() > 1) {
            QString pile_name = names.last();

            CardOverview *overview = new CardOverview;
            overview->setWindowTitle(QStringLiteral("%1 %2").arg(ClientInstance->getPlayerName(player_name), Sanguosha->translate(pile_name)));
            overview->loadFromList(player->pile(pile_name));
            overview->show();
        } else {
            IdSet cards;
            foreach (const Card *card, player->handCards()) {
                if (card->id() >= 0)
                    cards << card->id();
            }

            CardOverview *overview = new CardOverview;
            overview->setWindowTitle(QStringLiteral("%1 %2").arg(ClientInstance->getPlayerName(player_name), tr("Known cards")));
            overview->loadFromList(cards);
            overview->show();
        }
    }
}

void RoomScene::showPile(const QList<int> &card_ids, const QString &name, const Player *target)
{
    pileContainer->clear();
    bringToFront(pileContainer);
    pileContainer->setObjectName(name);
    if (name == QStringLiteral("huashencard") && target->hasGeneralCardSkill(QStringLiteral("anyun"))) { //target->hasSkill("anyun", true)
        //        QStringList huashens = target->getHiddenGenerals();
        //        QList<CardItem *> generals;
        //        foreach (QString arg, huashens) {
        //            CardItem *item = new CardItem(arg);
        //            addItem(item);
        //            item->setParentItem(pileContainer);
        //            generals.append(item);
        //        }
        //        pileContainer->fillGeneralCards(generals);
    } else {
        if (name == QStringLiteral("zhenli")) {
            QList<Card *> zhenlis;
            foreach (int id, card_ids) {
                zhenlis << ClientInstance->card(id);
            }
            std::sort(zhenlis.begin(), zhenlis.end(), [](const Card *a, const Card *b) {
                return a->number() < b->number();
            });
            QList<int> zhenids;
            foreach (Card *c, zhenlis)
                zhenids << c->id();
            pileContainer->fillCards(zhenids);
        } else
            pileContainer->fillCards(card_ids);
    }
    pileContainer->setPos(m_tableCenterPos - QPointF(pileContainer->boundingRect().width() / 2, pileContainer->boundingRect().height() / 2));
    pileContainer->show();
}

QString RoomScene::getCurrentShownPileName()
{
    if (pileContainer->isVisible())
        return pileContainer->objectName();
    else
        return QString();
}

void RoomScene::hidePile()
{
    pileContainer->clear();
}

KOFOrderBox::KOFOrderBox(bool self, QGraphicsScene *scene)
{
    QString basename = self ? QStringLiteral("self") : QStringLiteral("enemy");
    QString path = QStringLiteral("image/system/1v1/%1.png").arg(basename);
    setPixmap(QPixmap(path));
    scene->addItem(this);

    for (int i = 0; i < 3; i++) {
        avatars[i] = new QSanSelectableItem;
        avatars[i]->load(QStringLiteral("image/system/1v1/unknown.png"), QSize(122, 50));
        avatars[i]->setParentItem(this);
        avatars[i]->setPos(5, 23 + 62 * i);
        avatars[i]->setObjectName(QStringLiteral("unknown"));
    }

    revealed = 0;
}

void KOFOrderBox::revealGeneral(const QString &name)
{
    if (revealed < 3) {
        avatars[revealed]->setPixmap(G_ROOM_SKIN.getGeneralPixmap(name, QSanRoomSkin::S_GENERAL_ICON_SIZE_KOF, false));
        avatars[revealed]->setObjectName(name);
        const General *general = Sanguosha->general(name);
        if (general != nullptr)
            avatars[revealed]->setToolTip(ClientInstance->getGeneralSkillDescription(general->name(), true));
        revealed++;
    }
}

void KOFOrderBox::killPlayer(const QString &general_name)
{
    for (int i = 0; i < revealed; i++) {
        QSanSelectableItem *avatar = avatars[i];
        if (avatar->isEnabled() && avatar->objectName() == general_name) {
            QPixmap pixmap(QStringLiteral("image/system/death/unknown.png"));
            QGraphicsPixmapItem *death = new QGraphicsPixmapItem(pixmap, avatar);
            death->setScale(0.5);
            death->moveBy(15, 0);
            avatar->makeGray();
            avatar->setEnabled(false);

            return;
        }
    }
}

void RoomScene::onGameStart()
{
    Audio::stopBGM();

    if (ClientInstance->getReplayer() != nullptr && (m_chooseGeneralBox != nullptr))
        m_chooseGeneralBox->clear();

    main_window->activateWindow();
    if (Config.GameMode.contains(QStringLiteral("_mini_"))) {
        QString id = Config.GameMode;
        id.replace(QStringLiteral("_mini_"), QString());
        _m_currentStage = id.toInt();
    } else if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3") || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_XMode")
               || ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1")) {
        chat_widget->show();
        log_box->show();

        if ((self_box != nullptr) && (enemy_box != nullptr)) {
            self_box->show();
            enemy_box->show();
        }
    }

    if (control_panel != nullptr)
        control_panel->hide();

    if (!Self->hasFlag(QStringLiteral("marshalling")))
        log_box->append(QString(tr("<font color='%1'>---------- Game Start ----------</font>").arg(Config.TextEditColor.name())));

    trust_button->setEnabled(true);

    setLordBGM();

    setLordBackdrop();

    game_started = true;

    if (isHegemonyGameMode(ClientInstance->serverInfo()->GameModeStr)) {
        dashboard->refresh();
        dashboard->showSeat();
        foreach (Photo *photo, photos)
            photo->showSeat();
    }
}

void RoomScene::freeze()
{
    dashboard->setEnabled(false);
    dashboard->stopHuaShen();
    foreach (Photo *photo, photos) {
        photo->hideProgressBar();
        photo->stopHuaShen();
        photo->setEnabled(false);
    }
    item2player.clear();
    chat_edit->setEnabled(false);

    Audio::stopBGM();

    dashboard->hideProgressBar();
    main_window->setStatusBar(nullptr);
}

void RoomScene::_cancelAllFocus()
{
    foreach (Photo *photo, photos) {
        photo->hideProgressBar();
        if (photo->getPlayer()->phase() == QSanguosha::PhaseNotActive)
            photo->setFrame(Photo::S_FRAME_NO_FRAME);
    }
}

void RoomScene::moveFocus(const QStringList &players, const Countdown &countdown)
{
    _cancelAllFocus();
    foreach (QString player, players) {
        Photo *photo = name2photo[player];
        if (photo == nullptr) {
            Q_ASSERT(player == Self->objectName());
            continue;
        }

        if (ClientInstance->serverInfo()->OperationTimeout > 0)
            photo->showProgressBar(countdown);
        else if (photo->getPlayer()->phase() == QSanguosha::PhaseNotActive)
            photo->setFrame(Photo::S_FRAME_RESPONDING);
    }
}

void RoomScene::setEmotion(const QString &who, const QString &emotion)
{
    bool permanent = false;
    if (emotion == QStringLiteral("question") || emotion == QStringLiteral("no-question"))
        permanent = true;
    setEmotion(who, emotion, permanent);
}

void RoomScene::setEmotion(const QString &who, const QString &emotion, bool permanent)
{
    if (emotion.startsWith(QStringLiteral("weapon/")) || emotion.startsWith(QStringLiteral("armor/")) || emotion.startsWith(QStringLiteral("treasure/"))) {
        if (Config.value(QStringLiteral("NoEquipAnim"), false).toBool())
            return;
        QString name = emotion.split(QStringLiteral("/")).last();
        Audio::playAudioEffect(G_ROOM_SKIN.getPlayerAudioEffectPath(name, QStringLiteral("equip"), -1));
    } else if (emotion == QStringLiteral("chain"))
        Audio::playAudioEffect(QStringLiteral("chained"));

    Photo *photo = name2photo[who];
    if (photo != nullptr) {
        photo->setEmotion(emotion, permanent);
    } else {
        PixmapAnimation *pma = PixmapAnimation::GetPixmapAnimation(dashboard, emotion);
        if (pma != nullptr) {
            pma->moveBy(0, -dashboard->boundingRect().height() / 1.5);
            pma->setZValue(20002.0);
        }
    }
}

void RoomScene::showSkillInvocation(const QString &who, const QString &skill_name)
{
    const Player *player = ClientInstance->findChild<const Player *>(who);
    //for hegemony gamemode: invoke hidden skill before showskill
    QStringList skills = Sanguosha->skillNames();
    if (skill_name == QStringLiteral("GameRule_AskForGeneralShowHead") || skill_name == QStringLiteral("GameRule_AskForGeneralShowDeputy"))
        return;
    if (!isHegemonyGameMode(ClientInstance->serverInfo()->GameModeStr) && !player->hasValidSkill(skill_name) && !player->hasEquipSkill(skill_name))
        return;
    else if (isHegemonyGameMode(ClientInstance->serverInfo()->GameModeStr) && !skills.contains(skill_name) && !player->hasEquipSkill(skill_name))
        return;

    QString type = QStringLiteral("#InvokeSkill");
    QString from_general = player->objectName();
    const QString &arg = skill_name;
    log_box->appendLog(type, from_general, QStringList(), QString(), arg);
}

void RoomScene::removeLightBox()
{
    LightboxAnimation *lightbox = qobject_cast<LightboxAnimation *>(sender());
    if (lightbox != nullptr) {
        removeItem(lightbox);
        lightbox->deleteLater();
    } else {
        PixmapAnimation *pma = qobject_cast<PixmapAnimation *>(sender());
        if (pma != nullptr) {
            removeItem(pma->parentItem());
        } else {
            QPropertyAnimation *animation = qobject_cast<QPropertyAnimation *>(sender());
            QGraphicsTextItem *line = qobject_cast<QGraphicsTextItem *>(animation->targetObject());
            if (line != nullptr) {
                removeItem(line->parentItem());
            } else {
                QSanSelectableItem *line = qobject_cast<QSanSelectableItem *>(animation->targetObject());
                removeItem(line->parentItem());
            }
        }
    }
}

QGraphicsObject *RoomScene::getAnimationObject(const QString &name) const
{
    if (name == Self->objectName())
        return dashboard;
    else
        return name2photo.value(name);
}

void RoomScene::doMovingAnimation(const QString &name, const QStringList &args)
{
    QSanSelectableItem *item = new QSanSelectableItem(QStringLiteral("image/system/animation/%1.png").arg(name));
    item->setZValue(10086.0);
    addItem(item);

    QGraphicsObject *fromItem = getAnimationObject(args.at(0));
    QGraphicsObject *toItem = getAnimationObject(args.at(1));

    QPointF from = fromItem->scenePos();
    QPointF to = toItem->scenePos();
    if (fromItem == dashboard)
        from.setX(fromItem->boundingRect().width() / 2);
    if (toItem == dashboard)
        to.setX(toItem->boundingRect().width() / 2);

    QSequentialAnimationGroup *group = new QSequentialAnimationGroup;

    QPropertyAnimation *move = new QPropertyAnimation(item, "pos");
    move->setStartValue(from);
    move->setEndValue(to);
    move->setDuration(1000);

    QPropertyAnimation *disappear = new QPropertyAnimation(item, "opacity");
    disappear->setEndValue(0.0);
    disappear->setDuration(1000);

    group->addAnimation(move);
    group->addAnimation(disappear);

    group->start(QAbstractAnimation::DeleteWhenStopped);
    connect(group, &QAbstractAnimation::finished, item, &QObject::deleteLater);
}

void RoomScene::doAppearingAnimation(const QString &name, const QStringList &args)
{
    QSanSelectableItem *item = new QSanSelectableItem(QStringLiteral("image/system/animation/%1.png").arg(name));
    addItem(item);

    QPointF from = getAnimationObject(args.at(0))->scenePos();
    item->setPos(from);

    QPropertyAnimation *disappear = new QPropertyAnimation(item, "opacity");
    disappear->setEndValue(0.0);
    disappear->setDuration(1000);

    disappear->start(QAbstractAnimation::DeleteWhenStopped);
    connect(disappear, &QAbstractAnimation::finished, item, &QObject::deleteLater);
}

void RoomScene::doLightboxAnimation(const QString & /*unused*/, const QStringList &args)
{
    QString word = args.first();
    bool reset_size = word.startsWith(QStringLiteral("_mini_"));
    word = Sanguosha->translate(word);

    QRect rect = main_window->rect();

    if (word.startsWith(QStringLiteral("image="))) {
        QGraphicsRectItem *lightbox = addRect(rect);

        lightbox->setBrush(QColor(32, 32, 32, 204));
        lightbox->setZValue(20001.0);

        QSanSelectableItem *line = new QSanSelectableItem(word.mid(6));
        addItem(line);

        QRectF line_rect = line->boundingRect();
        line->setParentItem(lightbox);
        line->setPos(m_tableCenterPos - line_rect.center());

        QPropertyAnimation *appear = new QPropertyAnimation(line, "opacity");
        appear->setStartValue(0.0);
        appear->setKeyValueAt(0.7, 1.0);
        appear->setEndValue(0.0);

        int duration = args.value(1, QStringLiteral("2000")).toInt();
        appear->setDuration(duration);

        appear->start(QAbstractAnimation::DeleteWhenStopped);

        connect(appear, &QAbstractAnimation::finished, line, &QObject::deleteLater);
        connect(appear, &QAbstractAnimation::finished, this, &RoomScene::removeLightBox);
    } else if (word.startsWith(QStringLiteral("anim="))) {
        QGraphicsRectItem *lightbox = addRect(rect);

        lightbox->setBrush(QColor(32, 32, 32, 204));
        lightbox->setZValue(20001.0);

        PixmapAnimation *pma = PixmapAnimation::GetPixmapAnimation(lightbox, word.mid(5));
        if (pma != nullptr) {
            pma->setZValue(20002.0);
            pma->moveBy(-sceneRect().width() * _m_roomLayout->m_infoPlaneWidthPercentage / 2, 0);
            connect(pma, &PixmapAnimation::finished, this, &RoomScene::removeLightBox);
        }
    } else if (word.startsWith(QStringLiteral("skill="))) {
        QStringList l = word.mid(6).split(QStringLiteral(":"));
        LightboxAnimation *animation = new LightboxAnimation(l.first(), l.last(), rect);
        animation->setZValue(20001.0);
        addItem(animation);
        connect(animation, &LightboxAnimation::finished, this, &RoomScene::removeLightBox);
    } else {
        QGraphicsRectItem *lightbox = addRect(rect);

        lightbox->setBrush(QColor(32, 32, 32, 204));
        lightbox->setZValue(20001.0);

        QFont font = Config.BigFont;
        if (reset_size)
            font.setPixelSize(100);
        QGraphicsTextItem *line = addText(word, font);
        line->setDefaultTextColor(Qt::white);

        QRectF line_rect = line->boundingRect();
        line->setParentItem(lightbox);
        line->setPos(m_tableCenterPos - line_rect.center());

        QPropertyAnimation *appear = new QPropertyAnimation(line, "opacity");
        appear->setStartValue(0.0);
        appear->setKeyValueAt(0.7, 1.0);
        appear->setEndValue(0.0);

        int duration = args.value(1, QStringLiteral("2000")).toInt();
        appear->setDuration(duration);

        appear->start(QAbstractAnimation::DeleteWhenStopped);

        connect(appear, &QAbstractAnimation::finished, this, &RoomScene::removeLightBox);
    }
}

void RoomScene::doHuashen(const QString & /*unused*/, const QStringList &args)
{
    Q_ASSERT(args.length() >= 2);

    QStringList hargs = args;
    QString name = hargs.first();
    hargs.removeOne(name);
    hargs = hargs.first().split(QStringLiteral(":"));
    Player *player = ClientInstance->findPlayer(name);
    bool owner = (hargs.first() != QStringLiteral("unknown"));

    QVariantList huashen_list;
    if (owner)
        huashen_list = Self->tag[QStringLiteral("Huashens")].toList();
    QList<CardItem *> generals;

    foreach (QString arg, hargs) {
        if (owner)
            huashen_list << arg;
        CardItem *item = new CardItem(arg);
        item->setPos(m_tableCenterPos);
        addItem(item);
        generals.append(item);
    }
    LegacyCardsMoveStruct move;
    move.to = player;
    move.from_place = QSanguosha::PlaceDrawPile;
    move.to_place = QSanguosha::PlaceSpecial;
    move.to_pile_name = QStringLiteral("huashen");

    GenericCardContainer *container = _getGenericCardContainer(QSanguosha::PlaceHand, player);
    container->addCardItems(generals, move);

    if (owner)
        Self->tag[QStringLiteral("Huashens")] = huashen_list;
}

void RoomScene::showIndicator(const QString &from, const QString &to)
{
    if (Config.value(QStringLiteral("NoIndicator"), false).toBool())
        return;

    QGraphicsObject *obj1 = getAnimationObject(from);
    QGraphicsObject *obj2 = getAnimationObject(to);

    if (obj1 == nullptr || obj2 == nullptr || obj1 == obj2)
        return;

    QPointF start = obj1->sceneBoundingRect().center();
    QPointF finish = obj2->sceneBoundingRect().center();

    IndicatorItem *indicator = new IndicatorItem(start, finish, ClientInstance->findPlayer(from));

    qreal x = qMin(start.x(), finish.x());
    qreal y = qMin(start.y(), finish.y());
    indicator->setPos(x, y);
    indicator->setZValue(30000.0);

    addItem(indicator);
    indicator->doAnimation();
}

void RoomScene::doIndicate(const QString & /*unused*/, const QStringList &args)
{
    showIndicator(args.first(), args.last());
}

void RoomScene::doBattleArray(const QString & /*unused*/, const QStringList &args)
{
    QStringList names = args.last().split(QStringLiteral("+"));
    if (names.contains(Self->objectName()))
        dashboard->playBattleArrayAnimations();
    foreach (Photo *p, photos) {
        const Player *target = p->getPlayer();
        if (names.contains(target->objectName()))
            p->playBattleArrayAnimations();
    }
}

void RoomScene::doAnimation(int name, const QStringList &args)
{
    static QMap<AnimateType, AnimationFunc> map;
    if (map.isEmpty()) {
        map[S_ANIMATE_NULLIFICATION] = &RoomScene::doMovingAnimation;

        map[S_ANIMATE_FIRE] = &RoomScene::doAppearingAnimation;
        map[S_ANIMATE_LIGHTNING] = &RoomScene::doAppearingAnimation;

        map[S_ANIMATE_LIGHTBOX] = &RoomScene::doLightboxAnimation;
        map[S_ANIMATE_HUASHEN] = &RoomScene::doHuashen;
        map[S_ANIMATE_INDICATE] = &RoomScene::doIndicate;
        map[S_ANIMATE_BATTLEARRAY] = &RoomScene::doBattleArray;
    }

    static QMap<AnimateType, QString> anim_name;
    if (anim_name.isEmpty()) {
        anim_name[S_ANIMATE_NULLIFICATION] = QStringLiteral("nullification");

        anim_name[S_ANIMATE_FIRE] = QStringLiteral("fire");
        anim_name[S_ANIMATE_LIGHTNING] = QStringLiteral("lightning");

        anim_name[S_ANIMATE_LIGHTBOX] = QStringLiteral("lightbox");
        anim_name[S_ANIMATE_HUASHEN] = QStringLiteral("huashen");
        anim_name[S_ANIMATE_INDICATE] = QStringLiteral("indicate");
        anim_name[S_ANIMATE_BATTLEARRAY] = QStringLiteral("battlearray");
    }

    AnimationFunc func = map.value((AnimateType)name, NULL);
    if (func != nullptr)
        (this->*func)(anim_name.value((AnimateType)name, QString()), args);
}

void RoomScene::showServerInformation()
{
    QDialog *dialog = new QDialog(main_window);
    dialog->setWindowTitle(tr("Server information"));

    QHBoxLayout *layout = new QHBoxLayout;
    ServerInfoWidget *widget = new ServerInfoWidget;
    widget->fill(*(ClientInstance->serverInfo()), Config.HostAddress);
    layout->addWidget(widget);
    dialog->setLayout(layout);

    dialog->show();
}

void RoomScene::surrender()
{
    if (ClientInstance->getStatus() != Client::Playing) {
        QMessageBox::warning(main_window, tr("Warning"), tr("You can only initiate a surrender poll at your play phase!"));
        return;
    }

    QMessageBox::StandardButton button = QMessageBox::question(main_window, tr("Surrender"), tr("Are you sure to surrender ?"));
    if (button == QMessageBox::Ok || button == QMessageBox::Yes)
        ClientInstance->requestSurrender();
}

void RoomScene::fillGenerals1v1(const QStringList &names)
{
    int len = names.length() / 2;
    QString path = QStringLiteral("image/system/1v1/select%1.png").arg(len == 5 ? QString() : QStringLiteral("2"));
    selector_box = new QSanSelectableItem(path, true);
    selector_box->setPos(m_tableCenterPos);
    selector_box->setFlag(QGraphicsItem::ItemIsMovable);
    addItem(selector_box);
    selector_box->setZValue(10000);

    const static int start_x = 42 + G_COMMON_LAYOUT.m_cardNormalWidth / 2;
    const static int width = 86;
    const static int start_y = 59 + G_COMMON_LAYOUT.m_cardNormalHeight / 2;
    const static int height = 121;

    foreach (QString name, names) {
        CardItem *item = new CardItem(name);
        item->setObjectName(name);
        general_items << item;
    }

    qShuffle(general_items);

    int n = names.length();
    double scaleRatio = 116.0 / G_COMMON_LAYOUT.m_cardNormalHeight;
    for (int i = 0; i < n; i++) {
        int row = 0;
        int column = 0;
        if (i < len) {
            row = 1;
            column = i;
        } else {
            row = 2;
            column = i - len;
        }

        CardItem *general_item = general_items.at(i);
        general_item->scaleSmoothly(scaleRatio);
        general_item->setParentItem(selector_box);
        general_item->setPos(start_x + width * column, start_y + height * row);
        general_item->setHomePos(general_item->pos());
        general_item->setAcceptedMouseButtons(Qt::LeftButton);
    }
}

void RoomScene::fillGenerals3v3(const QStringList &names)
{
    QString temperature;
    if (Self->roleString().startsWith(QStringLiteral("l")))
        temperature = QStringLiteral("warm");
    else
        temperature = QStringLiteral("cool");

    QString path = QStringLiteral("image/system/3v3/select-%1.png").arg(temperature);
    selector_box = new QSanSelectableItem(path, true);
    selector_box->setFlag(QGraphicsItem::ItemIsMovable);
    addItem(selector_box);
    selector_box->setZValue(10000);
    selector_box->setPos(m_tableCenterPos);

    const static int start_x = 109;
    const static int width = 86;
    const static int row_y[4] = {150, 271, 394, 516};

    int n = names.length();
    double scaleRatio = 116.0 / G_COMMON_LAYOUT.m_cardNormalHeight;
    for (int i = 0; i < n; i++) {
        int row = 0;
        int column = 0;
        if (i < 8) {
            row = 1;
            column = i;
        } else {
            row = 2;
            column = i - 8;
        }

        CardItem *general_item = new CardItem(names.at(i));
        general_item->scaleSmoothly(scaleRatio);
        general_item->setParentItem(selector_box);
        general_item->setPos(start_x + width * column, row_y[row]);
        general_item->setHomePos(general_item->pos());
        general_item->setObjectName(names.at(i));
        general_item->setAcceptedMouseButtons(Qt::LeftButton);

        general_items << general_item;
    }
}

void RoomScene::fillGenerals(const QStringList &names)
{
    if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3"))
        fillGenerals3v3(names);
    else if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1"))
        fillGenerals1v1(names);
}

void RoomScene::bringToFront(QGraphicsItem *front_item)
{
    m_zValueMutex.lock();
    if (_m_last_front_item != nullptr)
        _m_last_front_item->setZValue(_m_last_front_ZValue);
    _m_last_front_item = front_item;
    _m_last_front_ZValue = front_item->zValue();
    if ((pindian_box != nullptr) && front_item != pindian_box && pindian_box->isVisible()) {
        m_zValueMutex.unlock();
        bringToFront(pindian_box);
        m_zValueMutex.lock();
        front_item->setZValue(9999);
    } else {
        front_item->setZValue(10000);
    }
    m_zValueMutex.unlock();
}

void RoomScene::takeGeneral(const QString &who, const QString &name, const QString &rule)
{
    bool self_taken = Self->roleString().startsWith(QStringLiteral("r"));
    if (who == QStringLiteral("warm"))
        self_taken = Self->roleString().startsWith(QStringLiteral("l"));

    QList<CardItem *> *to_add = self_taken ? &down_generals : &up_generals;

    CardItem *general_item = nullptr;
    foreach (CardItem *item, general_items) {
        if (item->objectName() == name) {
            general_item = item;
            break;
        }
    }

    Q_ASSERT(general_item);

    general_item->disconnect(this);
    general_items.removeOne(general_item);
    to_add->append(general_item);

    int x = 0;
    int y = 0;
    if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3")) {
        x = 63 + (to_add->length() - 1) * (148 - 62);
        y = self_taken ? 452 : 85;
    } else {
        x = 43 + (to_add->length() - 1) * 86;
        y = self_taken ? 60 + 120 * 3 : 60;
    }
    x = x + G_COMMON_LAYOUT.m_cardNormalWidth / 2;
    y = y + G_COMMON_LAYOUT.m_cardNormalHeight / 2;
    general_item->setHomePos(QPointF(x, y));
    general_item->goBack(true);

    if (((ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3") && Self->roleString() != QStringLiteral("lord") && Self->roleString() != QStringLiteral("renegade"))
         || (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1") && rule == QStringLiteral("OL")))
        && general_items.isEmpty()) {
        if (selector_box != nullptr) {
            selector_box->hide();
            delete selector_box;
            selector_box = nullptr;
        }
    }
}

void RoomScene::recoverGeneral(int index, const QString &name)
{
    QString obj_name = QStringLiteral("x%1").arg(index);
    foreach (CardItem *item, general_items) {
        if (item->objectName() == obj_name) {
            item->changeGeneral(name);
            break;
        }
    }
}

void RoomScene::startGeneralSelection()
{
    foreach (CardItem *item, general_items) {
        item->setFlag(QGraphicsItem::ItemIsFocusable);
        connect(item, &CardItem::double_clicked, this, &RoomScene::selectGeneral);
    }
}

void RoomScene::selectGeneral()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item != nullptr) {
        ClientInstance->replyToServer(S_COMMAND_ASK_GENERAL, item->objectName());
        foreach (CardItem *item, general_items) {
            item->setFlag(QGraphicsItem::ItemIsFocusable, false);
            item->disconnect(this);
        }
        ClientInstance->setStatus(Client::NotActive);
    }
}

void RoomScene::changeGeneral(const QString &general)
{
    if ((to_change != nullptr) && (arrange_button != nullptr))
        to_change->changeGeneral(general);
}

void RoomScene::revealGeneral(bool self, const QString &general)
{
    if (self)
        self_box->revealGeneral(general);
    else
        enemy_box->revealGeneral(general);
}

void RoomScene::trust()
{
    if (Self->getState() != QStringLiteral("trust"))
        doCancelButton();
    ClientInstance->trust();
}

void RoomScene::skillInvalidityChange(Player *player)
{
    if (player != Self)
        return;

    dashboard->updateSkillButton();
}

void RoomScene::startArrange(const QString &to_arrange)
{
    arrange_items.clear();
    QString mode;
    QList<QPointF> positions;
    if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3")) {
        mode = QStringLiteral("3v3");
        positions << QPointF(279, 356) << QPointF(407, 356) << QPointF(535, 356);
    } else if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("02_1v1")) {
        mode = QStringLiteral("1v1");
        if (down_generals.length() == 5)
            positions << QPointF(130, 335) << QPointF(260, 335) << QPointF(390, 335);
        else
            positions << QPointF(173, 335) << QPointF(303, 335) << QPointF(433, 335);
    }

    if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_XMode")) {
        QStringList arrangeList = to_arrange.split(QStringLiteral("+"));
        if (arrangeList.length() == 5)
            positions << QPointF(130, 335) << QPointF(260, 335) << QPointF(390, 335);
        else
            positions << QPointF(173, 335) << QPointF(303, 335) << QPointF(433, 335);
        QString path = QStringLiteral("image/system/XMode/arrange%1.png").arg((arrangeList.length() == 5) ? 1 : 2);
        selector_box = new QSanSelectableItem(path, true);
        selector_box->setFlag(QGraphicsItem::ItemIsMovable);
        selector_box->setPos(m_tableCenterPos);
        addItem(selector_box);
        selector_box->setZValue(10000);
    } else {
        QString suffix = (mode == QStringLiteral("1v1") && down_generals.length() == 6) ? QStringLiteral("2") : QString();
        QString path = QStringLiteral("image/system/%1/arrange%2.png").arg(mode, suffix);
        selector_box->load(path);
        selector_box->setPos(m_tableCenterPos);
    }

    if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_XMode")) {
        Q_ASSERT(!to_arrange.isNull());
        down_generals.clear();
        foreach (QString name, to_arrange.split(QStringLiteral("+"))) {
            CardItem *item = new CardItem(name);
            item->setObjectName(name);
            item->scaleSmoothly(116.0 / G_COMMON_LAYOUT.m_cardNormalHeight);
            item->setParentItem(selector_box);
            int x = 43 + down_generals.length() * 86;
            int y = 60 + 120 * 3;
            x = x + G_COMMON_LAYOUT.m_cardNormalWidth / 2;
            y = y + G_COMMON_LAYOUT.m_cardNormalHeight / 2;
            item->setPos(x, y);
            item->setHomePos(QPointF(x, y));
            down_generals << item;
        }
    }
    foreach (CardItem *item, down_generals) {
        item->setFlag(QGraphicsItem::ItemIsFocusable);
        item->setAutoBack(false);
        connect(item, &CardItem::released, this, &RoomScene::toggleArrange);
    }

    QRect rect(0, 0, 80, 120);

    foreach (QPointF pos, positions) {
        QGraphicsRectItem *rect_item = new QGraphicsRectItem(rect, selector_box);
        rect_item->setPos(pos);
        rect_item->setPen(Qt::NoPen);
        arrange_rects << rect_item;
    }

    arrange_button = new Button(tr("Complete"), 0.8);
    arrange_button->setParentItem(selector_box);
    arrange_button->setPos(600, 330);
    connect(arrange_button, &Button::clicked, this, &RoomScene::finishArrange);
}

void RoomScene::toggleArrange()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == nullptr)
        return;

    QGraphicsItem *arrange_rect = nullptr;
    int index = -1;
    for (int i = 0; i < 3; i++) {
        QGraphicsItem *rect = arrange_rects.at(i);
        if (item->collidesWithItem(rect)) {
            arrange_rect = rect;
            index = i;
        }
    }

    if (arrange_rect == nullptr) {
        if (arrange_items.contains(item)) {
            arrange_items.removeOne(item);
            down_generals << item;
        }
    } else {
        arrange_items.removeOne(item);
        down_generals.removeOne(item);
        arrange_items.insert(index, item);
    }

    int n = qMin(arrange_items.length(), 3);
    for (int i = 0; i < n; i++) {
        QPointF pos = arrange_rects.at(i)->pos();
        CardItem *item = arrange_items.at(i);
        item->setHomePos(pos);
        item->goBack(true);
    }

    while (arrange_items.length() > 3) {
        CardItem *last = arrange_items.takeLast();
        down_generals << last;
    }

    for (int i = 0; i < down_generals.length(); i++) {
        QPointF pos;
        if (ClientInstance->serverInfo()->GameModeStr == QStringLiteral("06_3v3"))
            pos = QPointF(65 + G_COMMON_LAYOUT.m_cardNormalWidth / 2 + i * 86, 452 + G_COMMON_LAYOUT.m_cardNormalHeight / 2);
        else
            pos = QPointF(43 + G_COMMON_LAYOUT.m_cardNormalWidth / 2 + i * 86, 60 + G_COMMON_LAYOUT.m_cardNormalHeight / 2 + 3 * 120);
        CardItem *item = down_generals.at(i);
        item->setHomePos(pos);
        item->goBack(true);
    }
}

void RoomScene::finishArrange()
{
    if (arrange_items.length() != 3)
        return;

    arrange_button->deleteLater();
    arrange_button = nullptr;

    QStringList names;
    foreach (CardItem *item, arrange_items)
        names << item->objectName();

    if (selector_box != nullptr) {
        selector_box->deleteLater();
        selector_box = nullptr;
    }
    arrange_rects.clear();

    ClientInstance->replyToServer(S_COMMAND_ARRANGE_GENERAL, QSgsJsonUtils::toJsonArray(names));
    ClientInstance->setStatus(Client::NotActive);
}

void RoomScene::showPindianBox(const QString &from_name, int from_id, const QString &to_name, int to_id, const QString &reason)
{
    pindian_box->setOpacity(0.0);
    pindian_box->setPos(m_tableCenterPos);
    if (!reason.isEmpty())
        pindian_box->setTitle(Sanguosha->translate(reason));
    else
        pindian_box->setTitle(tr("pindian"));

    if (pindian_from_card != nullptr) {
        delete pindian_from_card;
        pindian_from_card = nullptr;
    }
    if (pindian_to_card != nullptr) {
        delete pindian_to_card;
        pindian_to_card = nullptr;
    }

    pindian_from_card = new CardItem(ClientInstance->card(from_id));
    pindian_from_card->setParentItem(pindian_box);
    pindian_from_card->setPos(QPointF(28 + pindian_from_card->boundingRect().width() / 2, 44 + pindian_from_card->boundingRect().height() / 2));
    pindian_from_card->setFlag(QGraphicsItem::ItemIsMovable, false);
    pindian_from_card->setHomePos(pindian_from_card->pos());
    pindian_from_card->setFootnote(ClientInstance->getPlayerName(from_name));

    pindian_to_card = new CardItem(ClientInstance->card(to_id));
    pindian_to_card->setParentItem(pindian_box);
    pindian_to_card->setPos(QPointF(126 + pindian_to_card->boundingRect().width() / 2, 44 + pindian_to_card->boundingRect().height() / 2));
    pindian_to_card->setFlag(QGraphicsItem::ItemIsMovable, false);
    pindian_to_card->setHomePos(pindian_to_card->pos());
    pindian_to_card->setFootnote(ClientInstance->getPlayerName(to_name));

    bringToFront(pindian_box);
    pindian_box->appear();
    QTimer::singleShot(500ms, this, &RoomScene::doPindianAnimation);
}

void RoomScene::doPindianAnimation()
{
    if (!pindian_box->isVisible() || (pindian_from_card == nullptr) || (pindian_to_card == nullptr))
        return;

    QString emotion = pindian_success ? QStringLiteral("success") : QStringLiteral("no-success");
    PixmapAnimation *pma = PixmapAnimation::GetPixmapAnimation(pindian_from_card, emotion);
    if (pma != nullptr)
        connect(pma, &PixmapAnimation::finished, pindian_box, &Window::disappear);
    else
        pindian_box->disappear();
}

static inline void AddRoleIcon(QMap<QChar, QPixmap> &map, char c, const QString &role)
{
    QPixmap pixmap(QStringLiteral("image/system/roles/small-%1.png").arg(role));

    QChar qc = QLatin1Char(c);
    map[qc.toUpper()] = pixmap;

    QSanUiUtils::makeGray(pixmap);
    map[qc.toLower()] = pixmap;
}

void RoomScene::updateRoles(const QString &roles)
{
    foreach (QGraphicsItem *item, role_items)
        removeItem(item);

    role_items.clear();

    static QMap<QChar, QPixmap> map;
    if (map.isEmpty()) {
        AddRoleIcon(map, 'Z', QStringLiteral("lord"));
        AddRoleIcon(map, 'C', QStringLiteral("loyalist"));
        AddRoleIcon(map, 'F', QStringLiteral("rebel"));
        AddRoleIcon(map, 'N', QStringLiteral("renegade"));
    }

    foreach (QChar c, roles) {
        if (map.contains(c)) {
            QGraphicsPixmapItem *item = addPixmap(map.value(c));
            role_items << item;
        }
    }
    updateRolesBox();
}

void RoomScene::updateRolesBox()
{
    double centerX = m_rolesBox->boundingRect().width() / 2;
    int n = role_items.length();
    for (int i = 0; i < n; i++) {
        QGraphicsPixmapItem *item = role_items[i];
        item->setParentItem(m_rolesBox);
        item->setPos(21 * (i - n / 2) + centerX, 6);
    }
    m_pileCardNumInfoTextBox->setTextWidth(m_rolesBox->boundingRect().width());
    m_pileCardNumInfoTextBox->setPos(0, 35);
}

void RoomScene::appendChatEdit(const QString &txt)
{
    chat_edit->setText(chat_edit->text() + QStringLiteral(" ") + txt);
    chat_edit->setFocus();
}

void RoomScene::appendChatBox(QString txt)
{
    QString prefix = QStringLiteral("<img src='image/system/chatface/");
    QString suffix = QStringLiteral(".png'></img>");
    txt = txt.replace(QStringLiteral("<#"), prefix).replace(QStringLiteral("#>"), suffix);
    chat_box->append(txt);
}

void RoomScene::setChatBoxVisible(bool show)
{
    if (!show) {
        chat_box_widget->hide();
        chat_edit->hide();
        chat_widget->hide();
        log_box->resize(_m_infoPlane.width(),
                        _m_infoPlane.height() * (_m_roomLayout->m_logBoxHeightPercentage + _m_roomLayout->m_chatBoxHeightPercentage) + _m_roomLayout->m_chatTextBoxHeight);
    } else {
        chat_box_widget->show();
        chat_edit->show();
        chat_widget->show();
        log_box->resize(_m_infoPlane.width(), _m_infoPlane.height() * _m_roomLayout->m_logBoxHeightPercentage);
    }
}

HeroSkinContainer *RoomScene::findHeroSkinContainer(const QString &generalName) const
{
    foreach (HeroSkinContainer *heroSkinContainer, m_heroSkinContainers) {
        if (heroSkinContainer->getGeneralName() == generalName) {
            return heroSkinContainer;
        }
    }

    return nullptr;
}

void RoomScene::addHeroSkinContainer(Player *player, HeroSkinContainer *heroSkinContainer)
{
    m_heroSkinContainers.insert(heroSkinContainer);

    QList<PlayerCardContainer *> playerCardContainers;
    foreach (Photo *photo, photos) {
        if (photo->getPlayer() == player) {
            continue;
        }
        playerCardContainers.append(photo);
    }
    if (dashboard->getPlayer() != player) {
        playerCardContainers.append(dashboard);
    }

    foreach (PlayerCardContainer *playerCardContainer, playerCardContainers) {
        const Player *player = playerCardContainer->getPlayer();
        const QString &heroSkinGeneralName = heroSkinContainer->getGeneralName();

        if (heroSkinGeneralName == player->generalName()) {
            connect(heroSkinContainer, &HeroSkinContainer::local_skin_changed, playerCardContainer->getAvartarItem(), &GraphicsPixmapHoverItem::startChangeHeroSkinAnimation);

            connect(heroSkinContainer, &HeroSkinContainer::skin_changed, this, &RoomScene::doSkinChange);
        }

        if (heroSkinGeneralName == player->getGeneral2Name()) {
            connect(heroSkinContainer, &HeroSkinContainer::local_skin_changed, playerCardContainer->getSmallAvartarItem(), &GraphicsPixmapHoverItem::startChangeHeroSkinAnimation);
            connect(heroSkinContainer, &HeroSkinContainer::skin_changed, this, &RoomScene::doSkinChange);
        }
    }
}
QSet<HeroSkinContainer *> RoomScene::getHeroSkinContainers()
{
    return m_heroSkinContainers;
}

Client *RoomScene::getClient() const
{
    return client;
}

void RoomScene::setCurrentViewAsSkillSelectionChain(const QStringList &chain)
{
    vschain = chain;
}

QStringList RoomScene::currentViewAsSkillSelectionChain() const
{
    return vschain;
}

void RoomScene::doSkinChange(const QString &generalName, int skinIndex)
{
    ClientInstance->changeSkin(generalName, skinIndex);
}

void RoomScene::showBubbleChatBox(const QString &who, const QString &line)
{
    if (Config.BubbleChatBoxDelaySeconds > 0) {
        if (!m_bubbleChatBoxs.contains(who)) {
            BubbleChatBox *bubbleChatBox = new BubbleChatBox(getBubbleChatBoxShowArea(who));
            addItem(bubbleChatBox);
            bubbleChatBox->setZValue(INT_MAX);
            m_bubbleChatBoxs.insert(who, bubbleChatBox);
        }

        m_bubbleChatBoxs[who]->setText(line);
    }
}

const QSize BUBBLE_CHAT_BOX_SHOW_AREA_SIZE(138, 64);
QRect RoomScene::getBubbleChatBoxShowArea(const QString &who) const
{
    Photo *photo = name2photo.value(who, NULL);
    if (photo != nullptr) {
        QRectF rect = photo->sceneBoundingRect();
        return QRect(QPoint(rect.left() + 18, rect.top() + 26), BUBBLE_CHAT_BOX_SHOW_AREA_SIZE);
    } else {
        QRectF rect = dashboard->getAvatarAreaSceneBoundingRect();
        return QRect(QPoint(rect.left() + 8, rect.top() + 52), BUBBLE_CHAT_BOX_SHOW_AREA_SIZE);
    }
}

void RoomScene::highlightSkillButton(const QString &skill_name, bool highlight)
{
    if (isHegemonyGameMode(ClientInstance->serverInfo()->GameModeStr))
        return;
    if (skill_name.isNull() || skill_name.isEmpty())
        return;
    foreach (QSanSkillButton *button, m_skillButtons) {
        QString button_name = button->getSkill()->name();
        if (button_name == skill_name || skill_name.startsWith(button_name)) {
            if (button->getSkill()->isCompulsory() && button->getSkill()->isLimited()) {
                if (!button->isDown()) {
                    if (highlight)
                        button->setState(QSanButton::S_STATE_HOVER, true);
                    else
                        button->setState(QSanButton::S_STATE_UP, true);
                }
            }
        }
    }
}

bool RoomScene::isHighlightStatus(Client::Status status)
{
    switch (status & Client::ClientStatusBasicMask) {
    case Client::AskForSkillInvoke: {
        return true;
    }
    case Client::AskForPlayerChoose: {
        return true;
    }
    case Client::Exchanging: {
        return true;
    }
    case Client::AskForAG: {
        return true;
    }
    case Client::AskForYiji: {
        return true;
    }
    case Client::AskForGuanxing: {
        return true;
    }
    case Client::AskForGongxin: {
        return true;
    }
    case Client::ExecDialog: {
        return true;
    }
    case Client::AskForChoice: {
        return true;
    }
    case Client::RespondingUse: {
        return true;
    }
    case Client::Responding: {
        return true;
    }
    case Client::Discarding: {
        return true;
    }
    default:
        break;
    }
    return false;
}

void RoomScene::setLordBGM(const QString &lord)
{
    if (!Config.EnableBgMusic)
        return;
    Audio::stopBGM();
    bool changeBGM = Config.value(QStringLiteral("UseLordBGM"), true).toBool();
    //intialize default path
    bgm_path = Config.value(QStringLiteral("BackgroundMusic"), QStringLiteral("audio/title/main.ogg")).toString();
    QString lord_name = (lord.isNull()) ? ClientInstance->lord_name : lord;
    if (lord_name.isNull())
        lord_name = Self->generalName();
    lord_name = lord_name.split(QStringLiteral("_")).at(0);
    if (changeBGM) { //change BGMpath to lordName
        bgm_path = QStringLiteral("audio/bgm/") + lord_name + QStringLiteral("_1.ogg");
        if (!(bgm_path.isNull()) || !QFile::exists(bgm_path)) {
            foreach (QString cv_pair, LordBGMConvertList) {
                bool shouldBreak = false;
                QStringList pairs = cv_pair.split(QStringLiteral("->"));
                QStringList cv_from = pairs.at(0).split(QStringLiteral("|"));
                foreach (QString from, cv_from) {
                    if (from == lord_name) {
                        lord_name = pairs.at(1).split(QStringLiteral("_")).at(0);
                        bgm_path = QStringLiteral("audio/bgm/") + lord_name + QStringLiteral("_1.ogg");
                        shouldBreak = true;
                        break;
                    }
                }
                if (shouldBreak)
                    break;
            }
        }
    }

    if (!QFile::exists(bgm_path))
        Audio::playBGM(Audio::getBgmFileNames(QString(), true));
    else
        Audio::playBGM(Audio::getBgmFileNames(lord_name, false));
    Audio::setBGMVolume(Config.BGMVolume);
}

void RoomScene::setLordBackdrop(const QString &lord)
{
    bool changeBackdrop = Config.value(QStringLiteral("UseLordBackdrop"), true).toBool();
    //intialize default path
    image_path = Config.TableBgImage;
    QString lord_name = (lord.isNull()) ? ClientInstance->lord_name : lord;
    if (lord_name.isNull())
        lord_name = Self->generalName();
    if (lord_name.endsWith(QStringLiteral("_hegemony")))
        lord_name = lord_name.replace(QStringLiteral("_hegemony"), QString());
    if (changeBackdrop) {
        image_path = QStringLiteral("backdrop/") + lord_name + QStringLiteral(".jpg");
        if ((image_path.isNull()) || !QFile::exists(image_path)) {
            foreach (QString cv_pair, LordBackdropConvertList) {
                bool shouldBreak = false;
                QStringList pairs = cv_pair.split(QStringLiteral("->"));
                QStringList cv_from = pairs.at(0).split(QStringLiteral("|"));
                foreach (QString from, cv_from) {
                    if (from == lord_name) {
                        image_path = QStringLiteral("backdrop/") + pairs.at(1) + QStringLiteral(".jpg");
                        shouldBreak = true;
                        break;
                    }
                }
                if (shouldBreak)
                    break;
            }
        }
    }
    if (!image_path.isNull() && QFile::exists(image_path))
        changeTableBg(image_path);
}

CommandLinkDoubleClickButton::CommandLinkDoubleClickButton(QWidget *parent)
    : QCommandLinkButton(parent)
{
}

CommandLinkDoubleClickButton::CommandLinkDoubleClickButton(const QString &text, QWidget *parent)
    : QCommandLinkButton(text, parent)
{
}

CommandLinkDoubleClickButton::CommandLinkDoubleClickButton(const QString &text, const QString &description, QWidget *parent)
    : QCommandLinkButton(text, description, parent)
{
}

void CommandLinkDoubleClickButton::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit double_clicked(QPrivateSignal());
    QCommandLinkButton::mouseDoubleClickEvent(event);
}

void RoomScene::anyunSelectSkill()
{
    static QStringList selectAllList {QStringLiteral("qiji")};
    QString skillName = Self->tag[QStringLiteral("anyun")].toString();
    if (selectAllList.contains(skillName))
        dashboard->selectAll();
}

//for client test log
void RoomScene::addlog(const QStringList &log)
{
    log_box->appendLog(log);
}
