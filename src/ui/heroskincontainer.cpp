#include "heroskincontainer.h"
#include "GenericCardContainerUI.h"
#include "engine.h"
#include "qsanbutton.h"
#include "settings.h"
#include "skinitem.h"

#include <QCursor>
#include <QDir>
#include <QFontDatabase>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneWheelEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QScrollBar>

const char *const HEROSKIN_PIXMAP_PATH = "image/heroskin/fullskin/generals/full";
const char *const KINGDOM_COLORMASK_PIXMAP_PATH = "image/kingdom/frame/dashboard/%1.png";

const int LEFT_MARGIN = 5;
const int AVAILABLE_AREA_WIDTH = 400;
const int Y_STEP = 12;
const int Y_START_POS = 32;

const int SKIN_ITEM_WIDTH = SKIN_ITEM_AREA.width();
const int SKIN_ITEM_HEIGHT = SKIN_ITEM_AREA.height();
const QRegularExpression SKIN_FILE_NAME_PATTERN(QRegularExpression::anchoredPattern(QStringLiteral("(?:[A-Za-z_0-9]+)(\\d+).png")));

HeroSkinContainer *HeroSkinContainer::m_currentTopMostContainer = nullptr;
QMap<QString, QStringList> HeroSkinContainer::m_generalToSkinFiles;
QMap<QString, bool> HeroSkinContainer::m_generalToHasSkin;

HeroSkinContainer::HeroSkinContainer(const QString &generalName, const QString &kingdom, QGraphicsItem *parent /* = 0*/)
    : QGraphicsObject(parent)
    , m_generalName(generalName)
    , m_backgroundPixmap(QStringLiteral("image/system/heroskin-container.png"))
    , m_vScrollBar(nullptr)
    , m_oldScrollValue(0)
{
    setFlag(ItemIsMovable);
    setCursor(Qt::ArrowCursor);

    QSanButton *closeButton = new QSanButton(QStringLiteral("player_container"), QStringLiteral("close-heroskin"), this); //"change-heroskin"

    closeButton->setPos(385, 5);

    connect(closeButton, &QSanButton::clicked, this, &HeroSkinContainer::close);

    QGraphicsPixmapItem *kingdomColorMaskIcon = nullptr;
    PlayerCardContainer::_paintPixmap(kingdomColorMaskIcon, QRect(11, -5, 130, 40), //11, 6, 87, 21
                                      G_ROOM_SKIN.getPixmapFromFileName(QString::fromUtf8(KINGDOM_COLORMASK_PIXMAP_PATH).arg(kingdom)), this);

    QGraphicsPixmapItem *kingdomIcon = nullptr;
    PlayerCardContainer::_paintPixmap(kingdomIcon, QRect(9, 2, 28, 25), G_ROOM_SKIN.getPixmap(QString::fromUtf8(QSanRoomSkin::S_SKIN_KEY_KINGDOM_ICON), kingdom), this);

    new QGraphicsPixmapItem(this);

    initSkins();
    fillSkins();
}

bool HeroSkinContainer::hasSkin(const QString &generalName)
{
    if (!m_generalToHasSkin.contains(generalName)) {
        QStringList files = HeroSkinContainer::getHeroSkinFiles(generalName);
        foreach (const QString &file, files) {
            QRegularExpressionMatch match;
            if ((match = SKIN_FILE_NAME_PATTERN.match(file)).hasMatch()) {
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
        QRegularExpressionMatch match;
        if ((match = SKIN_FILE_NAME_PATTERN.match(file)).hasMatch()) {
            int num = match.capturedTexts().at(1).toInt();
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
    QString unique_general = generalName;
    if (unique_general.endsWith(QStringLiteral("_hegemony")))
        unique_general = unique_general.replace(QStringLiteral("_hegemony"), QString());

    if (!m_generalToSkinFiles.contains(unique_general)) {
        QDir dir(QString::fromUtf8(HEROSKIN_PIXMAP_PATH));

        dir.setNameFilters(QStringList(QStringLiteral("%1_*.png").arg(unique_general)));
        QStringList tmpFiles = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);

        QStringList heroSkinFiles;
        //filter files
        foreach (const QString &file, tmpFiles) {
            if (file.count(QStringLiteral("_")) == unique_general.count(QStringLiteral("_")) + 1)
                heroSkinFiles << file;
        }

        if (!heroSkinFiles.isEmpty()) {
            std::sort(heroSkinFiles.begin(), heroSkinFiles.end(), caseInsensitiveLessThan);
            m_generalToSkinFiles[unique_general] = heroSkinFiles;
        }
    }

    return m_generalToSkinFiles[unique_general];
}

void HeroSkinContainer::initSkins()
{
    QGraphicsRectItem *dummyRectItem = new QGraphicsRectItem(QRectF(LEFT_MARGIN, 35, AVAILABLE_AREA_WIDTH, 174), this);
    dummyRectItem->setFlag(ItemHasNoContents);
    dummyRectItem->setFlag(ItemClipsChildrenToShape);
    QString unique_general = m_generalName;
    if (unique_general.endsWith(QStringLiteral("_hegemony")))
        unique_general = unique_general.replace(QStringLiteral("_hegemony"), QString());
    int skinIndexUsed = Config.value(QStringLiteral("HeroSkin/%1").arg(unique_general), 0).toInt();
    createSkinItem(skinIndexUsed, dummyRectItem, true);

    QStringList files = getHeroSkinFiles(m_generalName);
    foreach (const QString &file, files) {
        QRegularExpressionMatch match;
        if ((match = SKIN_FILE_NAME_PATTERN.match(file)).hasMatch()) {
            int skinIndex = match.capturedTexts().at(1).toInt();
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
        connect(skinItem, &SkinItem::clicked, this, &HeroSkinContainer::skinSelected);
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
        m_vScrollBar->setObjectName(QStringLiteral("sgsVSB"));
        m_vScrollBar->setStyleSheet(Settings::getQSSFileContent());
        m_vScrollBar->setFocusPolicy(Qt::StrongFocus);
        connect(m_vScrollBar, &QAbstractSlider::valueChanged, this, &HeroSkinContainer::scrollBarValueChanged);

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

void HeroSkinContainer::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // draw pixel map
    painter->drawPixmap(0, 0, m_backgroundPixmap);

    painter->setPen(Qt::white);
    painter->setBrush(Qt::NoBrush);

    // paint text.
    painter->setFont(getAvatarNameFont());
    // get name.
    QString name = Sanguosha->translate(QStringLiteral("&") + m_generalName);
    if (name.startsWith(QStringLiteral("&"))) {
        name = Sanguosha->translate(m_generalName);
    }

    painter->drawText(QRect(34, -8, 100, 40), Qt::AlignLeft | Qt::AlignJustify, name);

    // paint shadow
    // https://forum.qt.io/topic/47422/how-to-draw-a-text-with-drop-shadow-in-the-image-using-qpainter/2
}

void HeroSkinContainer::close()
{
    hide();
}

void HeroSkinContainer::skinSelected(int skinIndex)
{
    QString unique_general = m_generalName;
    if (unique_general.endsWith(QStringLiteral("_hegemony")))
        unique_general = unique_general.replace(QStringLiteral("_hegemony"), QString());
    Config.beginGroup(QStringLiteral("HeroSkin"));
    (0 == skinIndex) ? Config.remove(unique_general) : Config.setValue(unique_general, skinIndex);
    Config.endGroup();

    close();

    swapWithSkinItemUsed(skinIndex);

    if (nullptr != m_vScrollBar) {
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

    m_skins.swapItemsAt(0, m_skins.indexOf(newSkinItemUsed));
}

const QFont &HeroSkinContainer::getAvatarNameFont()
{
    return Config.TinyFont;
}

void HeroSkinContainer::mousePressEvent(QGraphicsSceneMouseEvent * /*event*/)
{
    bringToTopMost();
}

void HeroSkinContainer::bringToTopMost()
{
    if (nullptr != m_currentTopMostContainer) {
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
    if (nullptr != m_vScrollBar) {
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
