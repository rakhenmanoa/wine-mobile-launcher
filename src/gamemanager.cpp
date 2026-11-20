#include "gamemanager.h"

#include <cstdio>

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSaveFile>
#include <QScreen>
#include <QStandardPaths>
#include <QStringList>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTimer>
#include <QUuid>

static bool wrapInSway(const QString &program, const QStringList &args,
                       const QString &swayBinary, QString &outProgram,
                       QStringList &outArgs, QString &error);

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
    m_viewMode = m_settings.value(QStringLiteral("libraryViewMode"),
                                  QStringLiteral("list")).toString();
    if (m_viewMode != QLatin1String("list") && m_viewMode != QLatin1String("medium")
        && m_viewMode != QLatin1String("large"))
        m_viewMode = QStringLiteral("list");
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

// Mode d'affichage de la bibliothèque (« list », « medium », « large »),
// mémorisé dans les paramètres.
void GameManager::setViewMode(const QString &mode)
{
    const QString normalized = mode.trimmed();
    if (normalized != QLatin1String("list") && normalized != QLatin1String("medium")
        && normalized != QLatin1String("large"))
        return;
    if (m_viewMode == normalized)
        return;
    m_viewMode = normalized;
    m_settings.setValue(QStringLiteral("libraryViewMode"), normalized);
    Q_EMIT viewModeChanged();
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
    bool migrated = false;
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
        game.fullscreen = obj.value(QLatin1String("fullscreen")).toBool();
        game.lastPlayed = static_cast<qint64>(obj.value(QLatin1String("lastPlayed")).toDouble());
        // Migration du cache v2 : les icônes automatiques extraites avant le
        // passage à la grande variante sont re-extraites au démarrage (une
        // seule fois, puis la bibliothèque est réenregistrée).
        if (isAutoIconPath(game.iconPath)
            && !game.iconPath.startsWith(iconCacheDir() + QLatin1Char('/'))) {
            const QString oldPath = game.iconPath;
            QString iconError;
            const QString icon = extractExeIcon(game, &iconError);
            if (!icon.isEmpty()) {
                game.iconPath = icon;
                QFile::remove(oldPath);
                migrated = true;
            }
        }
        m_model->addGame(game);
    }
    if (migrated)
        saveLibrary();
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
        obj.insert(QLatin1String("fullscreen"), game.fullscreen);
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

// Sortie d'un processus : une entrée horodatée par ligne (Wine/Proton envoient
// souvent plusieurs lignes d'un coup).
void GameManager::appendProcessOutput(const QString &gameName, const QString &text)
{
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (!line.isEmpty())
            appendLog(QLatin1Char('[') + gameName + QStringLiteral("] ") + line);
    }
}

void GameManager::clearLog()
{
    m_log.clear();
    Q_EMIT logChanged();
}

// Dossier de cache des icônes extraites. « v2 » : depuis cette version, on
// parcourt toutes les variantes du .ico et on garde la plus grande ; les
// caches « v1 » (…/icons/<id>.png) pouvaient contenir une petite vignette.
QString GameManager::iconCacheDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/icons/v2");
}

// Vrai si le chemin d'icône pointe dans le cache automatique (…/icons/,
// v1 ou v2 — et non vers une image choisie manuellement par l'utilisateur).
bool GameManager::isAutoIconPath(const QString &path) const
{
    return path.startsWith(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                           + QStringLiteral("/icons/"));
}

// Extrait l'icône embarquée d'un exécutable Windows (.exe) avec wrestool
// (paquet icoutils) : les ressources RT_GROUP_ICON sont des .ico, que le
// greffon ICO de Qt sait décoder. Un même .ico contient souvent plusieurs
// variantes (16, 32, 48… 256 px) : on les parcourt toutes et on garde la
// plus grande — sinon le décodeur renvoie une petite vignette par défaut.
QString GameManager::extractExeIcon(const WineGame &game, QString *error)
{
    if (error)
        error->clear();
    const QString exe = game.exePath;
    // wrestool ne traite que les fichiers PE ; les raccourcis .lnk, scripts
    // .bat et installateurs .msi sont ignorés silencieusement.
    if (exe.isEmpty() || !exe.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
        return {};
    const QFileInfo exeInfo(exe);
    if (!exeInfo.isFile()) {
        if (error)
            *error = QStringLiteral("exécutable introuvable");
        return {};
    }
    const QString wrestool = QStandardPaths::findExecutable(QStringLiteral("wrestool"));
    if (wrestool.isEmpty()) {
        if (error)
            *error = QStringLiteral("wrestool introuvable (paquet icoutils)");
        return {};
    }

    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        if (error)
            *error = QStringLiteral("dossier temporaire indisponible");
        return {};
    }
    QProcess proc;
    proc.setProgram(wrestool);
    proc.setArguments({QStringLiteral("-x"), QStringLiteral("--type=14"),
                       QStringLiteral("--output"), tempDir.path(),
                       exeInfo.absoluteFilePath()});
    proc.start();
    if (!proc.waitForStarted(3000) || !proc.waitForFinished(10000)) {
        proc.kill();
        proc.waitForFinished(1000);
        if (error)
            *error = QStringLiteral("wrestool n'a pas pu analyser l'exécutable");
        return {};
    }

    const QFileInfoList entries =
        QDir(tempDir.path()).entryInfoList({QStringLiteral("*.ico*")}, QDir::Files);
    QFileInfo best;
    for (const QFileInfo &entry : entries) {
        if (best.filePath().isEmpty() || entry.size() > best.size())
            best = entry;
    }
    if (!best.isFile()) {
        if (error)
            *error = QStringLiteral("aucune ressource d'icône dans l'exécutable");
        return {};
    }
    QImageReader reader(best.absoluteFilePath());
    QImage image;
    const int frames = reader.imageCount();
    if (frames > 1) {
        QImage largest;
        for (int i = 0; i < frames; ++i) {
            if (!reader.jumpToImage(i))
                break;
            const QImage frame = reader.read();
            if (frame.isNull())
                break;
            if (frame.width() * frame.height()
                > largest.width() * largest.height())
                largest = frame;
        }
        image = largest;
    } else {
        image = reader.read();
    }
    if (image.isNull()) {
        if (error)
            *error = QStringLiteral("icône illisible (") + reader.errorString()
                     + QStringLiteral(")");
        return {};
    }
    if (image.width() > 256 || image.height() > 256)
        image = image.scaled(256, 256, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    const QString cache = iconCacheDir();
    if (!QDir().mkpath(cache)) {
        if (error)
            *error = QStringLiteral("cache d'icônes inaccessible");
        return {};
    }
    const QString pngPath = cache + QLatin1Char('/') + game.id + QStringLiteral(".png");
    if (!image.save(pngPath, "PNG")) {
        if (error)
            *error = QStringLiteral("écriture du cache impossible");
        return {};
    }
    appendLog(QStringLiteral("Icône de « ") + game.name + QStringLiteral(" » : variante la plus grande ")
              + QString::number(image.width()) + QStringLiteral("×")
              + QString::number(image.height()) + QStringLiteral(" px."));
    return pngPath;
}

void GameManager::addGame(const QString &name, const QString &exePath, const QString &args,
                          const QString &prefixPath, const QString &wineBinary,
                          const QString &protonDir, const QString &iconPath, bool mangohud,
                          bool fullscreen)
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
    game.fullscreen = fullscreen;
    // Icône automatique : si l'utilisateur n'a pas choisi d'image, tenter
    // l'extraction depuis l'exécutable (sans effet hors .exe / hors icoutils).
    if (game.iconPath.isEmpty()) {
        QString iconError;
        const QString icon = extractExeIcon(game, &iconError);
        if (!icon.isEmpty()) {
            game.iconPath = icon;
            appendLog(QStringLiteral("Icône extraite de l'exécutable pour « ")
                      + game.name + QStringLiteral(" »."));
        } else if (!iconError.isEmpty()) {
            appendLog(QStringLiteral("Pas d'icône pour « ") + game.name
                      + QStringLiteral(" » (") + iconError + QStringLiteral(")."));
        }
    }
    m_model->addGame(game);
    saveLibrary();
    appendLog(QStringLiteral("Jeu ajouté : ") + game.name);
}

void GameManager::updateGame(int row, const QString &name, const QString &exePath,
                             const QString &args, const QString &prefixPath,
                             const QString &wineBinary, const QString &protonDir,
                             const QString &iconPath, bool mangohud, bool fullscreen)
{
    if (row < 0 || row >= m_model->rowCount())
        return;
    WineGame game = m_model->gameAt(row);
    const QString previousExe = game.exePath;
    game.name = name.trimmed();
    game.exePath = exePath.trimmed();
    game.args = args.trimmed();
    game.prefixPath = prefixPath.trimmed();
    game.wineBinary = wineBinary.trimmed();
    game.protonDir = protonDir.trimmed();
    game.iconPath = iconPath.trimmed();
    game.mangohud = mangohud;
    game.fullscreen = fullscreen;
    // Icône automatique : extraction si aucune icône n'est définie, ou
    // re-extraction quand l'exécutable a changé et que l'icône actuelle est
    // celle du cache automatique — une image choisie manuellement reste.
    if (game.iconPath.isEmpty()
        || (previousExe != game.exePath && isAutoIconPath(game.iconPath))) {
        QString iconError;
        const QString icon = extractExeIcon(game, &iconError);
        if (!icon.isEmpty()) {
            game.iconPath = icon;
            appendLog(QStringLiteral("Icône extraite de l'exécutable pour « ")
                      + game.name + QStringLiteral(" »."));
        } else if (game.iconPath.isEmpty() && !iconError.isEmpty()) {
            appendLog(QStringLiteral("Pas d'icône pour « ") + game.name
                      + QStringLiteral(" » (") + iconError + QStringLiteral(")."));
        }
    }
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
    // L'icône mise en cache automatiquement ne doit pas survivre au jeu.
    if (isAutoIconPath(game.iconPath))
        QFile::remove(game.iconPath);
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

    // Plein écran (paysage) : seul le jeu est pivoté, pas l'écran du système.
    // Le jeu tourne dans un sway imbriqué (wlroots, backend Wayland) dont la
    // sortie est tournée de 90° et dont les fenêtres sont en plein écran ;
    // Wine passe par l'Xwayland de sway. La fenêtre de sway est elle-même
    // passée en plein écran par une règle de fenêtre KWin. Sans sway, repli
    // sur un bureau virtuel Wine paysage, sans rotation de l'image.
    QString swayBinary;
    if (game.fullscreen) {
        swayBinary = QStandardPaths::findExecutable(QStringLiteral("sway"));
        if (!swayBinary.isEmpty()) {
            appendLog(QStringLiteral("Plein écran (paysage) : jeu lancé dans sway imbriqué, sortie pivotée de 90°."));
        } else {
            const QScreen *screen = QGuiApplication::primaryScreen();
            // Pixels PHYSIQUES : Qt rapporte la géométrie logique.
            const qreal dpr = screen ? screen->devicePixelRatio() : 1.0;
            const QRect geom = screen ? screen->geometry() : QRect();
            QSize landscape(1280, 720);
            if (!geom.isEmpty())
                landscape = QSize(qRound(qMax(geom.width(), geom.height()) * dpr),
                                  qRound(qMin(geom.width(), geom.height()) * dpr));
            payload.prepend(QStringLiteral("/desktop=wml,%1x%2")
                                .arg(landscape.width()).arg(landscape.height()));
            payload.prepend(QStringLiteral("explorer"));
            appendLog(QStringLiteral("Plein écran (paysage) : sway introuvable (installez le paquet « sway ») — ")
                      + QStringLiteral("bureau virtuel Wine %1×%2 sans rotation d'image.")
                            .arg(landscape.width()).arg(landscape.height()));
        }
    }

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
    bool swayWrapped = false;
    if (!swayBinary.isEmpty()) {
        QString swayError;
        QString swayProgram;
        QStringList swayArguments;
        if (wrapInSway(proc->program(), arguments, swayBinary, swayProgram, swayArguments, swayError)) {
            proc->setProgram(swayProgram);
            arguments = swayArguments;
            swayWrapped = true;
        } else {
            appendLog(QStringLiteral("Plein écran (paysage) : ") + swayError
                      + QStringLiteral(" — lancement sans rotation."));
        }
    }
    proc->setArguments(arguments);
    proc->setWorkingDirectory(exeInfo.absolutePath());

    connect(proc, &QProcess::readyReadStandardOutput, this, [this, game, proc]() {
        appendProcessOutput(game.name, QString::fromLocal8Bit(proc->readAllStandardOutput()));
    });
    connect(proc, &QProcess::readyReadStandardError, this, [this, game, proc]() {
        appendProcessOutput(game.name, QString::fromLocal8Bit(proc->readAllStandardError()));
    });
    connect(proc, &QProcess::errorOccurred, this,
            [this, game](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            appendLog(QStringLiteral("Impossible de démarrer « ") + game.name
                      + QStringLiteral(" » : binaire Wine/Proton invalide ?"));
            m_processes.remove(game.id);
            m_model->setRunning(game.id, false);
        }
    });
    connect(proc, &QProcess::finished, this,
            [this, game, swayWrapped](int exitCode, QProcess::ExitStatus status) {
        m_processes.remove(game.id);
        m_model->setRunning(game.id, false);
        // sway fermé (fenêtre fermée, crash) : Wine ne meurt pas avec lui,
        // on arrête donc explicitement le jeu (wineserver -k).
        if (swayWrapped)
            stopForGame(game);
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
    // Plein écran : la fenêtre démarre en mode fenêtré (un plein écran forcé à
    // la création laisse la sortie sway noire). On neutralise d'abord les
    // règles KWin, puis on les active une fois le jeu affiché.
    if (game.fullscreen) {
        ensureFullscreenWindowRules(false);
        const QString marker = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                               + QStringLiteral("/wml-game-ready");
        QFile::remove(marker);
        auto *timer = new QTimer(proc);
        timer->setInterval(500);
        int ticks = 0;
        connect(timer, &QTimer::timeout, this, [this, timer, marker, swayWrapped, ticks]() mutable {
            ++ticks;
            // sway : le script crée le marqueur quand une fenêtre du jeu existe ;
            // sans sway : délai fixe de 8 s.
            const bool ready = swayWrapped ? QFile::exists(marker) : ticks >= 16;
            if (ready || ticks >= 180) {
                timer->stop();
                QFile::remove(marker);
                if (ready)
                    applyFullscreenScript();
            }
        });
        timer->start();
    }
    proc->start();
    Q_EMIT gameLaunched(row, true, {});
}

// Enveloppe la commande du jeu dans un sway imbriqué dont la sortie est
// tournée (WML_ROTATION = right (90°, défaut) | left (270°) | inverted (180°)).
// Écrit dans le dossier de données de l'appli un script qui lance le jeu puis
// demande à sway de quitter (swaymsg exit), et une config sway minimale :
// Xwayland, sans bordures, toute fenêtre en plein écran. sway se connecte à
// la session Wayland courante (backend Wayland de wlroots).
static bool wrapInSway(const QString &program, const QStringList &args,
                       const QString &swayBinary, QString &outProgram,
                       QStringList &outArgs, QString &error)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dir.isEmpty() || !QDir().mkpath(dir)) {
        error = QStringLiteral("dossier de données inaccessible");
        return false;
    }
    const auto quote = [](const QString &s) {
        QString q = s;
        q.replace(QStringLiteral("'"), QStringLiteral("'\\''"));
        return QStringLiteral("'") + q + QStringLiteral("'");
    };

    const QString scriptPath = dir + QStringLiteral("/wml-sway-run.sh");
    QFile script(scriptPath);
    if (!script.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QStringLiteral("impossible d'écrire ") + scriptPath;
        return false;
    }
    // Le jeu tourne en arrière-plan ; le script surveille les sorties de sway
    // (la fenêtre imbriquée fermée = plus de sortie) et arrête alors le jeu,
    // puis quitte sway. Fin normale du jeu : sway quitte aussi.
    // Avant de lancer le jeu, attendre que la fenêtre sway ait sa taille
    // définitive : KWin la passe en plein écran (règle) après sa création, et
    // un jeu démarré pendant ce redimensionnement reste derrière un écran noir.
    QString body = QStringLiteral(
        "#!/bin/sh\n"
        "prev=''; stable=0; t=0\n"
        "while [ \"$t\" -lt 40 ]; do\n"
        "    cur=$(swaymsg -t get_outputs 2>/dev/null | grep -m1 'Current mode')\n"
        "    if [ -n \"$cur\" ] && [ \"$cur\" = \"$prev\" ]; then stable=$((stable+1)); else stable=0; fi\n"
        "    prev=\"$cur\"\n"
        "    [ \"$stable\" -ge 4 ] && break\n"
        "    t=$((t+1)); sleep 0.5\n"
        "done\n")
        + quote(program);
    for (const QString &a : args)
        body += QStringLiteral(" ") + quote(a);
    body += QStringLiteral(
        " &\n"
        "pid=$!\n"
        "seen=0; zero=0; waited=0; ready=0\n"
        "while kill -0 \"$pid\" 2>/dev/null; do\n"
        "    if [ \"$ready\" = 0 ] && swaymsg -t get_tree 2>/dev/null | grep -q '\"shell\": \"xwayland\"'; then\n"
        "        ready=1; sleep 1; touch %1\n"
        "    fi\n"
        "    n=$(swaymsg -t get_outputs 2>/dev/null | grep -c '\"name\"')\n"
        "    if [ \"$n\" -gt 0 ] 2>/dev/null; then seen=1; zero=0; else zero=$((zero+1)); fi\n"
        "    waited=$((waited+1))\n"
        "    if [ \"$seen\" = 1 ] && [ \"$zero\" -ge 2 ]; then kill \"$pid\" 2>/dev/null; break; fi\n"
        "    if [ \"$seen\" = 0 ] && [ \"$waited\" -ge 20 ]; then kill \"$pid\" 2>/dev/null; break; fi\n"
        "    sleep 1\n"
        "done\n"
        "swaymsg exit >/dev/null 2>&1\n")
                .arg(quote(dir + QStringLiteral("/wml-game-ready")));
    script.write(body.toUtf8());
    script.close();
    script.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                          | QFileDevice::ExeOwner);

    const QString direction = qEnvironmentVariable("WML_ROTATION", QStringLiteral("right"));
    const QString transform = direction == QLatin1String("left") ? QStringLiteral("270")
                            : direction == QLatin1String("inverted") ? QStringLiteral("180")
                            : QStringLiteral("90");
    const QString confPath = dir + QStringLiteral("/wml-sway.conf");
    QFile conf(confPath);
    if (!conf.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        error = QStringLiteral("impossible d'écrire ") + confPath;
        return false;
    }
    conf.write(QStringLiteral("xwayland enable\n"
                              "default_border none\n"
                              "output * transform %1\n"
                              "for_window [shell=\".*\"] fullscreen enable\n"
                              "exec %2\n").arg(transform, scriptPath).toUtf8());
    conf.close();

    outProgram = swayBinary;
    outArgs = {QStringLiteral("-c"), confPath};
    return true;
}

// Plein écran : charge dans KWin (D-Bus, org.kde.kwin.Scripting) un petit
// script qui met en plein écran la fenêtre sway (« wlroots ») ou le bureau
// virtuel Wine (« wml »), puis le décharge. Pas de règle persistante.
void GameManager::applyFullscreenScript()
{
    QString qdbus = QStandardPaths::findExecutable(QStringLiteral("qdbus6"));
    if (qdbus.isEmpty())
        qdbus = QStandardPaths::findExecutable(QStringLiteral("qdbus-qt6"));
    if (qdbus.isEmpty())
        qdbus = QStandardPaths::findExecutable(QStringLiteral("qdbus"));
    if (qdbus.isEmpty()) {
        appendLog(QStringLiteral("Plein écran : qdbus6 introuvable (paquet qt6-tools)."));
        return;
    }
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    const QString path = dir + QStringLiteral("/wml-fullscreen.js");
    QFile js(path);
    if (!js.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        appendLog(QStringLiteral("Plein écran : impossible d'écrire ") + path);
        return;
    }
    js.write("var list = (typeof workspace.windowList === 'function') ? workspace.windowList() : workspace.clientList();\n"
             "for (var i = 0; i < list.length; ++i) {\n"
             "    var w = list[i];\n"
             "    var cls = String(w.resourceClass || w.resourceName || '');\n"
             "    var cap = String(w.caption || '');\n"
             "    if (cls.indexOf('wlroots') >= 0 || cap.indexOf('wlroots') >= 0 || cap.indexOf('wml - Wine') === 0)\n"
             "        w.fullScreen = true;\n"
             "}\n");
    js.close();

    const auto call = [&](const QStringList &args) {
        QProcess p;
        p.start(qdbus, args);
        if (!p.waitForStarted(3000) || !p.waitForFinished(5000))
            return QString();
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    };
    const QString name = QStringLiteral("wml-fullscreen");
    call({QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
          QStringLiteral("org.kde.kwin.Scripting.unloadScript"), name});
    const QString id = call({QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
                             QStringLiteral("org.kde.kwin.Scripting.loadScript"), path, name});
    if (id.isEmpty() || id == QLatin1String("-1")) {
        appendLog(QStringLiteral("Plein écran : KWin a refusé le script."));
        return;
    }
    call({QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting/Script") + id,
          QStringLiteral("org.kde.kwin.Script.run")});
    call({QStringLiteral("org.kde.KWin"), QStringLiteral("/Scripting"),
          QStringLiteral("org.kde.kwin.Scripting.unloadScript"), name});
    appendLog(QStringLiteral("Plein écran : demandé à KWin (script)."));
}

// Plein écran : installe (idempotent) des règles de fenêtre KWin qui forcent
// le plein écran de la fenêtre imbriquée de sway (classe « wlroots ») et du
// bureau virtuel Wine de repli (titre « wml »). KWin applique lui-même la règle dès que
// la fenêtre apparaît : ni script, ni minuteur, ni pid. L'orientation du
// système n'est jamais modifiée.
void GameManager::ensureFullscreenWindowRules(bool enable)
{
    QString writer = QStandardPaths::findExecutable(QStringLiteral("kwriteconfig6"));
    QString reader = QStandardPaths::findExecutable(QStringLiteral("kreadconfig6"));
    if (writer.isEmpty() || reader.isEmpty()) {
        writer = QStandardPaths::findExecutable(QStringLiteral("kwriteconfig5"));
        reader = QStandardPaths::findExecutable(QStringLiteral("kreadconfig5"));
    }
    if (writer.isEmpty() || reader.isEmpty()) {
        appendLog(QStringLiteral("Plein écran : kwriteconfig introuvable (paquet kconfig) — ")
                  + QStringLiteral("règle KWin non installée, passez la fenêtre en plein écran à la main."));
        return;
    }
    const auto run = [](const QString &program, const QStringList &args) {
        QProcess p;
        p.start(program, args);
        if (!p.waitForStarted(3000) || !p.waitForFinished(5000))
            return QString();
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    };
    const auto write = [&](const QString &group, const QString &key, const QString &value) {
        run(writer, {QStringLiteral("--file"), QStringLiteral("kwinrulesrc"),
                     QStringLiteral("--group"), group, QStringLiteral("--key"), key, value});
    };

    struct Rule { QString id, description, key, value; };
    const QList<Rule> rules = {
        // Fenêtre imbriquée de sway : classe « wlroots » (backend Wayland).
        {QStringLiteral("wml-fullscreen-sway"),
         QStringLiteral("Wine Mobile Launcher : plein écran (sway)"),
         QStringLiteral("wmclass"), QStringLiteral("wlroots")},
        // Même fenêtre de sway, par son titre (« wlroots - WL-1 ») au cas où la
        // classe ne serait pas celle attendue.
        {QStringLiteral("wml-fullscreen-sway-title"),
         QStringLiteral("Wine Mobile Launcher : plein écran (sway, titre)"),
         QStringLiteral("title"), QStringLiteral("wlroots")},
        // Bureau virtuel Wine (repli sans sway) : titre « wml - Wine desktop ».
        {QStringLiteral("wml-fullscreen-desktop"),
         QStringLiteral("Wine Mobile Launcher : plein écran (bureau virtuel Wine)"),
         QStringLiteral("title"), QStringLiteral("wml")},
    };

    // KWin récent liste ses règles dans « Order » (clé écrite par les
    // Paramètres système) ; « rules » et « count » sont les anciennes clés.
    // Les règles qui ne figurent pas dans « Order » sont ignorées : on tient
    // donc les trois à jour, en conservant les règles existantes de l'utilisateur.
    const auto readList = [&](const QString &key) {
        return run(reader, {QStringLiteral("--file"), QStringLiteral("kwinrulesrc"),
                            QStringLiteral("--group"), QStringLiteral("General"),
                            QStringLiteral("--key"), key})
            .split(QLatin1Char(','), Qt::SkipEmptyParts);
    };
    const QStringList orderIds = readList(QStringLiteral("Order"));
    QStringList ids = orderIds;
    for (const QString &old : readList(QStringLiteral("rules"))) {
        if (!ids.contains(old))
            ids << old;
    }
    for (const Rule &r : rules) {
        if (!ids.contains(r.id))
            ids << r.id;
        write(r.id, QStringLiteral("Description"), r.description);
        write(r.id, r.key, r.value);
        write(r.id, r.key + QStringLiteral("match"), QStringLiteral("2")); // sous-chaîne
        if (r.key == QLatin1String("wmclass"))
            write(r.id, QStringLiteral("wmclasscomplete"), QStringLiteral("false"));
        write(r.id, QStringLiteral("fullscreen"), QStringLiteral("true"));
        write(r.id, QStringLiteral("fullscreenrule"),
              enable ? QStringLiteral("2") : QStringLiteral("1")); // forcer / ne pas toucher
    }
    const QString joined = ids.join(QLatin1Char(','));
    write(QStringLiteral("General"), QStringLiteral("Order"), joined);
    write(QStringLiteral("General"), QStringLiteral("rules"), joined);
    write(QStringLiteral("General"), QStringLiteral("count"), QString::number(ids.size()));

    // KWin relit ses règles.
    const QString dbusSend = QStandardPaths::findExecutable(QStringLiteral("dbus-send"));
    if (dbusSend.isEmpty()) {
        appendLog(QStringLiteral("Plein écran : dbus-send introuvable — règle KWin écrite, ")
                  + QStringLiteral("mais KWin ne l'a pas rechargée (déconnexion/reconnexion nécessaire)."));
        return;
    }
    run(dbusSend, {QStringLiteral("--session"), QStringLiteral("--dest=org.kde.KWin"),
                   QStringLiteral("/KWin"), QStringLiteral("org.kde.KWin.reconfigure")});
    if (enable)
        appendLog(QStringLiteral("Plein écran : règle de fenêtre KWin activée (jeu affiché)."));
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

// Action explicite de l'interface : re-extraction forcée de l'icône depuis
// l'exécutable (remplace une image choisie manuellement ou un cache périmé).
void GameManager::refreshIcon(int row)
{
    if (row < 0 || row >= m_model->rowCount())
        return;
    WineGame game = m_model->gameAt(row);
    QString iconError;
    const QString icon = extractExeIcon(game, &iconError);
    if (!icon.isEmpty()) {
        game.iconPath = icon;
        m_model->updateGame(row, game);
        saveLibrary();
        appendLog(QStringLiteral("Icône mise à jour depuis l'exécutable pour « ")
                  + game.name + QStringLiteral(" »."));
    } else {
        appendLog(QStringLiteral("Aucune icône extraite pour « ") + game.name
                  + QStringLiteral(" »")
                  + (iconError.isEmpty() ? QStringLiteral(".")
                                         : QStringLiteral(" (") + iconError
                                               + QStringLiteral(").")));
    }
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
