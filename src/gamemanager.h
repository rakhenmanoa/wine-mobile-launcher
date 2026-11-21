#pragma once

#include <QHash>
#include <QObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSet>
#include <QSettings>
#include <QString>
#include <QUrl>
#include <QVariantMap>
// QML_SINGLETON / QML_ELEMENT sont définis par QtQml (qqmlintegration.h), pas par QtCore.
#include <QtQml>

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
    Q_PROPERTY(QString defaultProtonDir READ defaultProtonDir
               WRITE setDefaultProtonDir NOTIFY defaultProtonDirChanged)
    Q_PROPERTY(QString defaultPrefixDir READ defaultPrefixDir
               WRITE setDefaultPrefixDir NOTIFY defaultPrefixDirChanged)
    Q_PROPERTY(bool verboseTerminal READ verboseTerminal
               WRITE setVerboseTerminal NOTIFY verboseTerminalChanged)
    Q_PROPERTY(QString viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    // Vrai tant qu'un jeu lancé avec la manette à l'écran tourne
    Q_PROPERTY(bool gamepadActive READ gamepadActive NOTIFY gamepadActiveChanged)

public:
    explicit GameManager(QObject *parent = nullptr);

    GameModel *model() const { return m_model; }
    QString logOutput() const { return m_log; }
    QString defaultWineBinary() const { return m_defaultWineBinary; }
    void setDefaultWineBinary(const QString &binary);
    QString defaultProtonDir() const { return m_defaultProtonDir; }
    void setDefaultProtonDir(const QString &dir);
    QString defaultPrefixDir() const { return m_defaultPrefixDir; }
    void setDefaultPrefixDir(const QString &dir);
    bool verboseTerminal() const { return m_verboseTerminal; }
    void setVerboseTerminal(bool verbose);
    QString viewMode() const { return m_viewMode; }
    bool gamepadActive() const { return !m_gamepadGames.isEmpty(); }

    void setViewMode(const QString &mode);

    Q_INVOKABLE void addGame(const QString &name, const QString &exePath,
                             const QString &args, const QString &prefixPath,
                             const QString &wineBinary, const QString &protonDir,
                             const QString &iconPath, bool mangohud, bool fullscreen,
                             bool gamepad = false);
    Q_INVOKABLE void updateGame(int row, const QString &name, const QString &exePath,
                                const QString &args, const QString &prefixPath,
                                const QString &wineBinary, const QString &protonDir,
                                const QString &iconPath, bool mangohud, bool fullscreen,
                                bool gamepad = false);
    Q_INVOKABLE void removeGame(int row);
    // Re-extrait l'icône de l'exécutable d'un jeu (remplace l'icône actuelle).
    Q_INVOKABLE void refreshIcon(int row);

    Q_INVOKABLE void launchGame(int row);
    Q_INVOKABLE void stopGame(int row);

    Q_INVOKABLE void createPrefix(const QString &prefixPath);
    Q_INVOKABLE QString detectWine(const QString &overrideBinary,
                                  const QString &overrideProton = {}) const;

    Q_INVOKABLE QString urlToPath(const QUrl &url) const;
    Q_INVOKABLE void openPath(const QString &path) const;
    Q_INVOKABLE void clearLog();
    // Fichier où le journal est aussi enregistré quand le mode verbose est actif.
    Q_INVOKABLE QString logFilePath() const;

signals:
    void logChanged();
    void defaultWineBinaryChanged();
    void defaultProtonDirChanged();
    void defaultPrefixDirChanged();
    void verboseTerminalChanged();
    void viewModeChanged();
    void prefixCreationFinished(const QString &path, bool success, const QString &message);
    void gameLaunched(int row, bool success, const QString &message);
    void gamepadActiveChanged();
    void gamepadUnavailable(const QString &message);

private:
    void loadLibrary();
    void saveLibrary();
    QString libraryFile() const;
    // « Runner » : ce qui exécute réellement le jeu. Soit un binaire Wine
    // classique (wine, Wine-GE, wine64…), soit un dossier Proton / GE-Proton
    // (script « proton », y compris les builds aarch64), lancé via « proton run ».
    struct Runner {
        enum class Kind { None, Wine, Proton };
        Kind kind = Kind::None;
        QString program; // Wine : binaire wine ; Proton : script « proton »
        QString root;    // Proton : racine du dossier GE-Proton
        QString error;   // raison de l'échec quand kind == None
    };
    Runner resolveRunner(const QString &overrideBinary,
                         const QString &overrideProton = {}) const;
    QString runnerWineserver(const Runner &runner) const;
    QString effectiveWinePrefix(const Runner &runner, const QString &prefix) const;
    QProcessEnvironment runnerEnvironment(const Runner &runner, const QString &prefix) const;
    QString runnerCommand(const Runner &runner, const QStringList &payload,
                          QStringList &arguments) const;
    void logArchitectureHint(const Runner &runner);
    QString defaultPrefixFor(const QString &gameName) const;
    void stopForGame(const WineGame &game);
    // Plein écran : installe des règles de fenêtre KWin (kwinrulesrc) qui
    // forcent le plein écran de sway imbriqué / du bureau virtuel Wine.
    void ensureFullscreenWindowRules(bool enable);
    void applyFullscreenScript();
    void setGamepadGame(const QString &gameId, bool active, int rotation = 0);
    void appendLog(const QString &line);
    void writeLogFile(const QString &line);
    void appendProcessOutput(const QString &gameName, const QString &text);
    // Dossier de cache des icônes extraites des exécutables.
    QString iconCacheDir() const;
    // Vrai si le chemin d'icône pointe dans le cache automatique (et non
    // vers une image choisie manuellement par l'utilisateur).
    bool isAutoIconPath(const QString &path) const;
    // Extrait l'icône embarquée d'un .exe (wrestool, paquet icoutils) vers un
    // PNG en cache, en gardant la plus grande variante (256×256 quand elle
    // existe) ; renvoie le chemin, ou vide avec la raison dans *error.
    QString extractExeIcon(const WineGame &game, QString *error = nullptr);

    GameModel *m_model = nullptr;
    QHash<QString, QProcess *> m_processes; // id de jeu -> processus wine
    QString m_log;
    QSettings m_settings;
    QString m_defaultWineBinary;
    QString m_defaultProtonDir;
    QString m_defaultPrefixDir;
    bool m_verboseTerminal = false;
    QString m_viewMode;
    QProcess *m_overlayProcess = nullptr; // « --gamepad-overlay » : manette virtuelle + overlay
    QSet<QString> m_gamepadGames; // jeux en cours avec la manette à l'écran
    int m_gamepadRotation = 0;
};
