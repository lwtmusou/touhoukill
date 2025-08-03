#ifndef TOUHOUKILL_QMLUI_H_
#define TOUHOUKILL_QMLUI_H_

#include <QObject>
#include <QUrl>

class TouhouKillQmlUiGlobal : public QObject
{
    Q_OBJECT

public:
    Q_INVOKABLE QUrl getUrl(const QString &path) const;
};

#endif
