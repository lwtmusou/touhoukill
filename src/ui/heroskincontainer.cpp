#include "heroskincontainer.h"
#include "GenericCardContainerUI.h"
#include "engine.h"
#include "qsanbutton.h"
#include "settings.h"
#include "skinitem.h"

#include "sanshadowtextfont.h"
#include <QGraphicsProxyWidget>

#include <QCursor>
#include <QDir>
#include <QGraphicsSceneWheelEvent>
#include <QPainter>
#include <QScrollBar>

const char *HEROSKIN_PIXMAP_PATH = "image/heroskin/fullskin/generals/full";
const char *KINGDOM_COLORMASK_PIXMAP_PATH = "image/kingdom/frame/dashboard/%1.png";
//"image/fullskin/kingdom/frame/dashboard/%1.png";

const int LEFT_MARGIN = 5;
const int AVAILABLE_AREA_WIDTH = 400;
const int Y_STEP = 12;
const int Y_START_POS = 32;

const int SKIN_ITEM_WIDTH = SKIN_ITEM_AREA.width();
const int SKIN_ITEM_HEIGHT = SKIN_ITEM_AREA.height();
//(?:[A-Za-z_]+)(\\d+).png
const QRegExp SKIN_FILE_NAME_PATTERN = QRegExp("(?:[A-Za-z_0-9]+)(\\d+).png");

HeroSkinContainer *HeroSkinContainer::m_currentTopMostContainer = NULL;
QMap<QString, QStringList> HeroSkinContainer::m_generalToSkinFiles;
QMap<QString, bool> HeroSkinContainer::m_generalToHasSkin;

HeroSkinContainer::HeroSkinContainer(const QString &generalName, const QString &kingdom, QGraphicsItem *parent /* = 0*/)
    : QGraphicsObject(parent)
    , m_generalName(generalName)
    , m_backgroundPixmap("image/system/heroskin-container.png")
    , m_vScrollBar(NULL)
    , m_oldScrollValue(0)
{
    setFlag(ItemIsMovable);
    setCursor(Qt::ArrowCursor);

    QSanButton *closeButton = new QSanButton("player_container", "close-heroskin", this); //"change-heroskin"

    //QSanButton *closeButton = new QSanButton("card_container",
    //"close", this);

    closeButton->setPos(385, 5);

    connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));

    QGraphicsPixmapItem *kingdomColorMaskIcon = NULL;
    PlayerCardContainer::_paintPixmap(kingdomColorMaskIcon, QRect(11, -5, 130, 40), //11, 6, 87, 21
                                      G_ROOM_SKIN.getPixmapFromFileName(QString(KINGDOM_COLORMASK_PIXMAP_PATH).arg(kingdom)), this);

    QGraphicsPixmapItem *kingdomIcon = NULL;
    PlayerCardContainer::_paintPixmap(kingdomIcon, QRect(9, 2, 28, 25), G_ROOM_SKIN.getPixmap(QSanRoomSkin::S_SKIN_KEY_KINGDOM_ICON, kingdom), this);

    QString name = Sanguosha->translate("&" + m_generalName);
    if (name.startsWith("&")) {
        name = Sanguosha->translate(m_generalName);
    }
    QGraphicsPixmapItem *avatarNameItem = new QGraphicsPixmapItem(this);
    getAvatarNameFont().paintText(avatarNameItem, QRect(34, -8, 100, 40), //34, -8, 60, 40
                                  Qt::AlignLeft | Qt::AlignJustify, name);

    initSkins();
    fillSkins();
}

bool HeroSkinContainer::hasSkin(const QString &generalName)
{
    if (!m_generalToHasSkin.contains(generalName)) {
        QStringList files = HeroSkinContainer::getHeroSkinFiles(generalName);
        foreach (const QString &file, files) {
            if (SKIN_FILE_NAME_PATTERN.exactMatch(file)) {
                m_generalToHasSkin[generalName] = true;
                break;
            }
        }
    }

    return m_generalToHasSkin[generalName];
}

int HeroSkinContainer::getNextSkinIndex(const QString &generalName, int skinIndex)
{
    int result = skinIndex;

    QStringList files = HeroSkinContainer::getHeroSkinFiles(generalName);
    foreach (const QString &file, files) {
        if (SKIN_FILE_NAME_PATTERN.exactMatch(file)) {
            int num = SKIN_FILE_NAME_PATTERN.capturedTexts().at(1).toInt();
            if (num > skinIndex) {
                result = num;
                break;
            }
        }
    }

    return result;
}

bool caseInsensitiveLessThan(const QString &s1, const QString &s2)
{
    int s1Length = s1.length();
    int s2Length = s2.length();

    if (s1Length > s2Length) {
        return false;
    } else if (s1Length == s2Length) {
        return s1.toLower() < s2.toLower();
    } else {
        return true;
    }
}

QStringList HeroSkinContainer::getHeroSkinFiles(const QString &generalName)
{
    if (!m_generalToSkinFiles.contains(generalName)) {
        QDir dir(HEROSKIN_PIXMAP_PATH);

        dir.setNameFilters(QStringList(QString("%1_*.png").arg(generalName)));
        QStringList tmpFiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

        QStringList heroSkinFiles;
        //filter files
        foreach (const QString &file, tmpFiles) {
            if (file.count("_") == generalName.count("_") + 1)
                heroSkinFiles << file;
        }

        if (!heroSkinFiles.isEmpty()) {
            std::sort(heroSkinFiles.begin(), heroSkinFiles.end(), caseInsensitiveLessThan);
            m_generalToSkinFiles[generalName] = heroSkinFiles;
        }
    }

    return m_generalToSkinFiles[generalName];
}

void HeroSkinContainer::initSkins()
{
    QGraphicsRectItem *dummyRectItem = new QGraphicsRectItem(QRectF(LEFT_MARGIN, 35, AVAILABLE_AREA_WIDTH, 174), this);
    dummyRectItem->setFlag(ItemHasNoContents);
    dummyRectItem->setFlag(ItemClipsChildrenToShape);

    int skinIndexUsed = Config.value(QString("HeroSkin/%1").arg(m_generalName), 0).toInt();
    createSkinItem(skinIndexUsed, dummyRectItem, true);

    QStringList files = getHeroSkinFiles(m_generalName);
    foreach (const QString &file, files) {
        if (SKIN_FILE_NAME_PATTERN.exactMatch(file)) {
            int skinIndex = SKIN_FILE_NAME_PATTERN.capturedTexts().at(1).toInt();
            if (skinIndexUsed != skinIndex) {
                createSkinItem(skinIndex, dummyRectItem);
            }
        }
    }

    if (0 != skinIndexUsed) {
        createSkinItem(0, dummyRectItem);
    }
}

void HeroSkinContainer::createSkinItem(int skinIndex, QGraphicsItem *parent, bool used /* = false*/)
{
    QString generalIconPath;
    QRect clipRegion;
    G_ROOM_SKIN.getHeroSkinContainerGeneralIconPathAndClipRegion(m_generalName, skinIndex, generalIconPath, clipRegion);
    if (QFile::exists(generalIconPath)) {
        SkinItem *skinItem = new SkinItem(generalIconPath, clipRegion, skinIndex, used, parent);
        connect(skinItem, SIGNAL(clicked(int)), this, SLOT(skinSelected(int)));
        m_skins << skinItem;
        m_skinIndexToItem[skinIndex] = skinItem;
    }
}

void HeroSkinContainer::fillSkins()
{
    int skinCount = m_skins.count();
    if (0 == skinCount) {
        return;
    }

    int columns = (skinCount > 3) ? 3 : skinCount;
    int rows = skinCount / columns;
    if (skinCount % columns != 0) {
        ++rows;
    }

    if (skinCount > 3) {
        m_vScrollBar = new QScrollBar(Qt::Vertical);
        m_vScrollBar->setObjectName("sgsVSB");
        m_vScrollBar->setStyleSheet(Settings::getQSSFileContent());
        m_vScrollBar->setFocusPolicy(Qt::StrongFocus);
        connect(m_vScrollBar, SIGNAL(valueChanged(int)), this, SLOT(scrollBarValueChanged(int)));

        m_vScrollBar->setMaximum((rows - 1) * (SKIN_ITEM_HEIGHT + Y_STEP));
        m_vScrollBar->setPageStep(12 + (rows - 1) * 3);
        m_vScrollBar->setSingleStep(15 + (rows - 1) * 3);

        QGraphicsProxyWidget *scrollBarWidget = new QGraphicsProxyWidget(this);
        scrollBarWidget->setWidget(m_vScrollBar);
        scrollBarWidget->setGeometry(QRectF(391, 35, 10, 174));
    }

    int xStep = (AVAILABLE_AREA_WIDTH - columns * SKIN_ITEM_WIDTH) / (columns + 1);
    int xStartPos = LEFT_MARGIN + xStep;

    int x = xStartPos;
    int y = Y_START_POS;
    int skinItemIndex = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            m_skins[skinItemIndex]->setPos(x, y);

            ++skinItemIndex;
            if (skinItemIndex >= skinCount) {
                return;
            }

            x += (SKIN_ITEM_WIDTH + xStep);
        }

        x = xStartPos;
        y += (SKIN_ITEM_HEIGHT + Y_STEP);
    }
}

QRectF HeroSkinContainer::boundingRect() const
{
    return QRectF(QPoint(0, 0), m_backgroundPixmap.size());
}

void HeroSkinContainer::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->drawPixmap(0, 0, m_backgroundPixmap);
}

void HeroSkinContainer::close()
{
    hide();
}

void HeroSkinContainer::skinSelected(int skinIndex)
{
    Config.beginGroup("HeroSkin");
    (0 == skinIndex) ? Config.remove(m_generalName) : Config.setValue(m_generalName, skinIndex);
    Config.endGroup();

    close();

    swapWithSkinItemUsed(skinIndex);

    if (NULL != m_vScrollBar) {
        m_vScrollBar->setValue(0);
    }

    emit local_skin_changed(m_generalName); //for self clinet roomscene
    emit skin_changed(m_generalName, skinIndex); //for server notify
}

void HeroSkinContainer::swapWithSkinItemUsed(int skinIndex)
{
    SkinItem *oldSkinItemUsed = m_skins.first();
    SkinItem *newSkinItemUsed = m_skinIndexToItem[skinIndex];
    oldSkinItemUsed->setUsed(false);
    newSkinItemUsed->setUsed(true);

    QPointF oldSkinItemUsedPos = oldSkinItemUsed->pos();
    QPointF newSkinItemUsedPos = newSkinItemUsed->pos();
    oldSkinItemUsed->setPos(newSkinItemUsedPos);
    newSkinItemUsed->setPos(oldSkinItemUsedPos);

    m_skins.swap(0, m_skins.indexOf(newSkinItemUsed));
}

const SanShadowTextFont &HeroSkinContainer::getAvatarNameFont()
{
    static const SanShadowTextFont avatarNameFont("SimLi", QSize(18, 18), 1, 10, QColor(50, 50, 50, 200));
    //SanShadowTextFont avatarNameFont("SimLi");
    return avatarNameFont;
}

void HeroSkinContainer::mousePressEvent(QGraphicsSceneMouseEvent *)
{
    bringToTopMost();
}

void HeroSkinContainer::bringToTopMost()
{
    if (NULL != m_currentTopMostContainer) {
        if (this == m_currentTopMostContainer) {
            return;
        }

        m_currentTopMostContainer->setZValue(m_currentTopMostContainer->m_originalZValue);
    }

    m_originalZValue = zValue();
    m_currentTopMostContainer = this;
    m_currentTopMostContainer->setZValue(UINT_MAX);
}

void HeroSkinContainer::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    if (NULL != m_vScrollBar) {
        int deltaValue = event->delta();
        int scrollBarValue = m_vScrollBar->value();
        scrollBarValue += (-deltaValue / 120) * m_vScrollBar->pageStep();
        m_vScrollBar->setValue(scrollBarValue);
    }
}

void HeroSkinContainer::scrollBarValueChanged(int newValue)
{
    int diff = newValue - m_oldScrollValue;
    foreach (SkinItem *skinItem, m_skins) {
        skinItem->moveBy(0, -diff);
    }

    m_oldScrollValue = newValue;
}
