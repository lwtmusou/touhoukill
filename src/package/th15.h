#ifndef TH15_H
#define TH15_H

#include "card.h"
#include "package.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QVBoxLayout>

class YidanDialog : public QDialog
{
    Q_OBJECT

public:
    static YidanDialog *getInstance();

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit YidanDialog();

    QAbstractButton *createButton(const QString &name);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

signals:
    void onButtonClick();
};

class TH15Package : public Package
{
    Q_OBJECT

public:
    TH15Package();
};

#endif
