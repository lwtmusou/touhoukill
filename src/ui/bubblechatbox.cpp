#include "bubblechatbox.h"
#include "settings.h"

#include <QBitmap>
#include <QGraphicsScene>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTextDocument>

const int PIXELS_PER_LINE = 168;
const int BOX_MIN_WIDTH = 42;
const int MAX_LINE_COUNT = 2;
const int CHAT_FACE_WIDTH = 16;
const int BOX_LEFT_FRAME_WIDTH = 6;
const int BOX_RIGHT_FRAME_WIDTH = 10;
const int BOX_FRAME_HEIGHT = 28;
const int ANIMATION_DURATION = 500;

class BubbleChatLabel : public QGraphicsTextItem
{
public:
    explicit BubbleChatLabel(QGraphicsItem *parent = 0)
        : QGraphicsTextItem(parent)
        , m_doc(document())
    {
    }

    virtual QRectF boundingRect() const
    {
        return m_rect;
    }
    void setBoundingRect(const QRectF &newRect)
    {
        m_rect = newRect;
    }

    void setAlignment(Qt::Alignment alignment)
    {
        QTextOption opt = m_doc->defaultTextOption();
        opt.setAlignment(alignment);
        m_doc->setDefaultTextOption(opt);
    }

    void setWrapMode(QTextOption::WrapMode wrap)
    {
        QTextOption opt = m_doc->defaultTextOption();
        opt.setWrapMode(wrap);
        m_doc->setDefaultTextOption(opt);
    }

private:
    QRectF m_rect;
    QTextDocument *m_doc;
};

BubbleChatBox::BubbleChatBox(const QRect &area, QGraphicsItem *parent /* = 0*/)
    : QGraphicsObject(parent)
    , m_backgroundPixmap("image/system/bubble.png")
    , m_rect(m_backgroundPixmap.rect())
    , m_area(area)
    , m_chatLabel(new BubbleChatLabel(this))
    , m_appearAndDisappear(new QPropertyAnimation(this, "opacity", this))
{
    m_chatLabel->setFont(Config.UIFont);
    m_chatLabel->setWrapMode(QTextOption::WrapAnywhere);

    setFlag(ItemClipsChildrenToShape);
    setOpacity(0);

    connect(&m_timer, SIGNAL(timeout()), this, SLOT(clear()));

    m_appearAndDisappear->setStartValue(0);
    m_appearAndDisappear->setEndValue(1);
    m_appearAndDisappear->setDuration(ANIMATION_DURATION);
}

BubbleChatBox::~BubbleChatBox()
{
    m_timer.stop();
}

QRectF BubbleChatBox::boundingRect() const
{
    return m_rect;
}

void BubbleChatBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    painter->drawPixmap(m_rect.toRect(), m_backgroundPixmap);
}

QPainterPath BubbleChatBox::shape() const
{
    QRegion maskRegion(m_backgroundPixmap.mask().scaled(m_rect.size().toSize()));
    QPainterPath path;
    path.addRegion(maskRegion);
    return path;
}

void BubbleChatBox::setText(const QString &text)
{
    m_chatLabel->setHtml(text);

    QString plainText = m_chatLabel->toPlainText();
    if (plainText.isEmpty()) {
        return;
    }

    QFontMetrics fm(m_chatLabel->font());
    int imgCount = text.count("</img>");
    int width = qAbs(fm.width(plainText)) + imgCount * CHAT_FACE_WIDTH;
    int lineCount = 1;
    if (width > PIXELS_PER_LINE) {
        lineCount = width / PIXELS_PER_LINE;
        if (lineCount >= MAX_LINE_COUNT) {
            lineCount = MAX_LINE_COUNT;
        } else if (width % PIXELS_PER_LINE != 0) {
            ++lineCount;
        }

        width = PIXELS_PER_LINE;
    }

    int boxWidth = width + fm.maxWidth();
    if (boxWidth <= BOX_MIN_WIDTH) {
        boxWidth = BOX_MIN_WIDTH;
        m_chatLabel->setAlignment(Qt::AlignHCenter);
    } else {
        m_chatLabel->setAlignment(Qt::AlignLeft);
    }
    m_chatLabel->setTextWidth(boxWidth);

    QRectF m_oldRect = m_rect;

    int height = fm.lineSpacing() + fm.xHeight();
    m_rect.setSize(QSize(boxWidth + BOX_RIGHT_FRAME_WIDTH, height * lineCount + BOX_FRAME_HEIGHT));

    m_chatLabel->setPos(QPointF(BOX_LEFT_FRAME_WIDTH, m_rect.center().y() - (height * lineCount) + (lineCount - 1) * (height / 2) - (imgCount > 0 ? 1 : 0)));
    m_chatLabel->setBoundingRect(QRectF(0, 0, boxWidth, height * lineCount + (MAX_LINE_COUNT - lineCount) * 1));

    updatePos();

    if (opacity() != 1) {
        m_appearAndDisappear->setDirection(QAbstractAnimation::Forward);
        m_appearAndDisappear->start();
    }

    if (m_oldRect.width() > m_rect.width()) {
        QRectF sceneRect = mapRectToScene(m_oldRect);
        scene()->update(sceneRect);
    } else {
        update();
    }

    m_timer.start(Config.BubbleChatBoxDelaySeconds * 1000);
}

void BubbleChatBox::setArea(const QRect &newArea)
{
    m_area = newArea;
    updatePos();
}

QVariant BubbleChatBox::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSceneHasChanged && scene()) {
        scene()->addItem(m_chatLabel);
    }

    return QGraphicsItem::itemChange(change, value);
}

void BubbleChatBox::clear()
{
    m_timer.stop();

    m_appearAndDisappear->setDirection(QAbstractAnimation::Backward);
    m_appearAndDisappear->start();
}

void BubbleChatBox::updatePos()
{
    int xOffset = (m_area.width() - m_rect.width()) / 2;
    int yOffset = (m_area.height() - m_rect.height()) / 2;
    setPos(QPointF(m_area.left() + xOffset, m_area.top() + yOffset));
}
