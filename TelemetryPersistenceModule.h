// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — TelemetryPersistenceModule.h
//  C++26: [[nodiscard]], noexcept, const-correctness, std::optional
//  Periodic telemetry aggregation and historical data persistence.
//  Stores aggregated stats in SQLite via DatabaseModule.
//  Auto-start configurable via QSettings (ConfigManager).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef TELEMETRYPERSISTENCEMODULE_H
#define TELEMETRYPERSISTENCEMODULE_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QDateTime>

#include <optional>
#include <vector>
#include <map>
#include <cstdint>

#include "DatabaseModule.h"

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Telemetry {

// ── Aggregation window (time span) ──────────────────────────────────────────
enum class AggregationWindow : int {
    Minute  = 60,
    Hour    = 3600,
    Day     = 86400,
    Week    = 604800
};

// ═══════════════════════════════════════════════════════════════════════════════
class TelemetryPersistenceModule : public QObject
{
    Q_OBJECT

public:
    explicit TelemetryPersistenceModule(QObject *parent = nullptr);
    ~TelemetryPersistenceModule() override = default;

    // ── Lifecycle ──────────────────────────────────────────────────────────
    void start(int intervalMs = 60000) noexcept;   // Default: every 60 s
    void stop() noexcept;
    [[nodiscard]] bool isRunning() const noexcept;

    // ── Manual aggregation trigger ─────────────────────────────────────────
    //  Stores an aggregated snapshot of all interfaces into SQLite.
    void aggregateNow() noexcept;

    // ── Historical queries ─────────────────────────────────────────────────
    [[nodiscard]] std::vector<IPView::Storage::AggregatedTelemetryEntry>
    getHistory(int limit = 100) const noexcept;

    [[nodiscard]] std::vector<IPView::Storage::AggregatedTelemetryEntry>
    getHistoryForInterface(const QString &interfaceName,
                           int limit = 100) const noexcept;

    [[nodiscard]] std::optional<IPView::Storage::AggregatedTelemetryEntry>
    getLatestAggregation() const noexcept;

    [[nodiscard]] std::optional<IPView::Storage::AggregatedTelemetryEntry>
    getStatsForWindow(const QDateTime &from, const QDateTime &to) const noexcept;

    // ── Auto-start configuration ──────────────────────────────────────────
    static void setAutoStartEnabled(bool enabled) noexcept;
    [[nodiscard]] static bool isAutoStartEnabled() noexcept;
    static void setAutoStartInterval(int intervalMs) noexcept;
    [[nodiscard]] static int autoStartInterval() noexcept;

    // ── Maintenance ────────────────────────────────────────────────────────
    static bool clearAggregatedHistory() noexcept;

signals:
    void aggregationStored(const Storage::AggregatedTelemetryEntry &record);
    void aggregationError(const QString &message);
    void aggregationStarted();
    void aggregationCompleted();

private slots:
    void onAggregationTick() noexcept;

private:
    // ── Interface enumeration from /proc/net/dev ───────────────────────────
    [[nodiscard]] static QStringList listPhysicalInterfaces() noexcept;

    // ── Fetch current stats for one interface ─────────────────────────────
    [[nodiscard]] static std::optional<std::pair<quint64, quint64>>
    fetchInterfaceBytes(const QString &interfaceName) noexcept;

    // ── State ─────────────────────────────────────────────────────────────
    QTimer *mTimer{nullptr};

    // ── Previous sample for delta calculation ──────────────────────────────
    struct Sample {
        quint64 rxBytes{0};
        quint64 txBytes{0};
    };
    // Maps interface name → last sample + timestamp
    struct InterfaceSample {
        Sample    sample;
        QDateTime timestamp;
    };
    mutable std::map<QString, InterfaceSample> mPreviousSamples;
    mutable bool                               mSamplesLoaded{false};
};

} // namespace IPView::Telemetry

#endif // TELEMETRYPERSISTENCEMODULE_H
