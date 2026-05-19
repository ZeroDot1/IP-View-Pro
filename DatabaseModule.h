// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — DatabaseModule.h
//  C++26: [[nodiscard]], noexcept, const-correctness
//  Persistence layer (SQLite) for IP history and telemetry data.
//  Thread-safe via QMutex. Singleton pattern.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef DATABASEMODULE_H
#define DATABASEMODULE_H

#include <QSqlDatabase>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutex>
#include <QString>
#include <QStringList>
#include <QDateTime>

#include <optional>   // C++17/26: std::optional
#include <vector>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Storage {

// ── Database entry for IP history ────────────────────────────────────────
struct HistoryEntry {
    qint64    id{-1};
    QString   ip;
    QString   countryName;
    QString   countryCode;
    QString   city;
    QString   org;
    QString   asn;
    QString   jsonPayload;    // Complete JSON dataset
    QDateTime timestamp;
};

// ── Aggregated telemetry data (stored in telemetry_aggregated table) ────────
struct AggregatedTelemetryEntry {
    qint64    id{-1};
    QString   interfaceName;
    double    avgRxSpeed{0.0};
    double    avgTxSpeed{0.0};
    double    minRxSpeed{0.0};
    double    minTxSpeed{0.0};
    double    maxRxSpeed{0.0};
    double    maxTxSpeed{0.0};
    quint64   totalRxBytes{0};
    quint64   totalTxBytes{0};
    QDateTime windowStart;
    QDateTime windowEnd;
};

// ═══════════════════════════════════════════════════════════════════════════════
class DatabaseModule
{
public:
    // ── Singleton ───────────────────────────────────────────────────────────
    DatabaseModule(const DatabaseModule &) = delete;
    DatabaseModule &operator=(const DatabaseModule &) = delete;

    [[nodiscard]] static DatabaseModule& instance() noexcept;

    // ── Init / Shutdown ────────────────────────────────────────────────────
    [[nodiscard]] static bool init(const QString &dbPath = QString()) noexcept;
    static void               shutdown() noexcept;
    [[nodiscard]] static bool isInitialized() noexcept;

    // ── Write Operations ───────────────────────────────────────────────────
    static bool storeResult(const QJsonObject &data) noexcept;
    static bool storeTelemetry(const QString &interfaceName,
                               quint64 rxBytes, quint64 txBytes,
                               double rxSpeed, double txSpeed) noexcept;

    // ── Read Operations ────────────────────────────────────────────────────
    [[nodiscard]] static std::vector<HistoryEntry> getHistory(int limit = 100) noexcept;
    [[nodiscard]] static std::optional<HistoryEntry> getLatestEntry() noexcept;
    [[nodiscard]] static int getHistoryCount() noexcept;

    // ── Aggregated Telemetry (for TelemetryPersistenceModule) ────────────
    static bool storeTelemetryAggregated(const QString &interfaceName,
                                         double avgRxSpeed, double avgTxSpeed,
                                         double minRxSpeed, double minTxSpeed,
                                         double maxRxSpeed, double maxTxSpeed,
                                         quint64 totalRxBytes, quint64 totalTxBytes,
                                         const QDateTime &windowStart,
                                         const QDateTime &windowEnd) noexcept;

    [[nodiscard]] static std::vector<AggregatedTelemetryEntry>
    getTelemetryAggregated(int limit = 100) noexcept;

    [[nodiscard]] static std::vector<AggregatedTelemetryEntry>
    getTelemetryAggregatedForInterface(const QString &interfaceName,
                                       int limit = 100) noexcept;

    [[nodiscard]] static std::optional<AggregatedTelemetryEntry>
    getLatestTelemetryAggregated() noexcept;

    [[nodiscard]] static std::optional<AggregatedTelemetryEntry>
    getTelemetryStatsForWindow(const QDateTime &from,
                               const QDateTime &to) noexcept;

    static bool clearTelemetryAggregated() noexcept;

    // ── Maintenance ────────────────────────────────────────────────────────
    static bool clearHistory() noexcept;
    static bool vacuum() noexcept;

private:
    DatabaseModule() = default;
    ~DatabaseModule() = default;

    // ── Schema ─────────────────────────────────────────────────────────────
    static bool createSchema() noexcept;
    [[nodiscard]] static QString defaultDbPath() noexcept;

    // ── State ─────────────────────────────────────────────────────────────
    static QSqlDatabase   sDb;
    static QMutex         sMutex;
    static bool           sInitialized;
    static QString        sDbPath;
};

} // namespace IPView::Storage

#endif // DATABASEMODULE_H
