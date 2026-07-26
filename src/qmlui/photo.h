#ifndef TOUHOUKILL_PHOTO_H_
#define TOUHOUKILL_PHOTO_H_

#include <QQuickItem>

// Bridge class for Photo.qml. Holds target-selection state (driven by RoomScene)
// so QML only does visual binding. targetable: this Photo is a valid target for the
// currently selected card. targetSelected: this Photo is in the selected target list.
// playerName: the ClientPlayer objectName, set from QML so C++ can match targets.
// Registered as QML type "CppPhoto" (name kept for stability; class renamed to Photo).
class Photo : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool targetable READ targetable WRITE setTargetable NOTIFY targetableChanged)
    Q_PROPERTY(bool targetSelected READ targetSelected WRITE setTargetSelected NOTIFY targetSelectedChanged)
    Q_PROPERTY(QString playerName READ playerName WRITE setPlayerName NOTIFY playerNameChanged)

public:
    explicit Photo(QQuickItem *parent = nullptr);

    [[nodiscard]] bool targetable() const;
    void setTargetable(bool v);

    [[nodiscard]] bool targetSelected() const;
    void setTargetSelected(bool v);

    [[nodiscard]] QString playerName() const;
    void setPlayerName(const QString &v);

signals:
    void targetableChanged();
    void targetSelectedChanged();
    void playerNameChanged();

private:
    bool m_targetable = false;
    bool m_targetSelected = false;
    QString m_playerName;
};

#endif
