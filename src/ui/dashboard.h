#ifndef _DASHBOARD_H
#define _DASHBOARD_H

#include "GenericCardContainerUI.h"
#include "QSanSelectableItem.h"
#include "TimedProgressBar.h"
#include "carditem.h"
#include "pixmapanimation.h"
#include "player.h"
#include "protocol.h"
#include "qsanbutton.h"
#include "skill.h"
#include "sprite.h"

#include <QComboBox>
#include <QGraphicsLinearLayout>
#include <QLineEdit>
#include <QMutex>
#include <QPropertyAnimation>
#include <QPushButton>

#ifdef Q_OS_WIN
class QWinTaskbarButton;
#endif

class Dashboard : public PlayerCardContainer
{
    Q_OBJECT
    Q_ENUMS(SortType)

public:
    enum SortType
    {
        ByType,
        BySuit,
        ByNumber
    };

    explicit Dashboard(QGraphicsItem *button_widget);
    //Dashboard(QGraphicsPixmapItem *button_widget);
    QRectF boundingRect() const override;
    void refresh() override;
    //void repaintAll();
    void setWidth(int width);
    int getMiddleWidth();
    inline QRectF getAvatarArea()
    {
        QRectF rect;
        QRect avatarArea = (ServerInfo.Enable2ndGeneral) ? _dlayout->m_avatarAreaDouble : _dlayout->m_avatarArea;
        rect.setSize(avatarArea.size());
        QPointF topLeft = mapFromItem(_getAvatarParent(), avatarArea.topLeft());
        rect.moveTopLeft(topLeft);
        return rect;
    }

    void hideControlButtons();
    void showControlButtons();

    void setPlayer(ClientPlayer *player) override; //hegemony
    void showSeat() override; //hegemony

    void showProgressBar(QSanProtocol::Countdown countdown) override;
    void hideProgressBar() override;

    QRectF getAvatarAreaSceneBoundingRect() const
    {
        return _m_rightFrame->sceneBoundingRect();
    }
    QSanSkillButton *removeSkillButton(const QString &skillName, bool head);
    QSanSkillButton *addSkillButton(const QString &skillName, const bool &head = true);
    bool isAvatarUnderMouse();

    void highlightEquip(QString skillName, bool highlight);

    void setTrust(bool trust);
    void killPlayer() override;
    void revivePlayer() override;
    virtual void setDeathColor();
    void selectCard(const QString &pattern, bool forward = true, bool multiple = false);
    void selectEquip(int position);
    void selectOnlyCard(bool need_only = false);
    void useSelected();
    const Card *getSelected() const;
    void unselectAll(const CardItem *except = nullptr);
    void hideAvatar();

    void disableAllCards();
    void enableCards();
    void enableAllCards();

    void adjustCards(bool playAnimation = true);

    QGraphicsItem *getMouseClickReceiver() override;

    QList<CardItem *> removeCardItems(const QList<int> &card_ids, Player::Place place) override;
    QList<CardItem *> cloneCardItems(QList<int> card_ids) override;

    // pending operations
    void startPending(const ViewAsSkill *skill);
    void stopPending();
    void updatePending();
    const ViewAsSkill *currentSkill() const;
    const Card *pendingCard() const;

    void expandPileCards(const QString &pile_name);
    void expandSpecialCard();
    void retractPileCards(const QString &pile_name);
    void retractSpecialCard();
    inline QStringList getPileExpanded() const
    {
        return _m_pile_expanded.keys();
    }

    void selectCard(CardItem *item, bool isSelected);

    int getButtonWidgetWidth() const;
    int getTextureWidth() const;

    int width();
    int height();

    void showNullificationButton();
    void hideNullificationButton();

    static const int S_PENDING_OFFSET_Y = -25;

    inline void updateSkillButton()
    {
        if (_m_skillDock != nullptr)
            _m_skillDock->update();
        if (_m_rightSkillDock != nullptr)
            _m_rightSkillDock->update();
    }

    void playBattleArrayAnimations();
    static const int CARDITEM_Z_DATA_KEY = 0413;

public slots:
    virtual void updateAvatar();
    virtual void updateSmallAvatar();
    void updateChaoren();
    void updateShown();
    void updateHiddenMark();
    void updateRightHiddenMark(); //hegemony

    void sortCards();
    void beginSorting();
    void reverseSelection();
    void cancelNullification();
    void skillButtonActivated();
    void skillButtonDeactivated();
    void selectAll();
    void controlNullificationButton(bool show);
    void updateHandPile();

    void selectLingshou();

#ifdef Q_OS_WIN
    void updateTimedProgressBar(time_t val, time_t max);
#endif
    void onCardItemHover();
    void onCardItemLeaveHover();

protected:
    void _createExtraButtons();
    void _adjustComponentZValues(bool killed = false) override;
    virtual void addHandCards(QList<CardItem *> &cards);
    virtual QList<CardItem *> removeHandCards(const QList<int> &cardIds);

    // initialization of _m_layout is compulsory for children classes.
    inline QGraphicsItem *_getEquipParent() override
    {
        return _m_leftFrame;
    }
    inline QGraphicsItem *_getDelayedTrickParent() override
    {
        return _m_leftFrame;
    }
    inline QGraphicsItem *_getAvatarParent() override
    {
        return _m_rightFrame;
    }
    inline QGraphicsItem *_getMarkParent() override
    {
        return _m_floatingArea;
    }
    inline QGraphicsItem *_getPhaseParent() override
    {
        return _m_floatingArea;
    }
    inline QGraphicsItem *_getRoleComboBoxParent() override
    {
        return _m_rightFrame;
    }
    inline QGraphicsItem *_getPileParent() override
    {
        return _m_rightFrame;
    }
    inline QGraphicsItem *_getProgressBarParent() override
    {
        return _m_floatingArea;
    }
    inline QGraphicsItem *_getFocusFrameParent() override
    {
        return _m_rightFrame;
    }
    inline QGraphicsItem *_getDeathIconParent() override
    {
        return _m_middleFrame;
    }
    inline QString getResourceKeyName() override
    {
        return QSanRoomSkin::S_SKIN_KEY_DASHBOARD;
    }
    inline QAbstractAnimation *_getPlayerRemovedEffect() override
    {
        return _removedEffect;
    }
    QPointF getHeroSkinContainerPosition() const override;
    //virtual const QSanShadowTextFont &getSkillNameFont() const {
    //    return G_DASHBOARD_LAYOUT.m_skillNameFont;
    //}
    //virtual const QRect &getSkillNameArea() const { return G_DASHBOARD_LAYOUT.m_skillNameArea; }

    bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

    //virtual bool sceneEventFilter(QGraphicsItem *watched, QEvent *event);

    //void _addHandCard(CardItem *card_item);
    void _addHandCard(CardItem *card_item, bool prepend = false, const QString &footnote = QString());
    void _adjustCards();
    void _adjustCards(const QList<CardItem *> &list, int y);

    int _m_width;
    // sync objects
    QMutex m_mutex;
    QMutex m_mutexEnableCards;

    QSanButton *m_btnReverseSelection;
    QSanButton *m_btnSortHandcard;
    QSanButton *m_btnNoNullification;
    QGraphicsPixmapItem *_m_leftFrame, *_m_middleFrame, *_m_rightFrame;
    // we can not draw bg directly _m_rightFrame because then it will always be
    // under avatar (since it's avatar's parent).
    QGraphicsPixmapItem *_m_rightFrameBg;
    QGraphicsItem *button_widget;

    CardItem *selected;
    QList<CardItem *> m_handCards;

    QGraphicsRectItem *trusting_item;
    QGraphicsSimpleTextItem *trusting_text;

    QSanInvokeSkillDock *_m_skillDock;
    QSanInvokeSkillDock *_m_rightSkillDock; //hegemony
    const QSanRoomSkin::DashboardLayout *_dlayout;

    //for animated effects
    EffectAnimation *animations;

    QGraphicsRectItem *_m_shadow_layer1, *_m_shadow_layer2; //hegemony  //for avatar shadow layer
    QGraphicsPixmapItem *leftHiddenMark;
    QGraphicsPixmapItem *rightHiddenMark; //hegemony

    // for parts creation
    void _createLeft();
    void _createRight();
    void _createMiddle();
    void _updateFrames();

    // for pendings
    QList<CardItem *> pendings;
    const Card *pending_card;
    const ViewAsSkill *view_as_skill;
    const FilterSkill *filter;
    //QStringList _m_pile_expanded;
    QMap<QString, QList<int>> _m_pile_expanded;
    QList<int> _m_id_expanded; //just for chaoren

    // for equip skill/selections
    PixmapAnimation *_m_equipBorders[5];
    QSanSkillButton *_m_equipSkillBtns[5];
    bool _m_isEquipsAnimOn[5];
    QList<QSanSkillButton *> _m_button_recycle;

    void _createEquipBorderAnimations();
    void _setEquipBorderAnimation(int index, bool turnOn);

    void drawEquip(QPainter *painter, const CardItem *equip, int order);
    void setSelectedItem(CardItem *card_item);

    // for battle array
    QHash<QString, PixmapAnimation *> _m_frameBorders;
    QHash<QString, PixmapAnimation *> _m_roleBorders;
    void _createBattleArrayAnimations();

    void _initializeRemovedEffect() override;
    QPropertyAnimation *_removedEffect;

    QMenu *_m_sort_menu;
    //QMenu *_m_carditem_context_menu;

    QList<CardItem *> _m_cardItemsAnimationFinished;
    QMutex m_mutexCardItemsAnimationFinished;

#ifdef Q_OS_WIN
    QWinTaskbarButton *taskbarButton;
#endif

protected slots:
    void _onEquipSelectChanged() override;

    void onAnimationFinished() override;

    void onAvatarHoverEnter() override;
    void doAvatarHoverLeave() override
    {
        _m_screenNameItem->hide();
    }

    bool isItemUnderMouse(QGraphicsItem *item) const override;

private slots:
    void onCardItemClicked();

    //void onCardItemContextMenu();

    void onCardItemDoubleClicked();
    void onCardItemThrown();

    void onMarkChanged();
    void onHeadStateChanged();
    void onDeputyStateChanged();

    void bringSenderToTop();
    void resetSenderZValue();

signals:
    void card_selected(const Card *card);
    void card_to_use();
    void progressBarTimedOut();
};

#endif
