#ifndef _SKIN_BANK_H
#define _SKIN_BANK_H

#define QSAN_UI_LIBRARY_AVAILABLE

#include "card.h"
#include "json.h"
#include "qsanbutton.h"

#include <QAbstractAnimation>
#include <QFont>
#include <QGraphicsPixmapItem>
#include <QHash>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <QString>

class IQSanComponentSkin
{ // interface class
public:
    class QSanSimpleTextFont
    {
    public:
        QString m_family_name;
        QSize m_fontSize;
        int m_spacing;
        int m_weight;
        QColor m_color;
        bool m_vertical;
        QSanSimpleTextFont();
        bool tryParse(const QVariant &arg);
        void paintText(QPainter *painter, QRect pos, Qt::Alignment align, const QString &text) const;
        // this function's prototype is confusing. It will CLEAR ALL contents on the
        // QGraphicsPixmapItem passed in and then start drawing.
        void paintText(QGraphicsPixmapItem *item, QRect pos, Qt::Alignment align, const QString &text) const;

    protected:
        static QHash<QString, QString> _m_fontBank;
    };

    class QSanShadowTextFont : public QSanSimpleTextFont
    {
    public:
        int m_shadowRadius;
        double m_shadowDecadeFactor;
        QPoint m_shadowOffset;
        QColor m_shadowColor;
        bool tryParse(const QVariant &arg);
        void paintText(QPainter *painter, QRect pos, Qt::Alignment align, const QString &text) const;
        // this function's prototype is confusing. It will CLEAR ALL contents on the
        // QGraphicsPixmapItem passed in and then start drawing.
        void paintText(QGraphicsPixmapItem *item, QRect pos, Qt::Alignment align, const QString &text) const;
    };

    class AnchoredRect
    {
    public:
        QRect getTranslatedRect(QRect parentRect) const;
        QRect getTranslatedRect(QRect parentRect, QSize childSize) const;
        bool tryParse(const QVariant &value);

    protected:
        Qt::Alignment m_anchorChild;
        Qt::Alignment m_anchorParent;
        QPoint m_offset;
        QSize m_fixedSize;
        bool m_useFixedSize;
    };

    static const char *S_SKIN_KEY_DEFAULT;
    static const char *S_SKIN_KEY_DEFAULT_SECOND;
    bool load(const QString &layoutConfigFileName, const QString &imageConfigFileName, const QString &audioConfigFileName, const QString &animationConfigFileName);
    QPixmap getPixmap(const QString &key, const QString &arg = QString(), bool cache = false, bool heroSkin = true) const;
    QPixmap getPixmapFileName(const QString &key) const;
    QPixmap getPixmapFromFileName(const QString &fileName, bool cache = false) const;
    QStringList getAudioFileNames(const QString &key) const;
    QString getRandomAudioFileName(const QString &key) const;
    bool isImageKeyDefined(const QString &key) const;
    QStringList getAnimationFileNames() const;

protected:
    virtual bool _loadLayoutConfig(const QVariant &config) = 0;
    virtual bool _loadImageConfig(const QVariant &config);
    virtual bool _loadAnimationConfig(const QVariant &config) = 0;
    QString _readConfig(const QVariant &dictionary, const QString &key, const QString &defaultValue = QString()) const;
    QString _readImageConfig(const QString &key, QRect &clipRegion, bool &clipping, QSize &newScale, bool &scaled, const QString &defaultValue = QString()) const;

    JsonObject _m_imageConfig;
    JsonObject _m_audioConfig;
    JsonObject _m_animationConfig;
};

class QSanRoomSkin : public IQSanComponentSkin
{
public:
    struct RoomLayout
    {
        int m_scenePadding;
        int m_roleBoxHeight;
        int m_chatTextBoxHeight;
        int m_discardPileMinWidth;
        int m_discardPilePadding;
        double m_logBoxHeightPercentage;
        double m_chatBoxHeightPercentage;
        double m_infoPlaneWidthPercentage;
        double m_photoDashboardPadding;
        double m_photoRoomPadding;
        int m_photoHDistance;
        int m_photoVDistance;
        QSize m_minimumSceneSize;
        QSize m_maximumSceneSize;
        QSize m_minimumSceneSize10Player;
        QSize m_maximumSceneSize10Player;
        double scale;
    };

    struct PlayerCardContainerLayout
    {
        int m_normalHeight;
        QRect m_boundingRect;
        QRect m_focusFrameArea;
        QRect m_focusFrameAreaDouble;
        QRect m_handCardArea;

        // equips
        QRect m_equipAreas[5];
        QRect m_equipImageArea;
        QRect m_equipTextArea;
        QRect m_equipSuitArea;
        QRect m_equipDistanceArea;
        QRect m_equipPointArea;
        QRect m_equipTianyiArea;
        QSanShadowTextFont m_equipFont;
        QSanShadowTextFont m_equipPointFontBlack;
        QSanShadowTextFont m_equipPointFontRed;

        // delayed trick area
        QRect m_delayedTrickFirstRegion;
        QPoint m_delayedTrickStep;

        AnchoredRect m_markTextArea;
        QPoint m_roleComboBoxPos;
        QRect m_roleShownArea;

        QPoint m_changePrimaryHeroSkinBtnPos;
        QPoint m_changeSecondaryHeroSkinBtnPos;

        // photo area
        QRect m_avatarArea;
        QRect m_headAvatarArea;
        QRect m_avatarAreaDouble; // hegemony head
        int m_avatarSize;
        QRect m_smallAvatarArea;
        int m_smallAvatarSize;
        int m_primaryAvatarSize;
        QRect m_circleArea;
        int m_circleImageSize;
        QRect m_avatarNameArea;
        QRect m_headAvatarNameArea;
        QRect m_smallAvatarNameArea;
        QSanShadowTextFont m_avatarNameFont;
        QSanShadowTextFont m_smallAvatarNameFont;
        QRect m_kingdomIconArea;
        QRect m_kingdomIconAreaDouble;
        QRect m_kingdomMaskArea;
        QRect m_dashboardKingdomMaskArea;
        QRect m_dashboardPrimaryKingdomMaskArea;
        QRect m_dashboardSecondaryKingdomMaskArea;
        QSanShadowTextFont m_handCardFont;
        QRect m_screenNameArea;
        QRect m_screenNameAreaDouble;
        QSanShadowTextFont m_screenNameFont;

        QRect leftDisableShowLockArea;
        QRect rightDisableShowLockArea;

        // progress bar and other controls
        bool m_isProgressBarHorizontal;
        AnchoredRect m_progressBarArea;
        QSize m_magatamaSize;
        QRect m_magatamaImageArea;
        QRect m_sub_magatamaImageArea;
        bool m_magatamasHorizontal;
        bool m_magatamasBgVisible;
        QPoint m_magatamasAnchor;
        QPoint m_magatamasAnchorDouble;
        QPoint m_sub_magatamasAnchor;
        Qt::Alignment m_magatamasAlign;
        Qt::Alignment m_magatamasAlignDouble;
        Qt::Alignment m_sub_magatamasAlign;
        AnchoredRect m_phaseArea;

        // private pile (e.g. 7 stars, buqu)
        QPoint m_privatePileStartPos;
        QPoint m_privatePileStartPosDouble;
        QPoint m_privatePileStep;
        QSize m_privatePileButtonSize;

        // various icons
        QRect m_actionedIconRegion;
        QRect m_saveMeIconRegion;
        QRect m_chainedIconRegion;
        AnchoredRect m_deathIconRegion;
        QRect m_votesIconRegion;
        QRect m_hiddenMarkRegion1, m_hiddenMarkRegion2, m_hiddenMarkRegion3; //hegemony
        QRect m_seatIconRegion;
        QRect m_seatIconRegionDouble; //hegemony
        QColor m_drankMaskColor;
        QColor m_duanchangMaskColor;
        QColor m_deathEffectColor;
        QColor m_generalShadowColor; //hegemony

        QRect m_extraSkillArea;
        QSanShadowTextFont m_extraSkillFont;
        QRect m_extraSkillTextArea;
    };

    struct PhotoLayout : public PlayerCardContainerLayout
    {
        int m_normalWidth;
        QRect m_mainFrameArea;
        QRect m_cardMoveRegion;
        QRect m_onlineStatusArea;
        QSanShadowTextFont m_onlineStatusFont;
        QColor m_onlineStatusBgColor;
        QRect m_skillNameArea;
        QSanShadowTextFont m_skillNameFont;
    };

    struct DashboardLayout : public PlayerCardContainerLayout
    {
        int m_leftWidth, m_rightWidth;
        int m_rightWidthDouble; //hegemony
        int m_floatingAreaHeight;
        int m_rswidth;
        QSize m_buttonSetSize;
        QRect m_confirmButtonArea;
        QRect m_cancelButtonArea;
        QRect m_discardButtonArea;
        QRect m_trustButtonArea;
        QSize m_skillButtonsSize[3];
        QRect m_skillTextArea[3];
        QRect m_skillTextAreaDown[3];
        QPoint m_equipBorderPos;
        QPoint m_equipSelectedOffset;
        int m_disperseWidth;
        QColor m_trustEffectColor;
        QSanShadowTextFont m_skillTextFonts[3];
        QRect m_skillNameArea;
        QRect m_secondarySkillNameArea;
        QSanShadowTextFont m_skillNameFont;
        QColor m_skillTextColors[QSanButton::S_NUM_BUTTON_STATES * QSanInvokeSkillButton::S_NUM_SKILL_TYPES];
        QColor m_skillTextShadowColors[QSanButton::S_NUM_BUTTON_STATES * QSanInvokeSkillButton::S_NUM_SKILL_TYPES];

        QSanShadowTextFont getSkillTextFont(QSanButton::ButtonState state, QSanInvokeSkillButton::SkillType type, QSanInvokeSkillButton::SkillButtonWidth width) const;
    };

    struct CommonLayout
    {
        // card related
        int m_cardNormalWidth;
        int m_cardNormalHeight;
        QRect m_cardMainArea;
        QRect m_cardSuitArea;
        QRect m_cardNumberArea;
        QRect m_cardFootnoteArea;
        QRect m_cardAvatarArea;
        QRect m_cardFrameArea;
        QSanShadowTextFont m_cardFootnoteFont;
        QSanShadowTextFont m_hpFont[6];
        int m_hpExtraSpaceHolder;

        // dialogs
        // when # of generals <= switchIconSizeThreadshold
        QSize m_chooseGeneralBoxSparseIconSize;
        // when # of generals > switchIconSizeThreadshold
        QSize m_chooseGeneralBoxDenseIconSize;
        int m_chooseGeneralBoxSwitchIconSizeThreshold;
        int m_chooseGeneralBoxSwitchIconEachRow;
        int m_chooseGeneralBoxSwitchIconEachRowForTooManyGenerals;
        int m_chooseGeneralBoxNoIconThreshold;

        // avatar size
        QSize m_tinyAvatarSize;

        //role combo box (hegemony)
        QSize m_roleNormalBgSize;
        QHash<QString, QRect> m_rolesRect;
        QHash<QString, QColor> m_rolesColor;
        QColor m_roleDarkColor;

        //Graphics Box
        QColor graphicsBoxBackgroundColor;
        QColor graphicsBoxBorderColor;
        QSanSimpleTextFont graphicsBoxTitleFont;

        //Choose General Box
        QSanSimpleTextFont m_chooseGeneralBoxDestSeatFont;

        //General Card Item
        QRect m_generalCardItemCompanionPromptRegion;

        //General Button
        QRect generalButtonPositionIconRegion;
        QRect generalButtonNameRegion;

        //Option Button
        QSanShadowTextFont optionButtonText;
    };

    enum GeneralIconSize
    {
        S_GENERAL_ICON_SIZE_TINY,
        S_GENERAL_ICON_SIZE_SMALL,
        S_GENERAL_ICON_SIZE_LARGE,
        S_GENERAL_ICON_SIZE_CARD,
        S_GENERAL_ICON_SIZE_PHOTO_SECONDARY,
        S_GENERAL_ICON_SIZE_DASHBOARD_SECONDARY,
        S_GENERAL_ICON_SIZE_PHOTO_PRIMARY,
        S_GENERAL_ICON_SIZE_DASHBOARD_PRIMARY,
        S_GENERAL_ICON_SIZE_KOF
    };

    const RoomLayout &getRoomLayout() const;
    const PhotoLayout &getPhotoLayout() const;
    const CommonLayout &getCommonLayout() const;
    const DashboardLayout &getDashboardLayout() const;

    QString getButtonPixmapPath(const QString &groupName, const QString &buttonName, QSanButton::ButtonState state) const;
    QPixmap getButtonPixmap(const QString &groupName, const QString &buttonName, QSanButton::ButtonState state) const;
    QPixmap getSkillButtonPixmap(QSanButton::ButtonState state, QSanInvokeSkillButton::SkillType type, QSanInvokeSkillButton::SkillButtonWidth width) const;
    QPixmap getCardMainPixmap(const QString &cardName, bool cache = false, bool heroSkin = true) const;
    QPixmap getCardSuitPixmap(Card::Suit suit) const;
    QPixmap getCardTianyiPixmap() const;
    QPixmap getCardNumberPixmap(int point, bool isBlack) const;
    QPixmap getCardJudgeIconPixmap(const QString &judgeName) const;
    QPixmap getCardFramePixmap(const QString &frameType) const;
    QPixmap getCardAvatarPixmap(const QString &generalName, bool heroSkin = true) const;
    QPixmap getGeneralPixmap(const QString &generalName, GeneralIconSize size, bool heroSkin = true) const;
    QString getPlayerAudioEffectPath(const QString &eventName, bool isMale, int index = -1) const;
    QString getPlayerAudioEffectPath(const QString &eventName, const QString &category, int index = -1) const;
    QPixmap getProgressBarPixmap(int percentile) const;

    void getHeroSkinContainerGeneralIconPathAndClipRegion(const QString &generalName, int skinIndex, QString &generalIconPath, QRect &clipRegion) const;

    // Animations
    QAbstractAnimation *createHuaShenAnimation(QPixmap &huashenAvatar, QPoint topLeft, QGraphicsItem *parent, QGraphicsItem *&huashenItemCreated) const;
    //QAbstractAnimation *createHuaShenAnimation(QPixmap &huashenAvatar, QPoint topLeft, QGraphicsItem *parent,
    //                                          GraphicsPixmapHoverItem *&huashenItemCreated) const;

    // static consts
    // main keys
    static const char *S_SKIN_KEY_DASHBOARD;
    static const char *S_SKIN_KEY_PHOTO;
    static const char *S_SKIN_KEY_COMMON;
    static const char *S_SKIN_KEY_ROOM;

    // role box (hegemony)
    static const char *S_SKIN_KEY_ROLE_BOX_RECT;
    static const char *S_SKIN_KEY_ROLE_BOX_COLOR;

    //bg
    static const char *S_SKIN_KEY_TABLE_BG;

    // button
    static const char *S_SKIN_KEY_BUTTON;
    static const char *S_SKIN_KEY_DASHBOARD_BUTTON_SET_BG;
    static const char *S_SKIN_KEY_BUTTON_DASHBOARD_CONFIRM;
    static const char *S_SKIN_KEY_BUTTON_DASHBOARD_CANCEL;
    static const char *S_SKIN_KEY_BUTTON_DASHBOARD_DISCARD;
    static const char *S_SKIN_KEY_BUTTON_DASHBOARD_TRUST;
    static const char *S_SKIN_KEY_PLATTER;
    static const char *S_SKIN_KEY_BUTTON_SKILL;

    // player container
    static const char *S_SKIN_KEY_MAINFRAME;
    static const char *S_SKIN_KEY_LEFTFRAME;
    static const char *S_SKIN_KEY_RIGHTFRAME;
    static const char *S_SKIN_KEY_MIDDLEFRAME;
    static const char *S_SKIN_KEY_HANDCARDNUM;
    static const char *S_SKIN_KEY_FACETURNEDMASK;
    static const char *S_SKIN_KEY_FACETURNEDMASK_HEGEMONY;
    static const char *S_SKIN_KEY_BLANK_GENERAL;
    static const char *S_SKIN_KEY_CHAIN;
    static const char *S_SKIN_KEY_PHASE;
    static const char *S_SKIN_KEY_SELECTED_FRAME;
    static const char *S_SKIN_KEY_FOCUS_FRAME;
    static const char *S_SKIN_KEY_SAVE_ME_ICON;
    static const char *S_SKIN_KEY_ACTIONED_ICON;
    static const char *S_SKIN_KEY_KINGDOM_ICON;
    static const char *S_SKIN_KEY_KINGDOM_COLOR_MASK;
    static const char *S_SKIN_KEY_DASHBOARD_KINGDOM_COLOR_MASK;
    static const char *S_SKIN_KEY_VOTES_NUMBER;
    static const char *S_SKIN_KEY_SEAT_NUMBER;
    static const char *S_SKIN_KEY_HAND_CARD_BACK;
    static const char *S_SKIN_KEY_HAND_CARD_SUIT;
    static const char *S_SKIN_KEY_CARD_TIANYI;
    static const char *S_SKIN_KEY_JUDGE_CARD_ICON;
    static const char *S_SKIN_KEY_HAND_CARD_MAIN_PHOTO;
    static const char *S_SKIN_KEY_HAND_CARD_NUMBER_BLACK;
    static const char *S_SKIN_KEY_HAND_CARD_NUMBER_RED;
    static const char *S_SKIN_KEY_HAND_CARD_FRAME;
    static const char *S_SKIN_KEY_PLAYER_GENERAL_ICON;
    static const char *S_SKIN_KEY_EXTRA_SKILL_BG;
    static const char *S_SKIN_KEY_MAGATAMAS_BG;
    static const char *S_SKIN_KEY_MAGATAMAS;
    static const char *S_SKIN_KEY_MAGATAMAS_DYINGLINE;
    static const char *S_SKIN_KEY_PLAYER_AUDIO_EFFECT;
    static const char *S_SKIN_KEY_SYSTEM_AUDIO_EFFECT;
    static const char *S_SKIN_KEY_EQUIP_ICON;
    static const char *S_SKIN_KEY_EQUIP_BROKEN_ICON;
    static const char *S_SKIN_KEY_PROGRESS_BAR_IMAGE;
    static const char *S_SKIN_KEY_GENERAL_CIRCLE_IMAGE;
    static const char *S_SKIN_KEY_GENERAL_CIRCLE_MASK;
    static const char *S_SKIN_KEY_HEAD_ICON;
    static const char *S_SKIN_KEY_DEPUTY_ICON;
    static const char *S_SKIN_KEY_ROLE_SHOWN;
    static const char *S_SKIN_KEY_HIDDEN_MARK;
    static const char *S_SKIN_KEY_DISABLE_SHOW_LOCK;

    // Animations
    static const char *S_SKIN_KEY_ANIMATIONS;
    static const char *S_SKIN_KEY_LIGHTBOX;

    // RoleComboBox (hegemony)
    static const char *S_SKIN_KEY_EXPANDING_ROLE_BOX;
    static const char *S_SKIN_KEY_ROLE_BOX_KINGDOM_MASK;

    //ChooseGeneralBox
    static const char *S_SKIN_KEY_CHOOSE_GENERAL_BOX_SPLIT_LINE;
    static const char *S_SKIN_KEY_CHOOSE_GENERAL_BOX_DEST_SEAT;

    //GeneralCardItem
    static const char *S_SKIN_KEY_GENERAL_CARD_ITEM_COMPANION_FONT;
    static const char *S_SKIN_KEY_GENERAL_CARD_ITEM_COMPANION_ICON;

    static const char *S_HERO_SKIN_KEY_GENERAL_ICON;

protected:
    RoomLayout _m_roomLayout;
    PhotoLayout _m_photoLayout;
    CommonLayout _m_commonLayout;
    DashboardLayout _m_dashboardLayout;
    bool _loadLayoutConfig(const QVariant &layoutConfig) override;
    bool _loadAnimationConfig(const QVariant &animationConfig) override;
};

class QSanSkinScheme
{
    // Why do we need another layer above room skin? Because we may add lobby, login interface
    // in the future; and we may need to assemble a set of different skins into a scheme.
public:
    bool load(const QVariant &configs);
    const QSanRoomSkin &getRoomSkin() const;

protected:
    QSanRoomSkin _m_roomSkin;
};

class QSanSkinFactory
{
public:
    static QSanSkinFactory &getInstance();
    static void destroyInstance();
    const QString &getCurrentSkinName() const;
    const QSanSkinScheme &getCurrentSkinScheme();
    bool switchSkin(QString skinName);

    QString S_DEFAULT_SKIN_NAME;
    QString S_COMPACT_SKIN_NAME;

protected:
    explicit QSanSkinFactory(const char *fileName);
    static QSanSkinFactory *_sm_singleton;
    QSanSkinScheme _sm_currentSkin;
    JsonObject _m_skinList;
    QString _m_skinName;
};

#define G_ROOM_SKIN (QSanSkinFactory::getInstance().getCurrentSkinScheme().getRoomSkin())
#define G_DASHBOARD_LAYOUT (G_ROOM_SKIN.getDashboardLayout())
#define G_ROOM_LAYOUT (G_ROOM_SKIN.getRoomLayout())
#define G_PHOTO_LAYOUT (G_ROOM_SKIN.getPhotoLayout())
#define G_COMMON_LAYOUT (G_ROOM_SKIN.getCommonLayout())

#endif
