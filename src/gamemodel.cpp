#include "gamemodel.h"

GameModel::GameModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int GameModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_games.size());
}

QVariant GameModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_games.size())
        return {};
    const WineGame &g = m_games.at(index.row());
    switch (role) {
    case IdRole:          return g.id;
    case NameRole:        return g.name;
    case ExePathRole:     return g.exePath;
    case ArgsRole:        return g.args;
    case PrefixPathRole:  return g.prefixPath;
    case WineBinaryRole:  return g.wineBinary;
    case ProtonDirRole:   return g.protonDir;
    case IconPathRole:    return g.iconPath;
    case MangohudRole:    return g.mangohud;
    case FullscreenRole:  return g.fullscreen;
    case GamepadRole:     return g.gamepad;
    case ZinkRole:        return g.zink;
    case WaylandRole:     return g.wayland;
    case TouchMouseRole:  return g.touchMouse;
    case FsrRole:         return g.fsr;
    case MaxHeightRole:   return g.maxHeight;
    case LastPlayedRole:  return g.lastPlayed;
    case RunningRole:     return m_runningIds.contains(g.id);
    }
    return {};
}

QHash<int, QByteArray> GameModel::roleNames() const
{
    return {
        {IdRole,         "id"},
        {NameRole,       "name"},
        {ExePathRole,    "exe"},
        {ArgsRole,       "args"},
        {PrefixPathRole, "prefix"},
        {WineBinaryRole, "wine"},
        {ProtonDirRole,  "proton"},
        {IconPathRole,   "icon"},
        {MangohudRole,   "mangohud"},
        {FullscreenRole, "fullscreen"},
        {GamepadRole,    "gamepad"},
        {ZinkRole,       "zink"},
        {WaylandRole,    "wayland"},
        {TouchMouseRole, "touchMouse"},
        {FsrRole,        "fsr"},
        {MaxHeightRole,  "maxHeight"},
        {LastPlayedRole, "lastPlayed"},
        {RunningRole,    "running"}
    };
}

QVariantMap GameModel::get(int row) const
{
    if (row < 0 || row >= m_games.size())
        return {};
    const WineGame &g = m_games.at(row);
    return {
        {"index",      row},
        {"id",         g.id},
        {"name",       g.name},
        {"exe",        g.exePath},
        {"args",       g.args},
        {"prefix",     g.prefixPath},
        {"wine",       g.wineBinary},
        {"proton",     g.protonDir},
        {"icon",       g.iconPath},
        {"mangohud",   g.mangohud},
        {"fullscreen", g.fullscreen},
        {"gamepad", g.gamepad},
        {"zink", g.zink},
        {"wayland", g.wayland},
        {"touchMouse", g.touchMouse},
        {"fsr", g.fsr},
        {"maxHeight", g.maxHeight},
        {"lastPlayed", g.lastPlayed},
        {"running",    m_runningIds.contains(g.id)}
    };
}

void GameModel::addGame(const WineGame &game)
{
    beginInsertRows(QModelIndex(), m_games.size(), m_games.size());
    m_games.append(game);
    endInsertRows();
}

void GameModel::updateGame(int row, const WineGame &game)
{
    if (row < 0 || row >= m_games.size())
        return;
    m_games[row] = game;
    const QModelIndex idx = index(row);
    Q_EMIT dataChanged(idx, idx);
}

void GameModel::removeGame(int row)
{
    if (row < 0 || row >= m_games.size())
        return;
    beginRemoveRows(QModelIndex(), row, row);
    m_games.removeAt(row);
    endRemoveRows();
}

void GameModel::setRunning(const QString &id, bool running)
{
    int row = -1;
    for (int i = 0; i < m_games.size(); ++i) {
        if (m_games.at(i).id == id) {
            row = i;
            break;
        }
    }
    if (row < 0)
        return;
    if (running)
        m_runningIds.insert(id);
    else
        m_runningIds.remove(id);
    const QModelIndex idx = index(row);
    Q_EMIT dataChanged(idx, idx, {RunningRole});
}

const WineGame &GameModel::gameAt(int row) const
{
    static const WineGame empty;
    if (row < 0 || row >= m_games.size())
        return empty;
    return m_games.at(row);
}
