#pragma once

#include <QHash>
#include <QObject>
#include <QProcess>
#include <QSettings>
#include <QString>
#include <QUrl>
#include <QVariantMap>

#include "gamemodel.h"

// Cœur de l'application : bibliothèque persistante (JSON), lancement Wine,
// arrêt via wineserver, création de préfixes, journal d'exécution.
class GameManager : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

    Q_PROPERTY(GameModel* model READ model CONSTANT)
    Q_PROPERTY(QString logOutput READ logOutput NOTIFY logChanged)
    Q_PROPERTY(QString defaultWineBinary READ defaultWineBinary
               WRITE setDefaultWineBinary NOTIFY defaultWineBinaryChanged)
    Q_PROPERTY(QString defaultPrefixDir READ defaultPrefixDir
               WRITE setDefaultPrefixDir NOTIFY defaultPrefixDirChanged)

public:
    explicit GameManager(QObject *parent = nullptr);

    GameModel *model() const { return m_model; }
    QString logOutput() const { return m_log; }
    QString defaultWineBinary() const { return m_defaultWineBinary; }
    void setDefaultWineBinary(const QString &binary);
    QString defaultPrefixDir() const { return m_defaultPrefixDir; }
    void setDefaultPrefixDir(const QString &dir);

    Q_INVOKABLE void addGame(const QString &name, const QString &exePath,
                             const QString &args, const QString &prefixPath,
                             const QString &wineBinary, const QString &iconPath);
    Q_INVOKABLE void updateGame(int row, const QString &name, const QString &exePath,
                                const QString &args, const QString &prefixPath,
                                const QString &wineBinary, const QString &iconPath);
    Q_INVOKABLE void removeGame(int row);

    Q_INVOKABLE void launchGame(int row);
    Q_INVOKABLE void stopGame(int row);

    Q_INVOKABLE void createPrefix(const QString &prefixPath);
    Q_INVOKABLE QString detectWine(const QString &overrideBinary) const;

    Q_INVOKABLE QString urlToPath(const QUrl &url) const;
    Q_INVOKABLE void openPath(const QString &path) const;
    Q_INVOKABLE void clearLog();

signals:
    void logChanged();
    void defaultWineBinaryChanged();
    void defaultPrefixDirChanged();
    void prefixCreationFinished(const QString &path, bool success, const QString &message);
    void gameLaunched(int row, bool success, const QString &message);

private:
    void loadLibrary();
    void saveLibrary();
    QString libraryFile() const;
    QString resolveWine(const QString &overrideBinary) const;
    QString defaultPrefixFor(const QString &gameName) const;
    void stopForGame(const WineGame &game);
    void appendLog(const QString &line);

    GameModel *m_model = nullptr;
    QHash<QString, QProcess *> m_processes; // id de jeu -> processus wine
    QString m_log;
    QSettings m_settings;
    QString m_defaultWineBinary;
    QString m_defaultPrefixDir;
};
