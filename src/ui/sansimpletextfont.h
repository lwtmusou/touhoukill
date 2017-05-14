#ifndef SANSIMPLETEXTFONT_H
#define SANSIMPLETEXTFONT_H

#include <QColor>
#include <QHash>
#include <QSize>
#include <Qt>

#include "json.h"

class QString;
class QPainter;
class QRect;
class QGraphicsPixmapItem;

class SanSimpleTextFont
{
public:
    SanSimpleTextFont();
    explicit SanSimpleTextFont(const QString &fontName);
    SanSimpleTextFont(const QString &fontName, const QSize &fontSize, const QColor &color = Qt::white, int spacing = 0, int weight = 0);

    const QSize &size() const
    {
        return m_fontSize;
    }
    void setSize(const QSize &size)
    {
        m_fontSize = size;
    }

    const QColor &color() const
    {
        return m_color;
    }
    void setColor(const QColor &color)
    {
        m_color = color;
    }

    int spacing() const
    {
        return m_spacing;
    }
    void setSpacing(int spacing)
    {
        m_spacing = spacing;
    }

    int weight() const
    {
        return m_weight;
    }
    void setWeight(int weight)
    {
        m_weight = weight;
    }

    bool tryParse(const QVariant &arg);

    void paintText(QPainter *const painter, const QRect &pos, const Qt::Alignment &align, const QString &text) const;

    // this function's prototype is confusing. It will CLEAR ALL contents on the
    // QGraphicsPixmapItem passed in and then start drawing.
    void paintText(QGraphicsPixmapItem *const item, const QRect &pos, const Qt::Alignment &align, const QString &text) const;

protected:
    void _initFontFace(const QString &fontName);

    const int *m_fontFace;
    QSize m_fontSize;
    QColor m_color;
    int m_spacing;
    int m_weight;
    bool m_vertical;

    static QHash<QString, const int *> m_fontBank;
};

#endif // SANSIMPLETEXTFONT_H
