#include "util.h"

#include <QMessageBox>
#include <QStringList>
#include <QVariant>

QStringList IntList2StringList(const QList<int> &intlist)
{
    QStringList stringlist;
    for (int i = 0; i < intlist.size(); i++)
        stringlist.append(QString::number(intlist.at(i)));
    return stringlist;
}

QList<int> StringList2IntList(const QStringList &stringlist)
{
    QList<int> intlist;
    for (int i = 0; i < stringlist.size(); i++) {
        QString n = stringlist.at(i);
        bool ok;
        intlist.append(n.toInt(&ok));
        if (!ok)
            return QList<int>();
    }
    return intlist;
}

QVariantList IntList2VariantList(const QList<int> &intlist)
{
    QVariantList variantlist;
    for (int i = 0; i < intlist.size(); i++)
        variantlist.append(QVariant(intlist.at(i)));
    return variantlist;
}

QList<int> VariantList2IntList(const QVariantList &variantlist)
{
    QList<int> intlist;
    for (int i = 0; i < variantlist.size(); i++) {
        QVariant n = variantlist.at(i);
        bool ok;
        intlist.append(n.toInt(&ok));
        if (!ok)
            return QList<int>();
    }
    return intlist;
}

bool isNormalGameMode(const QString &mode)
{
    return mode.endsWith("p") || mode.endsWith("pd") || mode.endsWith("pz");
}

bool isHegemonyGameMode(const QString &mode)
{
    return mode.startsWith("hegemony");
}
