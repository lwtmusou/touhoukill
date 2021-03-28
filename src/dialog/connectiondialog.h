#ifndef _CONNECTION_DIALOG_H
#define _CONNECTION_DIALOG_H

#include "engine.h"
#include "general.h"

#include <QAbstractListModel>
#include <QButtonGroup>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>

class QLabel;
class UdpDetector;

class AvatarModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit AvatarModel(const QList<const General *> &list);

    int rowCount(const QModelIndex &) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private:
    QList<const General *> list;
};

class ConnectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionDialog(QWidget *parent);
    ~ConnectionDialog() override;
    void hideAvatarList();
    void showAvatarList();

public slots:
    void accept() override;

private slots:
    void on_detectLANButton_clicked();
    void on_clearHistoryButton_clicked();
    void on_avatarList_doubleClicked(const QModelIndex &index);
    void on_changeAvatarButton_clicked();
    void on_fillreconnect_clicked();

private:
    QLineEdit *nameLineEdit;
    QComboBox *hostComboBox;
    QLabel *avatarPixmap;
    QListView *avatarList;

    QSize shrinkSize;
    QSize expandSize;

private:
    void showEvent(QShowEvent *e) override;
};

class UdpDetectorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UdpDetectorDialog(QDialog *parent);

private:
    QListWidget *list;
    UdpDetector *detector;
    QPushButton *detect_button;

private slots:
    void startDetection();
    void stopDetection();
    void chooseAddress(QListWidgetItem *item);
    void addServerAddress(const QString &server_name, const QString &address);

signals:
    void address_chosen(const QString &address);
};

#endif
