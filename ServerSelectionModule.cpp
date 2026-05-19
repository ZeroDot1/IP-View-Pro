// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — ServerSelectionModule.cpp
//  C++26: std::expected, std::from_chars, std::optional, [[nodiscard]], noexcept
//  Speedtest server selection — fetches, parses, filters, and sorts servers.
//  Extracted from SpeedtestTab for modular use across the application.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "ServerSelectionModule.h"

#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>

#include <charconv>
#include <system_error>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Speedtest {

// ═══════════════════════════════════════════════════════════════════════════════
ServerSelectionModule::ServerSelectionModule(QObject *parent)
    : QObject(parent)
{
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════════

std::expected<std::vector<ServerInfo>, QString>
ServerSelectionModule::getAvailableServers(int timeoutMs) noexcept
{
    QString const program = findSpeedtestBinary();
    if (program.isEmpty()) {
        return std::unexpected(
            QStringLiteral("speedtest-cli not found. Install: sudo pacman -S speedtest-cli"));
    }

    emit serverFetchStarted();

    QProcess listProc;
    listProc.setProcessChannelMode(QProcess::MergedChannels);
    listProc.start(program, QStringList{QStringLiteral("--list")});

    if (!listProc.waitForFinished(timeoutMs)) {
        listProc.kill();
        listProc.waitForFinished(2000);
        emit serverFetchError(QStringLiteral("Server list fetch timed out"));
        return std::unexpected(QStringLiteral("Server list fetch timed out after %1 ms")
                                   .arg(timeoutMs));
    }

    if (listProc.exitCode() != 0) {
        QString const stderrOut = QString::fromUtf8(listProc.readAllStandardError());
        emit serverFetchError(stderrOut);
        return std::unexpected(
            QStringLiteral("speedtest-cli --list failed (exit %1): %2")
                .arg(listProc.exitCode())
                .arg(stderrOut));
    }

    QByteArray const raw = listProc.readAll();
    QString const text = QString::fromUtf8(raw);

    std::vector<ServerInfo> servers = parseServerList(text);

    emit serverFetchFinished(servers);
    return servers;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  parseServerList — parses "speedtest-cli --list" output
//
//  Format:
//    34441) Telenor AB (Stockholm, Sweden) [7726.23 km]
//
//  Regex captures: id, sponsor+location, distance
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<ServerInfo>
ServerSelectionModule::parseServerList(const QString &rawText) noexcept
{
    std::vector<ServerInfo> servers;
    servers.reserve(128);   // Typical: 100–200 servers

    static QRegularExpression const re(
        QStringLiteral(R"(^\s*(\d+)\)\s+(.*)\s+\((.*)\)\s+\[([\d.]+)\s*km\]$)"));

    QStringList const lines = rawText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    for (QString const &line : lines) {
        QString const trimmed = line.trimmed();
        auto const m = re.match(trimmed);
        if (!m.hasMatch()) continue;

        ServerInfo s;
        s.id         = m.captured(1).toInt();
        s.sponsor    = m.captured(2).trimmed();
        s.location   = m.captured(3).trimmed();
        s.distanceKm = m.captured(4).toDouble();
        s.latencyMs  = 0.0;   // Unknown until pinged

        servers.push_back(std::move(s));
    }

    return servers;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Filtering
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<ServerInfo>
ServerSelectionModule::filterByDistance(const std::vector<ServerInfo> &servers,
                                        double maxKm) noexcept
{
    std::vector<ServerInfo> result;
    result.reserve(servers.size());

    std::copy_if(servers.begin(), servers.end(),
                 std::back_inserter(result),
                 [maxKm](const ServerInfo &s) noexcept {
                     return s.distanceKm <= maxKm;
                 });

    return result;
}

std::vector<ServerInfo>
ServerSelectionModule::filterByQuery(const std::vector<ServerInfo> &servers,
                                     const QString &query) noexcept
{
    if (query.isEmpty()) {
        return servers;
    }

    std::vector<ServerInfo> result;
    result.reserve(servers.size());

    QString const lowerQuery = query.toLower();

    std::copy_if(servers.begin(), servers.end(),
                 std::back_inserter(result),
                 [&lowerQuery](const ServerInfo &s) noexcept {
                     return s.sponsor.toLower().contains(lowerQuery)
                         || s.location.toLower().contains(lowerQuery);
                 });

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Sorting
// ═══════════════════════════════════════════════════════════════════════════════

void ServerSelectionModule::sortServers(std::vector<ServerInfo> &servers,
                                        SortBy by, SortOrder order) noexcept
{
    auto comparator = [by](const ServerInfo &a, const ServerInfo &b) noexcept -> bool {
        switch (by) {
        case SortBy::Id:       return a.id < b.id;
        case SortBy::Sponsor:  return a.sponsor.localeAwareCompare(b.sponsor) < 0;
        case SortBy::Location: return a.location.localeAwareCompare(b.location) < 0;
        case SortBy::Distance: return a.distanceKm < b.distanceKm;
        }
        return a.id < b.id; // fallback
    };

    if (order == SortOrder::Ascending) {
        std::sort(servers.begin(), servers.end(), comparator);
    } else {
        std::sort(servers.begin(), servers.end(),
                  [&comparator](const ServerInfo &a, const ServerInfo &b) noexcept {
                      return comparator(b, a);
                  });
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Find helpers
// ═══════════════════════════════════════════════════════════════════════════════

std::optional<ServerInfo>
ServerSelectionModule::findClosestServer(const std::vector<ServerInfo> &servers) noexcept
{
    if (servers.empty()) return std::nullopt;

    auto it = std::min_element(servers.begin(), servers.end(),
                               [](const ServerInfo &a, const ServerInfo &b) noexcept {
                                   return a.distanceKm < b.distanceKm;
                               });

    if (it == servers.end()) return std::nullopt;
    return *it;
}

std::optional<ServerInfo>
ServerSelectionModule::findServerById(const std::vector<ServerInfo> &servers,
                                       int serverId) noexcept
{
    auto it = std::find_if(servers.begin(), servers.end(),
                           [serverId](const ServerInfo &s) noexcept {
                               return s.id == serverId;
                           });

    if (it == servers.end()) return std::nullopt;
    return *it;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Binary helper
// ═══════════════════════════════════════════════════════════════════════════════

QString ServerSelectionModule::findSpeedtestBinary() noexcept
{
    QString p = QStandardPaths::findExecutable(QStringLiteral("speedtest-cli"));
    if (p.isEmpty()) {
        p = QStandardPaths::findExecutable(QStringLiteral("speedtest"));
    }
    return p;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private helpers
// ═══════════════════════════════════════════════════════════════════════════════

int ServerSelectionModule::extractServerId(const QString &line) noexcept
{
    static QRegularExpression const re(QStringLiteral(R"(^\s*(\d+)\))"));
    auto const m = re.match(line.trimmed());
    if (m.hasMatch()) {
        return m.captured(1).toInt();
    }
    return 0;
}

} // namespace IPView::Speedtest
