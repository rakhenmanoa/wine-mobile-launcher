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
#include <QSysInfo>
#include <QUuid>

GameManager::GameManager(QObject *parent)
    : QObject(parent)
{
    m_model = new GameModel(this);
    m_defaultWineBinary = m_settings.value(QStringLiteral("defaultWineBinary")).toString();
    m_defaultProtonDir = m_settings.value(QStringLiteral("defaultProtonDir")).toString();
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

void GameManager::setDefaultProtonDir(const QString &dir)
{
    if (m_defaultProtonDir == dir)
        return;
    m_defaultProtonDir = dir;
    m_settings.setValue(QStringLiteral("defaultProtonDir"), dir);
    Q_EMIT defaultProtonDirChanged();
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
    if (verbose)
        appendLog(QStringLiteral("Journal verbose activé — enregistrement dans ") + logFilePath());
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
        game.protonDir = obj.value(QLatin1String("proton")).toString();
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
        obj.insert(QLatin1String("proton"), game.protonDir);
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

namespace {

// Racine d'un dossier GE-Proton/Proton si `info` en désigne un (le dossier
// lui-même, ou son script « proton »), sinon chaîne vide.
QString protonRootFor(const QFileInfo &info)
{
    if (info.isDir()) {
        const QString dir = QDir::cleanPath(info.absoluteFilePath());
        if (QFileInfo(dir + QStringLiteral("/proton")).isFile())
            return dir;
    } else if (info.isFile() && info.fileName() == QLatin1String("proton")) {
        return QDir::cleanPath(info.absolutePath());
    }
    return {};
}

// Dossier bin de Proton : « files/bin » (GE-Proton, Proton récent) ou
// « dist/bin » (anciennes versions).
QString protonBinDir(const QString &root)
{
    for (const char *sub : {"/files/bin", "/dist/bin"}) {
        const QString dir = root + QLatin1String(sub);
        if (QFileInfo(dir).isDir())
            return dir;
    }
    return {};
}

// Architecture (e_machine) d'un binaire ELF, ou -1 si illisible.
int elfMachine(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return -1;
    const QByteArray head = file.read(20);
    if (head.size() < 20 || !head.startsWith("\x7f" "ELF"))
        return -1;
    return static_cast<unsigned char>(head[18])
           | (static_cast<unsigned char>(head[19]) << 8);
}

} // namespace

// Ordre de priorité : GE-Proton du jeu, Wine du jeu, GE-Proton par défaut,
// Wine par défaut, puis « wine » du système.
GameManager::Runner GameManager::resolveRunner(const QString &overrideBinary,
                                               const QString &overrideProton) const
{
    QString firstError;
    const QStringList candidates = {overrideProton, overrideBinary, m_defaultProtonDir,
                                    m_defaultWineBinary, QStringLiteral("wine")};
    for (QString candidate : candidates) {
        candidate = candidate.trimmed();
        if (candidate.isEmpty())
            continue;
        if (candidate == QLatin1String("~") || candidate.startsWith(QLatin1String("~/")))
            candidate = QDir::homePath() + candidate.mid(1);

        QString path;
        if (QFileInfo(candidate).isAbsolute()) {
            if (!QFileInfo::exists(candidate))
                continue;
            path = candidate;
        } else {
            path = QStandardPaths::findExecutable(candidate);
            if (path.isEmpty())
                continue;
        }
        const QFileInfo info(path);

        // GE-Proton / Proton : dossier racine ou script « proton ».
        const QString root = protonRootFor(info);
        if (!root.isEmpty()) {
            if (protonBinDir(root).isEmpty()) {
                if (firstError.isEmpty())
                    firstError = QStringLiteral("Dossier Proton invalide (files/bin introuvable) : ")
                                 + root;
                continue;
            }
            Runner runner;
            runner.kind = Runner::Kind::Proton;
            runner.root = root;
            runner.program = root + QStringLiteral("/proton");
            return runner;
        }

        if (info.isFile()) {
            Runner runner;
            runner.kind = Runner::Kind::Wine;
            runner.program = path;
            return runner;
        }
        if (firstError.isEmpty())
            firstError = QStringLiteral("Ni binaire Wine ni dossier Proton : ") + candidate;
    }
    Runner none;
    none.error = firstError;
    return none;
}

QString GameManager::runnerWineserver(const Runner &runner) const
{
    if (runner.kind == Runner::Kind::Proton) {
        const QString candidate = protonBinDir(runner.root) + QStringLiteral("/wineserver");
        return QFileInfo::exists(candidate) ? candidate : QString();
    }
    if (runner.kind == Runner::Kind::Wine) {
        // Le wineserver voisin du binaire doit correspondre à sa version
        // (Wine-GE, etc.) ; le wineserver système n'est qu'un repli.
        const QString sibling = QFileInfo(runner.program).absolutePath()
                                + QStringLiteral("/wineserver");
        if (QFileInfo::exists(sibling))
            return sibling;
    }
    return QStandardPaths::findExecutable(QStringLiteral("wineserver"));
}

// Préfixe Wine réel : avec Proton, le dossier choisi est STEAM_COMPAT_DATA_PATH
// et Proton place le préfixe dans « pfx/ ».
QString GameManager::effectiveWinePrefix(const Runner &runner, const QString &prefix) const
{
    return runner.kind == Runner::Kind::Proton ? prefix + QStringLiteral("/pfx") : prefix;
}

QProcessEnvironment GameManager::runnerEnvironment(const Runner &runner,
                                                   const QString &prefix) const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // Mode verbose : laisser Wine afficher ses canaux err/fixme (diagnostic
    // complet dans le journal et le terminal) ; sinon, sortie silencieuse.
    if (!m_verboseTerminal)
        env.insert(QStringLiteral("WINEDEBUG"), QStringLiteral("-all"));

    if (runner.kind != Runner::Kind::Proton) {
        env.insert(QStringLiteral("WINEPREFIX"), prefix);
        return env;
    }

    env.insert(QStringLiteral("STEAM_COMPAT_DATA_PATH"), prefix);
    if (!env.contains(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"))) {
        // Proton exige cette variable ; sans Steam, un dossier existant suffit.
        QString steam;
        for (const QString &c : {QDir::homePath() + QStringLiteral("/.steam/steam"),
                                 QDir::homePath() + QStringLiteral("/.local/share/Steam")}) {
            if (QFileInfo(c).isDir()) {
                steam = c;
                break;
            }
        }
        env.insert(QStringLiteral("STEAM_COMPAT_CLIENT_INSTALL_PATH"),
                   steam.isEmpty() ? runner.root : steam);
    }
    // Hors Steam : pas d'identifiant de jeu, et on évite les correctifs
    // protonfixes (réseau / base umu) qui n'ont pas de sens ici.
    if (!env.contains(QStringLiteral("SteamGameId")))
        env.insert(QStringLiteral("SteamGameId"), QStringLiteral("0"));
    if (!env.contains(QStringLiteral("SteamAppId")))
        env.insert(QStringLiteral("SteamAppId"), QStringLiteral("0"));
    if (!env.contains(QStringLiteral("PROTONFIXES_DISABLE")))
        env.insert(QStringLiteral("PROTONFIXES_DISABLE"), QStringLiteral("1"));
    return env;
}

// Construit la commande : renvoie le programme à lancer et remplit `arguments`
// (Wine : <payload> ; Proton : python3 proton run <payload>).
QString GameManager::runnerCommand(const Runner &runner, const QStringList &payload,
                                   QStringList &arguments) const
{
    arguments.clear();
    if (runner.kind == Runner::Kind::Proton) {
        const QString python = QStandardPaths::findExecutable(QStringLiteral("python3"));
        arguments << runner.program << QStringLiteral("run") << payload;
        // Sans python3 dans le PATH, on tente le script directement (shebang).
        if (python.isEmpty()) {
            const QString script = arguments.takeFirst();
            return script;
        }
        return python;
    }
    arguments = payload;
    return runner.program;
}

// Signale un runner x86_64 sur un hôte aarch64 (nécessite alors box64/FEX).
void GameManager::logArchitectureHint(const Runner &runner)
{
    if (runner.kind == Runner::Kind::None
        || QSysInfo::currentCpuArchitecture() != QLatin1String("arm64"))
        return;
    QString probe = runner.program;
    if (runner.kind == Runner::Kind::Proton) {
        const QString bin = protonBinDir(runner.root);
        probe = QFileInfo::exists(bin + QStringLiteral("/wine64"))
                    ? bin + QStringLiteral("/wine64")
                    : bin + QStringLiteral("/wine");
    }
    const int machine = elfMachine(probe);
    if (machine == 62) // EM_X86_64
        appendLog(QStringLiteral("Attention : ") + probe
                  + QStringLiteral(" est un binaire x86_64 sur un hôte aarch64 — il faut "
                                   "box64/FEX, ou une build GE-Proton aarch64."));
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
        writeLogFile(line);
    }
}

QString GameManager::logFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/wine-mobile-launcher.log");
}

// Mode verbose : le journal est aussi ajouté, avec date complète, à
// « wine-mobile-launcher.log » (dossier de données de l'appli). Rotation simple :
// au-delà de 5 Mo, l'ancien fichier devient « .log.1 ».
void GameManager::writeLogFile(const QString &line)
{
    const QString path = logFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.exists() && file.size() > 5 * 1024 * 1024) {
        QFile::remove(path + QStringLiteral(".1"));
        file.rename(path + QStringLiteral(".1"));
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    const QString stamped = QDateTime::currentDateTime().toString(Qt::ISODate)
                            + QStringLiteral("  ") + line + QStringLiteral("\n");
    file.write(stamped.toUtf8());
}

void GameManager::clearLog()
{
    m_log.clear();
    Q_EMIT logChanged();
}

void GameManager::addGame(const QString &name, const QString &exePath, const QString &args,
                          const QString &prefixPath, const QString &wineBinary,
                          const QString &protonDir, const QString &iconPath, bool mangohud)
{
    WineGame game;
    game.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    game.name = name.trimmed();
    game.exePath = exePath.trimmed();
    game.args = args.trimmed();
    game.prefixPath = prefixPath.trimmed();
    game.wineBinary = wineBinary.trimmed();
    game.protonDir = protonDir.trimmed();
    game.iconPath = iconPath.trimmed();
    game.mangohud = mangohud;
    m_model->addGame(game);
    saveLibrary();
    appendLog(QStringLiteral("Jeu ajouté : ") + game.name);
}

void GameManager::updateGame(int row, const QString &name, const QString &exePath,
                             const QString &args, const QString &prefixPath,
                             const QString &wineBinary, const QString &protonDir,
                             const QString &iconPath, bool mangohud)
{
    if (row < 0 || row >= m_model->rowCount())
        return;
    WineGame game = m_model->gameAt(row);
    game.name = name.trimmed();
    game.exePath = exePath.trimmed();
    game.args = args.trimmed();
    game.prefixPath = prefixPath.trimmed();
    game.wineBinary = wineBinary.trimmed();
    game.protonDir = protonDir.trimmed();
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

    const Runner runner = resolveRunner(game.wineBinary, game.protonDir);
    if (runner.kind == Runner::Kind::None) {
        appendLog(runner.error.isEmpty()
                      ? QStringLiteral("Wine introuvable : installez le paquet wine ou définissez ")
                            + QStringLiteral("un binaire Wine ou un dossier GE-Proton dans les paramètres.")
                      : runner.error);
        Q_EMIT gameLaunched(row, false, QStringLiteral("Wine / Proton introuvable."));
        return;
    }
    logArchitectureHint(runner);

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
    proc->setProcessEnvironment(runnerEnvironment(runner, prefix));

    QStringList payload{game.exePath};
    const QStringList extra = game.args.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (!extra.isEmpty())
        payload += extra;
    QStringList arguments;
    const QString wine = runnerCommand(runner, payload, arguments);

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
                      + QStringLiteral(" » : binaire Wine/Proton invalide ?"));
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
    const Runner runner = resolveRunner(game.wineBinary, game.protonDir);
    const QString server = runnerWineserver(runner);
    if (server.isEmpty()) {
        appendLog(QStringLiteral("wineserver introuvable, impossible d'arrêter « ")
                  + game.name + QStringLiteral(" »."));
        return;
    }

    const QString prefix = game.prefixPath.isEmpty() ? defaultPrefixFor(game.name)
                                                     : game.prefixPath;
    auto *proc = new QProcess(this);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("WINEPREFIX"), effectiveWinePrefix(runner, prefix));
    if (runner.kind == Runner::Kind::Proton) {
        // Le wineserver de Proton charge ses bibliothèques depuis son dossier.
        QStringList libs;
        for (const char *sub : {"/files/lib64", "/files/lib", "/dist/lib64", "/dist/lib"}) {
            const QString dir = runner.root + QLatin1String(sub);
            if (QFileInfo(dir).isDir())
                libs << dir;
        }
        const QString old = env.value(QStringLiteral("LD_LIBRARY_PATH"));
        if (!old.isEmpty())
            libs << old;
        if (!libs.isEmpty())
            env.insert(QStringLiteral("LD_LIBRARY_PATH"), libs.join(QLatin1Char(':')));
    }
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
    const Runner runner = resolveRunner({});
    if (runner.kind == Runner::Kind::None) {
        Q_EMIT prefixCreationFinished(path, false,
                                      runner.error.isEmpty()
                                          ? QStringLiteral("Wine / Proton introuvable.")
                                          : runner.error);
        return;
    }
    logArchitectureHint(runner);
    if (!QDir().mkpath(path)) {
        Q_EMIT prefixCreationFinished(path, false,
                                      QStringLiteral("Impossible de créer le dossier : ") + path);
        return;
    }

    auto *proc = new QProcess(this);
    proc->setProcessEnvironment(runnerEnvironment(runner, path));
    QStringList bootArgs;
    proc->setProgram(runnerCommand(
        runner, {QStringLiteral("wineboot"), QStringLiteral("-i")}, bootArgs));
    proc->setArguments(bootArgs);
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

QString GameManager::detectWine(const QString &overrideBinary,
                                const QString &overrideProton) const
{
    const Runner runner = resolveRunner(overrideBinary, overrideProton);
    switch (runner.kind) {
    case Runner::Kind::Wine:
        return runner.program;
    case Runner::Kind::Proton:
        return runner.root + QStringLiteral(" (Proton)");
    case Runner::Kind::None:
        break;
    }
    return {};
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
