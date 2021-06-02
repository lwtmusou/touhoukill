#ifndef _CHAT_WIDGET_H
#define _CHAT_WIDGET_H

#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QIcon>
#include <QObject>
#include <QPixmap>
#include <QPushButton>

class MyPixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

public:
    explicit MyPixmapItem(const QPixmap &pixmap, QGraphicsItem *parentItem = nullptr);
    ~MyPixmapItem() override = default;

public:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
    void setSize(int x, int y);
    QString itemName;

private:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void initFaceBoardPos();
    void initEasyTextPos();
    int mouseCanClick(int x, int y);
    int mouseOnIcon(int x, int y);
    int mouseOnText(int x, int y);

    int sizex;
    int sizey;
    QList<QRect> faceboardPos;
    QList<QRect> easytextPos;
    QStringList easytext;

signals:
    void my_pixmap_item_msg(QString);
};

class ChatWidget : public QGraphicsObject
{
    Q_OBJECT

public:
    ChatWidget();
    ~ChatWidget() override = default;
    QRectF boundingRect() const override;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    QPixmap base_pixmap;
    QPushButton *returnButton;
    QPushButton *chatfaceButton;
    QPushButton *easytextButton;
    MyPixmapItem *chat_face_board, *easy_text_board;
    QGraphicsRectItem *base;

    QGraphicsProxyWidget *addWidget(QWidget *widget, int x);
    QPushButton *addButton(const QString &name, int x);
    QPushButton *createButton(const QString &name);

private slots:
    void showEasyTextBoard();
    void showFaceBoard();
    void sendText();

signals:
    void chat_widget_msg(QString);
    void return_button_click();
};

#endif
