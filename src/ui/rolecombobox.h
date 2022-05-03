#ifndef _ROLE_COMBO_BOX_H
#define _ROLE_COMBO_BOX_H

#include <QObject>

#include "QSanSelectableItem.h"
#include "player.h"
#include "serverinfostruct.h"

class Photo;

class RoleComboBoxItem : public QSanSelectableItem
{
    Q_OBJECT

public:
    RoleComboBoxItem(const QString &role, int number, QSize size);
    QString getRole() const;
    void setRole(const QString &role);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

private:
    QString m_role;
    int m_number;
    QSize m_size;

signals:
    void clicked();
};

class RoleComboBox : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit RoleComboBox(QGraphicsItem *photo);
    static const int S_ROLE_COMBO_BOX_WIDTH = 25;
    static const int S_ROLE_COMBO_BOX_HEIGHT = 26;
    static const int S_ROLE_COMBO_BOX_GAP = 5;
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

public slots:
    void fix(const QString &role);

protected:
    qreal _m_posX, _m_posY;
    QString _m_fixedRole;

private:
    QList<RoleComboBoxItem *> items;
    RoleComboBoxItem *m_currentRole;

    static int getRoleIndex();

private slots:
    void collapse();
    void expand();
    void toggle();
};

#endif
