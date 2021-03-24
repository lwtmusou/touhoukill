#include "QSanSelectableItem.h"
#include "uiUtils.h"

#include <QGraphicsColorizeEffect>
#include <QImageReader>
#include <QMessageBox>
#include <QPainter>

QSanSelectableItem::QSanSelectableItem(const QString &filename, bool center_as_origin)
{
    load(filename, center_as_origin);
    markable = false;
    marked = false;
    _m_width = _m_mainPixmap.width();
    _m_height = _m_mainPixmap.height();
}

bool QSanSelectableItem::load(const QString &filename, bool center_as_origin)
{
    return _load(filename, QSize(), false, center_as_origin);
}

bool QSanSelectableItem::load(const QString &filename, QSize size, bool center_as_origin)
{
    return _load(filename, size, true, center_as_origin);
}

bool QSanSelectableItem::_load(const QString &filename, QSize size, bool useNewSize, bool center_as_origin)
{
    bool success = _m_mainPixmap.load(filename);

    if (!success) {
        QImageReader reader(filename);
        QString error_string = reader.errorString();
        QString warning = tr("Can not load image %1[%2], error string is %3").arg(filename).arg(metaObject()->className()).arg(error_string);
        QMessageBox::warning(nullptr, tr("Warning"), warning);
    } else {
        if (useNewSize) {
            _m_width = size.width();
            _m_height = size.height();
        } else {
            _m_width = _m_mainPixmap.width();
            _m_height = _m_mainPixmap.height();
        }
        if (center_as_origin) {
            resetTransform();
            setTransform(QTransform::fromTranslate(-_m_width / 2, -_m_height / 2), true);
        } else
            prepareGeometryChange();
    }
    return success;
}

void QSanSelectableItem::setPixmap(const QPixmap &pixmap)
{
    _m_mainPixmap = pixmap;
    prepareGeometryChange();
}

QSanSelectableItem::QSanSelectableItem(bool center_as_origin)
    : markable(false)
    , marked(false)
{
    if (center_as_origin) {
        resetTransform();
        setTransform(QTransform::fromTranslate(-_m_mainPixmap.width() / 2, -_m_mainPixmap.height() / 2), true);
    }
    _m_width = _m_height = 0;
}

QRectF QSanSelectableItem::boundingRect() const
{
    return QRectF(0, 0, _m_width, _m_height);
}

void QSanSelectableItem::makeGray()
{
    QSanUiUtils::makeGray(_m_mainPixmap);
}

void QSanSelectableItem::scaleSmoothly(qreal ratio)
{
    QTransform trans = transform();
    trans.scale(ratio, ratio);
    setTransform(trans);
}

void QSanSelectableItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->drawPixmap(QRect(0, 0, _m_width, _m_height), _m_mainPixmap);
}

QVariant QSanSelectableItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemSelectedHasChanged) {
        if (value.toBool()) {
            QGraphicsColorizeEffect *effect = new QGraphicsColorizeEffect(this);
            effect->setColor(QColor(0xCC, 0x00, 0x00));
            setGraphicsEffect(effect);
        } else
            setGraphicsEffect(nullptr);

        emit selected_changed();
    } else if (change == ItemEnabledHasChanged) {
        emit enable_changed();
    }

    return QGraphicsObject::itemChange(change, value);
}

bool QSanSelectableItem::isMarked() const
{
    return markable && marked;
}

bool QSanSelectableItem::isMarkable() const
{
    return markable;
}

void QSanSelectableItem::mark(bool marked)
{
    if (markable) {
        if (this->marked != marked) {
            this->marked = marked;
            emit mark_changed();
        }
    }
}

void QSanSelectableItem::setMarkable(bool markable)
{
    this->markable = markable;
}
