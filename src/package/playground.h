#ifndef PLAYGROUND_H
#define PLAYGROUND_H

#include "card.h"
#include "package.h"
#include <QDialog>

class Fsu0413GainianDialog : public QDialog
{
    Q_OBJECT

public:
    static Fsu0413GainianDialog *getInstance(const QString &object);
    bool isResponseOk(const Player *player, const QString &pattern) const;

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit Fsu0413GainianDialog(const QString &object);

    void createButtons();

    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class PlaygroundPackage : public Package
{
    Q_OBJECT

public:
    PlaygroundPackage();
};

#endif // TH16_H
