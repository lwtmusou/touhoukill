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
    HeroSkinContainer(const QString &generalName,
        const QString &kingdom, QGraphicsItem *parent = 0);

    ~HeroSkinContainer() {
        if (this == m_currentTopMostContainer) {
            m_currentTopMostContainer = NULL;
        }
    }

    const QString &getGeneralName() const { return m_generalName; }

    void bringToTopMost();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

    static bool hasSkin(const QString &generalName);
    static int getNextSkinIndex(const QString &generalName, int skinIndex);

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *);
    virtual void wheelEvent(QGraphicsSceneWheelEvent *event);

private:
    //CloseButton *close_button;
	void initSkins();
    void createSkinItem(int skinIndex, QGraphicsItem *parent, bool used = false);
    void fillSkins();
    void swapWithSkinItemUsed(int skinIndex);

    static QStringList getHeroSkinFiles(const QString &generalName);
    static const SanShadowTextFont &getAvatarNameFont();

private:
    const QString m_generalName;
    const QPixmap m_backgroundPixmap;

    QList<SkinItem *> m_skins;
    QMap<int, SkinItem *> m_skinIndexToItem;

    //本类对ZValue的处理，是为了保证窗口在被选中或移动过程中，
    //始终保持在最上层，不被其他QGraphicsItem对象遮掩
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
    void skin_changed(const QString &generalName);
};

#endif // HEROSKINCONTAINER_H
