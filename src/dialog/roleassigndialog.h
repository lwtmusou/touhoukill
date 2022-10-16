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
    explicit RoleAssignDialog(QWidget *parent);

protected:
    void accept() override;
    void reject() override;

private:
    QListWidget *list;
    QComboBox *role_ComboBox;
    QMap<QString, QString> role_mapping;

private:
    static const QMap<char, QString> roleMap;

private slots:
    void updateRole(int index);
    void updateRole(QListWidgetItem *current);
    void moveUp();
    void moveDown();
};

#endif
