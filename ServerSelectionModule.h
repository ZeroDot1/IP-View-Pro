// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — ServerSelectionModule.h
//  C++26: [[nodiscard]], noexcept, const-correctness, std::optional
//  Speedtest server selection — fetch, parse, filter, sort servers.
//  Extracted from SpeedtestTab for modular use.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef SERVERSELECTIONMODULE_H
#define SERVERSELECTIONMODULE_H

#include <QString>
#include <QStringList>
#include <QProcess>
#include <QObject>

#include <vector>
#include <optional>
#include <algorithm>
#include <expected>     // C++26

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Speedtest {

// ── Server information structure ────────────────────────────────────────────
struct ServerInfo {
    int     id{0};
    QString sponsor;
    QString location;
    double  distanceKm{0.0};
    double  latencyMs{0.0};    // Populated after ping test (0.0 = unknown)
};

// ═══════════════════════════════════════════════════════════════════════════════
class ServerSelectionModule : public QObject
{
    Q_OBJECT

public:
    explicit ServerSelectionModule(QObject *parent = nullptr);
    ~ServerSelectionModule() override = default;

    // ── Fetch available servers via speedtest-cli --list ──────────────────
    //  Returns std::expected with vector of servers or error string.
    [[nodiscard]] std::expected<std::vector<ServerInfo>, QString>
    getAvailableServers(int timeoutMs = 30000) noexcept;

    // ── Convenience: parse raw --list output ──────────────────────────────
    [[nodiscard]] static std::vector<ServerInfo>
    parseServerList(const QString &rawText) noexcept;

    // ── Filtering ─────────────────────────────────────────────────────────
    [[nodiscard]] static std::vector<ServerInfo>
    filterByDistance(const std::vector<ServerInfo> &servers,
                     double maxKm) noexcept;

    [[nodiscard]] static std::vector<ServerInfo>
    filterByQuery(const std::vector<ServerInfo> &servers,
                  const QString &query) noexcept;

    // ── Sorting ───────────────────────────────────────────────────────────
    enum class SortBy { Id, Sponsor, Location, Distance };
    enum class SortOrder { Ascending, Descending };

    static void sortServers(std::vector<ServerInfo> &servers,
                            SortBy by = SortBy::Distance,
                            SortOrder order = SortOrder::Ascending) noexcept;

    // ── Find best server (shortest distance or lowest latency) ────────────
    [[nodiscard]] static std::optional<ServerInfo>
    findClosestServer(const std::vector<ServerInfo> &servers) noexcept;

    // ── Auto-Select (Item 44): bester Server via Distanz+Latenz-Scoring ──
    [[nodiscard]] static std::optional<ServerInfo>
    selectBestServer(const std::vector<ServerInfo> &servers) noexcept;

    // ── Find server by ID ─────────────────────────────────────────────────
    [[nodiscard]] static std::optional<ServerInfo>
    findServerById(const std::vector<ServerInfo> &servers,
                   int serverId) noexcept;

    // ── Find speedtest-cli binary path ────────────────────────────────────
    [[nodiscard]] static QString findSpeedtestBinary() noexcept;

signals:
    void serverFetchStarted();
    void serverFetchProgress(int current, int total);
    void serverFetchFinished(const std::vector<IPView::Speedtest::ServerInfo> &servers);
    void serverFetchError(const QString &error);

private:
    // ── Internal helper: extract server ID from a parsed line ─────────────
    [[nodiscard]] static int extractServerId(const QString &line) noexcept;
};

} // namespace IPView::Speedtest

#endif // SERVERSELECTIONMODULE_H
