#include "AboutUs.h"
#include "engine.h"

#include <QFile>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTextBrowser>
#include <QTextStream>

AboutUsDialog::AboutUsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About Us"));
    resize(800, 600);

    list = new QListWidget;
    list->setMaximumWidth(150);

    content_box = new QTextBrowser;
    content_box->setOpenExternalLinks(true);
    content_box->setProperty("description", true);

    QHBoxLayout *layout = new QHBoxLayout;

    layout->addWidget(content_box);
    layout->addWidget(list);

    setLayout(layout);

    QStringList developers = Sanguosha->getConfigFromConfigFile("developers").toStringList();
    developers.prepend("hegemony");
    developers.prepend("TouhouSatsu");

    foreach (QString name, developers) {
        QListWidgetItem *item = new QListWidgetItem(name, list);
        item->setData(Qt::UserRole, name);
    }

    connect(list, SIGNAL(currentRowChanged(int)), this, SLOT(loadContent(int)));

    if (!developers.isEmpty())
        loadContent(0);
}

void AboutUsDialog::loadContent(int row)
{
    QString name = list->item(row)->data(Qt::UserRole).toString();
    QString filename = QString("developers/%1.htm").arg(name);
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");
        QString content = stream.readAll();
        content_box->setHtml(content);
    }
}
