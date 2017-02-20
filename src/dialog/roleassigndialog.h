#ifndef _ROLE_ASSIGN_DIALOG_H
#define _ROLE_ASSIGN_DIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QListWidget>
#include <QMap>

class RoleAssignDialog : public QDialog
{
    Q_OBJECT

public:
    RoleAssignDialog(QWidget *parent);

protected:
    virtual void accept();
    virtual void reject();

private:
    QListWidget *list;
    QComboBox *role_ComboBox;
    QMap<QString, QString> role_mapping;

private slots:
    void updateRole(int index);
    void updateRole(QListWidgetItem *current);
    void moveUp();
    void moveDown();
};

#endif
