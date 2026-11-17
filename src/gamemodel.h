#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>
#include <QVariantMap>
// QML_ELEMENT est défini par QtQml (qqmlintegration.h), pas par QtCore.
#include <QtQml>

// Une entrée de la bibliothèque : une application Windows lancée via Wine.
struct WineGame {
    QString id;
    QString name;
    QString exePath;
    QString args;
    QString prefixPath;
    QString wineBinary;
    QString iconPath;
    qint64 lastPlayed = 0;
};

class GameModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ExePathRole,
        ArgsRole,
        PrefixPathRole,
        WineBinaryRole,
        IconPathRole,
        LastPlayedRole,
        RunningRole
    };
    Q_ENUM(Roles)

    explicit GameModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE QVariantMap get(int row) const;

    void addGame(const WineGame &game);
    void updateGame(int row, const WineGame &game);
    void removeGame(int row);
    void setRunning(const QString &id, bool running);

    const WineGame &gameAt(int row) const;
    const QList<WineGame> &games() const { return m_games; }

private:
    QList<WineGame> m_games;
    QSet<QString> m_runningIds;
};
