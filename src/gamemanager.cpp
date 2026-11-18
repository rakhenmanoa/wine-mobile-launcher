#include "gamemanager.h"

#include <cstdio>

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QStringList>
#include <QUuid>

GameManager::GameManager(QObject *parent)
    : QObject(parent)
{
    m_model = new GameModel(this);
    m_defaultWineBinary = m_settings.value(QStringLiteral("defaultWineBinary")).toString();
    const QString defaultPrefix =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/prefixes");
    m_defaultPrefixDir =
        m_settings.value(QStringLiteral("defaultPrefixDir"), defaultPrefix).toString();
    m_verboseTerminal = m_settings.value(QStringLiteral("verboseTerminal")).toBool();
    loadLibrary();
    appendLog(QStringLiteral("Bibliothèque chargée : %1 jeu(x).")
                  .arg(m_model->rowCount()));
}

void GameManager::setDefaultWineBinary(const QString &binary)
{
    if (m_defaultWineBinary == binary)
        return;
    m_defaultWineBinary = binary;
    m_settings.setValue(QStringLiteral("defaultWineBinary"), binary);
    Q_EMIT defaultWineBinaryChanged();
}

void GameManager::setDefaultPrefixDir(const QString &dir)
{
    if (m_defaultPrefixDir == dir)
        return;
    m_defaultPrefixDir = dir;
    m_settings.setValue(QStringLiteral("defaultPrefixDir"), dir);
    Q_EMIT defaultPrefixDirChanged();
}

void GameManager::setVerboseTerminal(bool verbose)
{
    if (m_verboseTerminal == verbose)
        return;
    m_verboseTerminal = verbose;
    m_settings.setValue(QStringLiteral("verboseTerminal"), verbose);
    Q_EMIT verboseTerminalChanged();
}

QString GameManager::libraryFile() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/library.json");
}

void GameManager::loadLibrary()
{
    QFile file(libraryFile());
    if (!file.exists())
        return;
    if (!file.open(QIODevice::ReadOnly)) {
        appendLog(QStringLiteral("Impossible de lire la bibliothèque : ") + file.errorString());
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonArray games = doc.object().value(QLatin1String("games")).toArray();
    for (const QJsonValue &value : games) {
        const QJsonObject obj = value.toObject();
        WineGame game;
        game.id = obj.value(QLatin1String("id")).toString();
        if (game.id.isEmpty())
            game.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        game.name = obj.value(QLatin1String("name")).toString();
        game.exePath = obj.value(QLatin1String("exe")).toString();
        game.args = obj.value(QLatin1String("args")).toString();
        game.prefixPath = obj.value(QLatin1String("prefix")).toString();
        game.wineBinary = obj.value(QLatin1String("wine")).toString();
        game.iconPath = obj.value(QLatin1String("icon")).toString();
        game.mangohud = obj.value(QLatin1String("mangohud")).toBool();
        game.lastPlayed = static_cast<qint64>(obj.value(QLatin1String("lastPlayed")).toDouble());
        m_model->addGame(game);
    }
}

void GameManager::saveLibrary()
{
    QJsonArray games;
    const auto &list = m_model->games();
    for (const WineGame &game : list) {
        QJsonObject obj;
        obj.insert(QLatin1String("id"), game.id);
        obj.insert(QLatin1String("name"), game.name);
        obj.insert(QLatin1String("exe"), game.exePath);
        obj.insert(QLatin1String("args"), game.args);
        obj.insert(QLatin1String("prefix"), game.prefixPath);
        obj.insert(QLatin1String("wine"), game.wineBinary);
        obj.insert(QLatin1String("icon"), game.iconPath);
        obj.insert(QLatin1String("mangohud"), game.mangohud);
        obj.insert(QLatin1String("lastPlayed"), double(game.lastPlayed));
        games.append(obj);
    }
    QJsonObject root;
    root.insert(QLatin1String("games"), games);

    QSaveFile file(libraryFile());
    if (!file.open(QIODevice::WriteOnly)) {
        appendLog(QStringLiteral("Impossible d'enregistrer la bibliothèque."));
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit())
        appendLog(QStringLiteral("Erreur d'écriture de la bibliothèque : ") + file.errorString());
}

QString GameManager::resolveWine(const QString &overrideBinary) const
{
    const QStringList candidates = {overrideBinary, m_defaultWineBinary, QStringLiteral("wine")};
    for (const QString &candidate : candidates) {
        if (candidate.isEmpty())
            continue;
        const QFileInfo info(candidate);
        if (info.isAbsolute()) {
            if (info.exists() && info.isFile())
                return candidate;
            continue;
        }
        const QString found = QStandardPaths::findExecutable(candidate);
        if (!found.isEmpty())
            return found;
    }
    return {};
}

QString GameManager::defaultPrefixFor(const QString &gameName) const
{
    static const QRegularExpression sanitize(QStringLiteral("[^A-Za-z0-9_.-]"));
    QString name = gameName;
    name.replace(sanitize, QStringLiteral("_"));
    if (name.isEmpty())
        name = QStringLiteral("jeu");
    return m_defaultPrefixDir + QLatin1Char('/') + name;
}

void GameManager::appendLog(const QString &line)
{
    const QString stamped = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"))
             + QStringLiteral("  ") + line;
    m_log += stamped + QStringLiteral("\n");
    if (m_log.size() > 16000)
        m_log = m_log.right(12000);
    Q_EMIT logChanged();
    // Mode verbose : miroir du journal vers stderr — visible en direct quand
    // l'application est lancée depuis un terminal (sortie Wine/Proton, erreurs,
    // cycle de vie des processus).
    if (m_verboseTerminal) {
        std::fprintf(stderr, "%s\n", qPrintable(stamped));
        std::fflush(stderr);
    }
}

void GameManager::clearLog()
{
    m_log.clear();
    Q_EMIT logChanged();
}

void GameManager::addGame(const QString &name, const QString &exePath, const QString &args,
                          const QString &prefixPath, const QString &wineBinary,
                          const QString &iconPath, bool mangohud)
{
    WineGame game;
    game.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    game.name = name.trimmed();
    game.exePath = exePath.trimmed();
    game.args = args.trimmed();
    game.prefixPath = prefixPath.trimmed();
    game.wineBinary = wineBinary.trimmed();
    game.iconPath = iconPath.trimmed();
    game.mangohud = mangohud;
    m_model->addGame(game);
    saveLibrary();
    appendLog(QStringLiteral("Jeu ajouté : ") + game.name);
}

void GameManager::updateGame(int row, const QString &name, const QString &exePath,
                             const QString &args, const QString &prefixPath,
                             const QString &wineBinary, const QString &iconPath,
                             bool mangohud)
{
    if (row < 0 || row >= m_model->rowCount())
        return;
    WineGame game = m_model->gameAt(row);
    game.name = name.trimmed();
    game.exePath = exePath.trimmed();
    game.args = args.trimmed();
    game.prefixPath = prefixPath.trimmed();
    game.wineBinary = wineBinary.trimmed();
    game.iconPath = iconPath.trimmed();
    game.mangohud = mangohud;
    m_model->updateGame(row, game);
    saveLibrary();
    appendLog(QStringLiteral("Jeu mis à jour : ") + game.name);
}

void GameManager::removeGame(int row)
{
    if (row < 0 || row >= m_model->rowCount())
        return;
    const WineGame game = m_model->gameAt(row);
    if (m_processes.contains(game.id)) {
        QProcess *proc = m_processes.value(game.id);
        proc->kill();
        m_processes.remove(game.id);
    }
    m_model->setRunning(game.id, false);
    stopForGame(game);
    m_model->removeGame(row);
    saveLibrary();
    appendLog(QStringLiteral("Jeu supprimé de la bibliothèque : ") + game.name);
}

void GameManager::launchGame(int row)
{
    if (row < 0 || row >= m_model->rowCount()) {
        Q_EMIT gameLaunched(row, false, QStringLiteral("Index de jeu invalide."));
        return;
    }
    const WineGame game = m_model->gameAt(row);
    if (m_processes.contains(game.id)) {
        appendLog(QStringLiteral("« ") + game.name + QStringLiteral(" » semble déjà lancé."));
        Q_EMIT gameLaunched(row, false, QStringLiteral("Ce jeu est déjà lancé."));
        return;
    }

    const QString wine = resolveWine(game.wineBinary);
    if (wine.isEmpty()) {
        appendLog(QStringLiteral("Wine introuvable : installez le paquet wine ou définissez ")
                  + QStringLiteral("un binaire personnalisé dans les paramètres."));
        Q_EMIT gameLaunched(row, false, QStringLiteral("Wine introuvable sur le système."));
        return;
    }

    const QFileInfo exeInfo(game.exePath);
    if (game.exePath.isEmpty() || !exeInfo.exists() || !exeInfo.isFile()) {
        appendLog(QStringLiteral("Exécutable introuvable : ") + game.exePath);
        Q_EMIT gameLaunched(row, false, QStringLiteral("Exécutable introuvable."));
        return;
    }

    QString prefix = game.prefixPath;
    if (prefix.isEmpty())
        prefix = defaultPrefixFor(game.name);
    if (!QDir().mkpath(prefix)) {
        Q_EMIT gameLaunched(row, false, QStringLiteral("Impossible de créer le préfixe : ") + prefix);
        return;
    }

    auto *proc = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("WINEPREFIX"), prefix);
    // Mode verbose : laisser Wine afficher ses canaux err/fixme (diagnostic
    // complet dans le journal et le terminal) ; sinon, sortie silencieuse.
    if (!m_verboseTerminal)
        env.insert(QStringLiteral("WINEDEBUG"), QStringLiteral("-all"));
    proc->setProcessEnvironment(env);

    QStringList arguments{game.exePath};
    const QStringList extra = game.args.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (!extra.isEmpty())
        arguments += extra;

    // MangoHud (option par jeu) : le HUD s'insère par LD_PRELOAD devant le
    // binaire Wine/umu-run, comme « mangohud wine jeu.exe ». Si le binaire
    // est absent, on lance quand même le jeu, sans overlay.
    QString hudBinary;
    if (game.mangohud) {
        hudBinary = QStandardPaths::findExecutable(QStringLiteral("mangohud"));
        if (hudBinary.isEmpty())
            appendLog(QStringLiteral("MangoHud introuvable (installez le paquet mangohud) : ")
                      + QStringLiteral("lancement sans overlay."));
    }
    if (hudBinary.isEmpty()) {
        proc->setProgram(wine);
    } else {
        appendLog(QStringLiteral("Overlay MangoHud activé pour « ") + game.name
                  + QStringLiteral(" »."));
        proc->setProgram(hudBinary);
        arguments.prepend(wine);
    }
    proc->setArguments(arguments);
    proc->setWorkingDirectory(exeInfo.absolutePath());

    connect(proc, &QProcess::readyReadStandardOutput, this, [this, game, proc]() {
        const QString out = QString::fromLocal8Bit(proc->readAllStandardOutput()).trimmed();
        if (!out.isEmpty())
            appendLog(QLatin1Char('[') + game.name + QStringLiteral("] ") + out);
    });
    connect(proc, &QProcess::readyReadStandardError, this, [this, game, proc]() {
        const QString err = QString::fromLocal8Bit(proc->readAllStandardError()).trimmed();
        if (!err.isEmpty())
            appendLog(QLatin1Char('[') + game.name + QStringLiteral("] ") + err);
    });
    connect(proc, &QProcess::errorOccurred, this, [this, game](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            appendLog(QStringLiteral("Impossible de démarrer « ") + game.name
                      + QStringLiteral(" » : binaire Wine invalide ?"));
            m_processes.remove(game.id);
            m_model->setRunning(game.id, false);
        }
    });
    connect(proc, &QProcess::finished, this, [this, game](int exitCode, QProcess::ExitStatus status) {
        m_processes.remove(game.id);
        m_model->setRunning(game.id, false);
        const QString detail = (status == QProcess::CrashExit)
            ? QStringLiteral("crash")
            : QStringLiteral("code %1").arg(exitCode);
        appendLog(QStringLiteral("« ") + game.name + QStringLiteral(" » terminé (") + detail
                  + QStringLiteral(")."));
    });

    WineGame updated = game;
    updated.lastPlayed = QDateTime::currentMSecsSinceEpoch();
    m_model->updateGame(row, updated);

    m_processes.insert(game.id, proc);
    m_model->setRunning(game.id, true);
    saveLibrary();
    appendLog(QStringLiteral("Lancement de « ") + game.name + QStringLiteral(" » (préfixe : ")
              + prefix + QStringLiteral(")…"));
    proc->start();
    Q_EMIT gameLaunched(row, true, {});
}

void GameManager::stopForGame(const WineGame &game)
{
    QString server = QStandardPaths::findExecutable(QStringLiteral("wineserver"));
    if (server.isEmpty()) {
        const QString wine = resolveWine(game.wineBinary);
        if (!wine.isEmpty()) {
            const QString candidate = QFileInfo(wine).absolutePath()
                                      + QStringLiteral("/wineserver");
            if (QFileInfo::exists(candidate))
                server = candidate;
        }
    }
    if (server.isEmpty()) {
        appendLog(QStringLiteral("wineserver introuvable, impossible d'arrêter « ")
                  + game.name + QStringLiteral(" »."));
        return;
    }

    const QString prefix = game.prefixPath.isEmpty() ? defaultPrefixFor(game.name)
                                                     : game.prefixPath;
    auto *proc = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("WINEPREFIX"), prefix);
    proc->setProcessEnvironment(env);
    proc->setProgram(server);
    proc->setArguments({QStringLiteral("-k")});
    connect(proc, &QProcess::finished, this, [this, game]() {
        appendLog(QStringLiteral("Arrêt demandé pour « ") + game.name
                  + QStringLiteral(" » (wineserver -k)."));
    });
    connect(proc, &QProcess::finished, proc, &QObject::deleteLater);
    proc->start();
}

void GameManager::stopGame(int row)
{
    if (row < 0 || row >= m_model->rowCount())
        return;
    stopForGame(m_model->gameAt(row));
}

void GameManager::createPrefix(const QString &prefixPath)
{
    const QString path = QDir::fromNativeSeparators(prefixPath.trimmed());
    if (path.isEmpty()) {
        Q_EMIT prefixCreationFinished(path, false, QStringLiteral("Chemin du préfixe vide."));
        return;
    }
    const QString wine = resolveWine({});
    if (wine.isEmpty()) {
        Q_EMIT prefixCreationFinished(path, false,
                                      QStringLiteral("Wine introuvable sur le système."));
        return;
    }
    if (!QDir().mkpath(path)) {
        Q_EMIT prefixCreationFinished(path, false,
                                      QStringLiteral("Impossible de créer le dossier : ") + path);
        return;
    }

    auto *proc = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("WINEPREFIX"), path);
    if (!m_verboseTerminal)
        env.insert(QStringLiteral("WINEDEBUG"), QStringLiteral("-all"));
    proc->setProcessEnvironment(env);
    proc->setProgram(wine);
    proc->setArguments({QStringLiteral("wineboot"), QStringLiteral("-i")});
    connect(proc, &QProcess::finished, this,
            [this, path](int exitCode, QProcess::ExitStatus status) {
                const bool ok = (status == QProcess::NormalExit && exitCode == 0);
                if (ok)
                    appendLog(QStringLiteral("Préfixe créé : ") + path);
                else
                    appendLog(QStringLiteral("Échec de création du préfixe ") + path
                              + QStringLiteral(" (code %1).").arg(exitCode));
                Q_EMIT prefixCreationFinished(path, ok,
                                              ok ? QStringLiteral("Préfixe Wine créé avec succès.")
                                                 : QStringLiteral("Échec de création (code %1).")
                                                       .arg(exitCode));
            });
    connect(proc, &QProcess::errorOccurred, this, [this, path](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            Q_EMIT prefixCreationFinished(path, false,
                                          QStringLiteral("Impossible de lancer wineboot."));
    });
    appendLog(QStringLiteral("Création du préfixe « ") + path
              + QStringLiteral(" » (wineboot -i)…"));
    proc->start();
}

QString GameManager::detectWine(const QString &overrideBinary) const
{
    return resolveWine(overrideBinary);
}

QString GameManager::urlToPath(const QUrl &url) const
{
    return url.isLocalFile() ? url.toLocalFile() : url.toString();
}

void GameManager::openPath(const QString &path) const
{
    if (path.isEmpty() || !QFileInfo::exists(path))
        return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}
