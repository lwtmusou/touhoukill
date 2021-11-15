#ifndef QSANSTYLE_H
#define QSANSTYLE_H

#include <QObject>
#include <QString>
#include <QtQml>

class QSanStyleFactoryPrivate;

class QSanStyleFactory : public QObject
{
    Q_OBJECT

public:
    QSanStyleFactory(QObject *parent = nullptr);
    ~QSanStyleFactory();
    Q_INVOKABLE QJSValue styleInstance(const QString &styleName);

private:
    Q_DECLARE_PRIVATE(QSanStyleFactory)
    QSanStyleFactoryPrivate *d_ptr;
};

//class QSanStyle : public QObject
//{
//    Q_OBJECT
//    Q_PROPERTY(QJSValue style READ style WRITE setStyle STORED false NOTIFY styleChanged)

//    friend class QSanStyleFactory;

//private:
//    QSanStyle(const QString &styleName, QObject *parent = nullptr);
//    void load(const QString &styleName);

//signals:
//    void styleChanged(const QJSValue &newStyle);
//};

//QML_DECLARE_TYPE(QSanStyle)

#endif
