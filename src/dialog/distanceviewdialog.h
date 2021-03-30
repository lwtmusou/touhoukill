#ifndef _DISTANCE_VIEW_DIALOG_H
#define _DISTANCE_VIEW_DIALOG_H

class ClientPlayer;

#include <QDialog>

class DistanceViewDialogUI;

class DistanceViewDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DistanceViewDialog(QWidget *parent = nullptr);
    ~DistanceViewDialog() override;

private:
    DistanceViewDialogUI *ui;

private slots:
    void showDistance();
};

#endif
