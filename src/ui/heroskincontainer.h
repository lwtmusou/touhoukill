#ifndef HEROSKINCONTAINER_H
#define HEROSKINCONTAINER_H

//#include "cardcontainer.h"
//#include "QSanSelectableItem.h"
//#include "carditem.h"
//#include "GenericCardContainerUI.h"

#include <QGraphicsObject>

class SanShadowTextFont;
class SkinItem;
class QScrollBar;

class HeroSkinContainer : public QGraphicsObject
{
    Q_OBJECT

public:
    HeroSkinContainer(const QString &generalName, const QString &kingdom, QGraphicsItem *parent = 0);

    ~HeroSkinContainer()
    {
        if (this == m_currentTopMostContainer) {
            m_currentTopMostContainer = NULL;
        }
    }

    const QString &getGeneralName() const
    {
        return m_generalName;
    }

    void bringToTopMost();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

    static bool hasSkin(const QString &generalName);
    static int getNextSkinIndex(const QString &generalName, int skinIndex);
    void swapWithSkinItemUsed(int skinIndex);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);

private:
    //CloseButton *close_button;
    void initSkins();
    void createSkinItem(int skinIndex, QGraphicsItem *parent, bool used = false);
    void fillSkins();
    //void swapWithSkinItemUsed(int skinIndex);

    static QStringList getHeroSkinFiles(const QString &generalName);
    static const SanShadowTextFont &getAvatarNameFont();

private:
    const QString m_generalName;
    const QPixmap m_backgroundPixmap;

    QList<SkinItem *> m_skins;
    QMap<int, SkinItem *> m_skinIndexToItem;

    qreal m_originalZValue;

    QScrollBar *m_vScrollBar;
    int m_oldScrollValue;

    static HeroSkinContainer *m_currentTopMostContainer;

    static QMap<QString, QStringList> m_generalToSkinFiles;
    static QMap<QString, bool> m_generalToHasSkin;

private slots:
    void close();
    void skinSelected(int skinIndex);
    void scrollBarValueChanged(int newValue);

signals:
    void local_skin_changed(const QString &generalName);
    void skin_changed(const QString &generalName, int skinIndex);
};

#endif // HEROSKINCONTAINER_H
