#ifndef _PHOTO_H
#define _PHOTO_H

#include "GenericCardContainerUI.h"
#include "QSanSelectableItem.h"
#include "carditem.h"
#include "pixmapanimation.h"
#include "player.h"
#include "protocol.h"
#include "sprite.h"

#include <QComboBox>
#include <QGraphicsObject>
#include <QPixmap>

class Player;
class RoleComboBox;
class HegemonyRoleComboBox;
class QPushButton;

class QPropertyAnimation;

class Photo : public PlayerCardContainer
{
    Q_OBJECT

public:
    explicit Photo();
    const Player *getPlayer() const;
    void speak(const QString &content);
    void repaintAll() override;
    QList<CardItem *> removeCardItems(const QList<int> &card_id, QSanguosha::Place place) override;

    void setEmotion(const QString &emotion, bool permanent = false);
    void tremble();

    enum FrameType
    {
        S_FRAME_PLAYING,
        S_FRAME_RESPONDING,
        S_FRAME_SOS,
        S_FRAME_NO_FRAME
    };

    void setFrame(FrameType type);
    QRectF boundingRect() const override;
    QGraphicsItem *getMouseClickReceiver() override;
    void playBattleArrayAnimations();

public slots:
    void updatePhase();
    void hideEmotion();
    void refresh() override;

protected:
    inline QGraphicsItem *_getEquipParent() override
    {
        return _m_groupMain;
    }
    inline QGraphicsItem *_getDelayedTrickParent() override
    {
        return _m_groupMain;
    }
    inline QGraphicsItem *_getAvatarParent() override
    {
        return _m_groupMain;
    }
    inline QGraphicsItem *_getMarkParent() override
    {
        return _m_floatingArea;
    }
    inline QGraphicsItem *_getPhaseParent() override
    {
        return _m_groupMain;
    }
    inline QGraphicsItem *_getRoleComboBoxParent() override
    {
        return _m_groupMain;
    }
    inline QGraphicsItem *_getProgressBarParent() override
    {
        return this;
    }
    inline QGraphicsItem *_getFocusFrameParent() override
    {
        return _m_groupMain;
    }
    inline QGraphicsItem *_getDeathIconParent() override
    {
        return _m_groupDeath;
    }
    QGraphicsItem *_getPileParent() override
    {
        return _m_groupMain;
    }
    inline QString getResourceKeyName() override
    {
        return QString::fromUtf8(QSanRoomSkin::S_SKIN_KEY_PHOTO);
    }
    inline QAbstractAnimation *_getPlayerRemovedEffect() override
    {
        return _blurEffect;
    }

    QPointF getHeroSkinContainerPosition() const override;

    void _adjustComponentZValues(bool killed = false) override;
    bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo) override;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    QPropertyAnimation *initializeBlurEffect(GraphicsPixmapHoverItem *icon);
    void _initializeRemovedEffect() override;

    FrameType _m_frameType;
    QGraphicsPixmapItem *_m_mainFrame;
    Sprite *emotion_item;
    QGraphicsPixmapItem *_m_focusFrame;
    QGraphicsPixmapItem *_m_onlineStatusItem;
    QParallelAnimationGroup *_blurEffect;

    QHash<QString, PixmapAnimation *> _m_frameBorders;
    QHash<QString, PixmapAnimation *> _m_roleBorders;
    void _createBattleArrayAnimations();
};

#endif
