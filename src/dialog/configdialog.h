#ifndef _CONFIG_DIALOG_H
#define _CONFIG_DIALOG_H

#include <QDialog>
#include <QLineEdit>

namespace Ui {
class ConfigDialog;
}

class ConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConfigDialog(QWidget *parent = nullptr);
    ~ConfigDialog() override;

private:
    Ui::ConfigDialog *ui;
    void showFont(QLineEdit *lineedit, const QFont &font);

private slots:
    void on_setTextEditColorButton_clicked();
    void on_setTextEditFontButton_clicked();
    void on_changeAppFontButton_clicked();
    void on_resetBgMusicButton_clicked();
    void on_browseBgMusicButton_clicked();
    void on_resetBgButton_clicked();
    void on_browseBgButton_clicked();
    void on_resetTableBgButton_clicked();
    void on_browseTableBgButton_clicked();
    void on_resetRecordPathButton_clicked();
    void on_browseRecordPathButton_clicked();
    void saveConfig();

signals:
    void bg_changed();
    void tableBg_changed();
};

#endif
