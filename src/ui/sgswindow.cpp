#include "sgswindow.h"
#include "button.h"
#include "settings.h"

#include <QGraphicsDropShadowEffect>
#include <QGraphicsRotation>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

Window::Window(const QString &title, QSizeF size, const QString &path)
    : size(size)
    , keep_when_disappear(false)
{
    setFlags(ItemIsMovable);

    QPixmap *bg = nullptr;
    if (!path.isEmpty())
        bg = new QPixmap(path);
    else
        bg = size.width() > size.height() ? new QPixmap(QStringLiteral("image/system/tip.png")) : new QPixmap(QStringLiteral("image/system/about.png"));
    QImage bgimg = bg->toImage();
    outimg = new QImage(size.toSize(), QImage::Format_ARGB32);

    qreal pad = 10;

    int w = bgimg.width();
    int h = bgimg.height();
    int tw = outimg->width();
    int th = outimg->height();

    qreal xc = (w - 2 * pad) / (tw - 2 * pad);
    qreal yc = (h - 2 * pad) / (th - 2 * pad);

    for (int i = 0; i < tw; i++) {
        for (int j = 0; j < th; j++) {
            int x = i;
            int y = j;

            if (x >= pad && x <= (tw - pad))
                x = pad + (x - pad) * xc;
            else if (x >= (tw - pad))
                x = w - (tw - x);

            if (y >= pad && y <= (th - pad))
                y = pad + (y - pad) * yc;
            else if (y >= (th - pad))
                y = h - (th - y);

            QRgb rgb = bgimg.pixel(x, y);
            outimg->setPixel(i, j, rgb);
        }
    }

    scaleTransform = new QGraphicsScale(this);
    scaleTransform->setXScale(1.05);
    scaleTransform->setYScale(0.95);
    scaleTransform->setOrigin(QVector3D(boundingRect().width() / 2, boundingRect().height() / 2, 0));

    QList<QGraphicsTransform *> transforms;
    transforms << scaleTransform;
    setTransformations(transforms);

    setOpacity(0.0);

    titleItem = new QGraphicsTextItem(this);
    setTitle(title);
}

void Window::addContent(const QString &content)
{
    QGraphicsTextItem *content_item = new QGraphicsTextItem(this);
    content_item->moveBy(15, 40);
    content_item->setHtml(content);
    content_item->setDefaultTextColor(Qt::white);
    content_item->setTextWidth(size.width() - 30);

    QFont *font = new QFont();
    font->setBold(true);
    font->setPointSize(10);
    content_item->setFont(*font);
}

Button *Window::addCloseButton(const QString &label)
{
    Button *ok_button = new Button(label, 0.6);
    QFont font = Config.TinyFont;
    font.setBold(true);
    ok_button->setFont(font);
    ok_button->setParentItem(this);

    qreal x = size.width() - ok_button->boundingRect().width() - 25;
    qreal y = size.height() - ok_button->boundingRect().height() - 25;
    ok_button->setPos(x, y);

    connect(ok_button, &Button::clicked, this, &Window::disappear);
    return ok_button;
}

void Window::shift(int pos_x, int pos_y)
{
    resetTransform();
    setTransform(QTransform::fromTranslate((pos_x - size.width()) / 2, (pos_y - size.height()) / 2), true);
}

void Window::keepWhenDisappear()
{
    keep_when_disappear = true;
}

QRectF Window::boundingRect() const
{
    return QRectF(QPointF(), size);
}

void Window::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    QRectF window_rect = boundingRect();

    painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    painter->drawImage(window_rect, *outimg);
}

void Window::appear()
{
    QPropertyAnimation *scale_x = new QPropertyAnimation(scaleTransform, "xScale");
    QPropertyAnimation *scale_y = new QPropertyAnimation(scaleTransform, "yScale");
    QPropertyAnimation *opacity = new QPropertyAnimation(this, "opacity");
    QParallelAnimationGroup *group = new QParallelAnimationGroup();

    scale_x->setEndValue(1);
    scale_y->setEndValue(1);
    opacity->setEndValue(1.0);
    group->addAnimation(scale_x);
    group->addAnimation(scale_y);
    group->addAnimation(opacity);

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void Window::disappear()
{
    QPropertyAnimation *scale_x = new QPropertyAnimation(scaleTransform, "xScale");
    QPropertyAnimation *scale_y = new QPropertyAnimation(scaleTransform, "yScale");
    QPropertyAnimation *opacity = new QPropertyAnimation(this, "opacity");
    QParallelAnimationGroup *group = new QParallelAnimationGroup();

    scale_x->setEndValue(1.05);
    scale_y->setEndValue(0.95);
    opacity->setEndValue(0.0);
    group->addAnimation(scale_x);
    group->addAnimation(scale_y);
    group->addAnimation(opacity);

    group->start(QAbstractAnimation::DeleteWhenStopped);

    if (!keep_when_disappear)
        connect(group, &QAbstractAnimation::finished, this, &QObject::deleteLater);
}

void Window::setTitle(const QString &title)
{
    QString style;
    style.append(QStringLiteral("font-size:18pt; "));
    style.append(QStringLiteral("color:#77c379; "));
    style.append(QStringLiteral("font-family: %1").arg(Config.SmallFont.family()));

    QString content;
    content.append(QStringLiteral("<h style=\"%1\">%2</h>").arg( style , title));

    titleItem->setHtml(content);
    titleItem->setPos(size.width() / 2 - titleItem->boundingRect().width() / 2, 10);
}
