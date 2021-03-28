#ifndef _CHOOSE_GENERAL_DIALOG_H
#define _CHOOSE_GENERAL_DIALOG_H

class General;

#include "TimedProgressBar.h"

#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QToolButton>

class OptionButton : public QToolButton
{
    Q_OBJECT

public:
    explicit OptionButton(const QString &icon_path, const QString &caption = "", QWidget *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QMouseEvent *) override;

signals:
    void double_clicked();
};

class ChooseGeneralDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseGeneralDialog(const QStringList &general_names, QWidget *parent, bool view_only = false, const QString &title = QString());

public slots:
    void done(int) override;

protected:
    QDialog *m_freeChooseDialog;

private:
    QSanCommandProgressBar *progress_bar;

private slots:
    void freeChoose();
};

class FreeChooseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FreeChooseDialog(QWidget *parent, bool pair_choose = false);

private:
    QButtonGroup *group;
    bool pair_choose;
    QWidget *createTab(const QList<const General *> &generals);

private slots:
    void chooseGeneral();
    void uncheckExtraButton(QAbstractButton *button);

signals:
    void general_chosen(const QString &name);
    void pair_chosen(const QString &first, const QString &second);
};

#endif
