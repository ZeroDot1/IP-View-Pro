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
#include <functional> // C++26: std::move_only_function
#include <functional>  // C++26: Callback for status feedback (Item 5)
// <coroutine> must come before <generator> so libstdc++'s
// __glibcxx_coroutine macro is defined; otherwise std::generator
// is silently dropped from <generator> on the C++26 feature-gate.
#include <coroutine>
#include <generator>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Storage {

class DatabaseWorker; // Forward (Item 14)

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
    [[nodiscard]] static bool storeResult(const QJsonObject &data) noexcept;
    [[nodiscard]] static bool storeTelemetry(const QString &interfaceName,
                               quint64 rxBytes, quint64 txBytes,
                               double rxSpeed, double txSpeed) noexcept;

    // ── Async Write Operations (Item 14) ─────────────────────────────────
    //  Delegate to DatabaseWorker → non-blocking.
    static void asyncStoreResult(const QJsonObject &data) noexcept;
    static void asyncStoreTelemetry(const QString &interfaceName,
                                    quint64 rxBytes, quint64 txBytes,
                                    double rxSpeed, double txSpeed) noexcept;

    // ── Read Operations ────────────────────────────────────────────────────
    [[nodiscard]] static std::vector<HistoryEntry> getHistory(int limit = 100) noexcept;
    [[nodiscard]] static std::optional<HistoryEntry> getLatestEntry() noexcept;
    [[nodiscard]] static int getHistoryCount() noexcept;

    // ── Lazy streaming history (Phase 2-C, C++26 std::generator) ────────────
    //  Same query as getHistory() but yields rows one at a time
    //  instead of materialising the whole result set into a
    //  std::vector. Useful for very large LIMITs where the
    //  caller only needs the first 10 matching rows (e.g. a
    //  search-as-you-type UI) or wants to abort early.
    //
    //  The generator holds the QSqlQuery alive for the whole
    //  iteration; once the consumer stops iterating, the
    //  destructor cleans up the cursor. The lock is acquired
    //  and released inside the generator body, so the QMutex
    //  is held for the duration of the query only — not for
    //  the whole consumer loop.
    [[nodiscard]] static std::generator<HistoryEntry> getHistoryStream(int limit = 100) noexcept;

    // ── Aggregated Telemetry (for TelemetryPersistenceModule) ────────────
    [[nodiscard]] static bool storeTelemetryAggregated(const QString &interfaceName,
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

    [[nodiscard]] static bool clearTelemetryAggregated() noexcept;

    // ── Transaction Support (Item 11) ────────────────────────────────────
    [[nodiscard]] static bool beginTransaction() noexcept;
    [[nodiscard]] static bool commitTransaction() noexcept;
    [[nodiscard]] static bool rollbackTransaction() noexcept;

    // ── Maintenance ────────────────────────────────────────────────────────
    [[nodiscard]] static bool clearHistory() noexcept;
    [[nodiscard]] static bool vacuum() noexcept;

    // ── Pruning (Item 18) ─────────────────────────────────────────────────
    [[nodiscard]] static bool pruneHistory(int keepDays = 30) noexcept;
    [[nodiscard]] static bool pruneTelemetry(int keepDays = 90) noexcept;

    // ── Integrity (Item 20) ───────────────────────────────────────────────
    [[nodiscard]] static bool integrityCheck() noexcept;

    // ── Status-Callback (Item 5) ─────────────────────────────────────────
    //  Enables UI feedback for DB background operations.
    //  std::move_only_function (C++26 stable) lets the callback
    //  capture non-copyable state like std::unique_ptr or
    //  QPointer<QObject>, which std::function could not.
    using StatusCallback = std::move_only_function<void(const QString &)>;
    static void setStatusCallback(StatusCallback cb) noexcept;
    static void clearStatusCallback() noexcept;

    // ── Cross-platform data directory (Item 16) ──────────────────────
    [[nodiscard]] static QString dataDirectory() noexcept;
    [[nodiscard]] static QString configDirectory() noexcept;

    // ── Worker Lifecycle (Item 14) ─────────────────────────────────────
    static void startWorker() noexcept;
    static void stopWorker() noexcept;
    [[nodiscard]] static bool isWorkerRunning() noexcept;

    // ── Internes Status-Forwarding (public for dbStatus helper) ──────────
    static void emitStatusMsg(const QString &msg) noexcept;

private:
    DatabaseModule() = default;
    ~DatabaseModule() = default;

    // ── Schema ─────────────────────────────────────────────────────────────
    [[nodiscard]] static bool createSchema() noexcept;
    [[nodiscard]] static QString defaultDbPath() noexcept;

    // ── State ─────────────────────────────────────────────────────────────
    static QSqlDatabase   sDb;
    static QMutex         sMutex;
    static bool           sInitialized;
    static QString        sDbPath;
    static StatusCallback sStatusCallback;

    // ── Worker (Item 14) ──────────────────────────────────────────────────
    static DatabaseWorker *sWorker;
};

} // namespace IPView::Storage

#endif // DATABASEMODULE_H
