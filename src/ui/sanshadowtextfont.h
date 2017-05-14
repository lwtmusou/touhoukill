#ifndef SANSHADOWTEXTFONT_H
#define SANSHADOWTEXTFONT_H

#include "sansimpletextfont.h"

#include <QPoint>

class QPixmap;

class SanShadowTextFont : public SanSimpleTextFont
{
public:
    SanShadowTextFont();
    explicit SanShadowTextFont(const QString &fontName);
    SanShadowTextFont(const QString &fontName, const QSize &fontSize, int shadowRadius, double shadowDecadeFactor, const QColor &shadowColor = Qt::black,
                      const QPoint &shadowOffset = QPoint(0, 0), const QColor &color = Qt::white, int spacing = 0, int weight = 0);

    int shadowRadius() const
    {
        return m_shadowRadius;
    }
    void setShadowRadius(int shadowRadius)
    {
        m_shadowRadius = shadowRadius;
    }

    double shadowDecadeFactor() const
    {
        return m_shadowDecadeFactor;
    }
    void setShadowDecadeFactor(double shadowDecadeFactor)
    {
        m_shadowDecadeFactor = shadowDecadeFactor;
    }

    const QColor &shadowColor() const
    {
        return m_shadowColor;
    }
    void setShadowColor(const QColor &shadowColor)
    {
        m_shadowColor = shadowColor;
    }

    const QPoint &shadowOffset() const
    {
        return m_shadowOffset;
    }
    void setShadowOffset(const QPoint &shadowOffset)
    {
        m_shadowOffset = shadowOffset;
    }

    bool tryParse(const QVariant &arg);

    void paintText(QPainter *const painter, const QRect &pos, const Qt::Alignment &align, const QString &text) const;

    // this function's prototype is confusing. It will CLEAR ALL contents on the
    // QGraphicsPixmapItem passed in and then start drawing.
    void paintText(QGraphicsPixmapItem *const item, const QRect &pos, const Qt::Alignment &align, const QString &text) const;

private:
    bool _paintTextHelper(const QRect &pos, const Qt::Alignment &align, const QString &text, QPixmap &pixmap) const;

    int m_shadowRadius;
    double m_shadowDecadeFactor;
    QColor m_shadowColor;
    QPoint m_shadowOffset;
};

#endif // SANSHADOWTEXTFONT_H
