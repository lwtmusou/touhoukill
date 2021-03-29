#ifndef _ABOUT_US_H
#define _ABOUT_US_H

#include <QDialog>

class QListWidget;
class QTextBrowser;

class AboutUsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutUsDialog(QWidget *parent);

private:
    QListWidget *list;
    QTextBrowser *content_box;

private slots:
    void loadContent(int row);
};

#endif
