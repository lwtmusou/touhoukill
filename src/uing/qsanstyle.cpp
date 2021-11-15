#include "qsanstyle.h"
#include <QCoreApplication>

class QSanStyleFactoryPrivate
{
public:
    QQmlApplicationEngine *engine;
    QHash<QString, QJSValue> styles;

    void load(const QString &styleName);
};

void QSanStyleFactoryPrivate::load(const QString &styleName)
{
    // not implemented
    // Q_UNUSED(styles[styleName]);

    QFile f(QString(QStringLiteral("styles/%1.js")).arg(styleName));
    if (f.exists()) {
        f.open(QIODevice::ReadOnly);
        QString program = QString::fromUtf8(f.readAll());
        QJSValue v = engine->evaluate(program, styleName);
        if (v.isError())
            qDebug("v.isError() = true, toString = %s", v.toString().toLocal8Bit().constData());
        else {
            if (v.isUndefined())
                qDebug("v = undefined");
            else if (v.isNull())
                qDebug("v = null");
            else {
                if (v.isObject()) {
                    QJSValueIterator it(v);
                    while (it.hasNext()) {
                        it.next();
                        qDebug() << it.name() << ": " << it.value().toString();
                    }
                }
            }
        }

        styles.insert(styleName, v);
    } else {
        styles.insert(styleName, QJSValue::NullValue);
    }
}

QSanStyleFactory::QSanStyleFactory(QObject *parent)
    : QObject(parent)
    , d_ptr(new QSanStyleFactoryPrivate)
{
    Q_D(QSanStyleFactory);

    d->engine = qobject_cast<QQmlApplicationEngine *>(parent);
    Q_ASSERT(d->engine != NULL);
}

QSanStyleFactory::~QSanStyleFactory()
{
    Q_D(QSanStyleFactory);
    delete d;
}

QJSValue QSanStyleFactory::styleInstance(const QString &styleName)
{
    Q_D(QSanStyleFactory);

    if (!d->styles.contains(styleName))
        d->load(styleName);

    return d->styles[styleName];
}

//QSanStyle::QSanStyle(const QString &styleName, QObject *parent)
//    : QObject(parent)
//    , start(new QObject(this))
//    , room(new QObject(this))
//    , image(new QObject(this))
//    , audio(new QObject(this))
//{
//    setObjectName(styleName);
//    load(styleName);
//}

//void QSanStyle::load(const QString &styleName)
//{
//    // not implemented
//    Q_UNUSED(styleName);
//}

//namespace {
//void registerQmlTypes()
//{
//    qmlRegisterUncreatableType<QSanStyle>("TouhouSatsu.Ui", 1, 0, "QSanStyle", "Use StyleFactory.styleInstance() for desired style");
//}
//}

//Q_COREAPP_STARTUP_FUNCTION(registerQmlTypes)
