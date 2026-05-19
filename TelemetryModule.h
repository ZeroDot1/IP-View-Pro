// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — TelemetryModule.h
//  C++26: std::expected, std::string_view, [[nodiscard]], noexcept
//  Real-time network telemetry via /proc/net/dev in a background thread.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef TELEMETRYMODULE_H
#define TELEMETRYMODULE_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QStringList>

#include <expected>     // C++26: std::expected
#include <string>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Telemetry {

// ── Network statistics structure ────────────────────────────────────────────
struct Stats {
    std::uint64_t rxBytes{0};   // Received bytes
    std::uint64_t txBytes{0};   // Transmitted bytes
    std::uint64_t rxPackets{0}; // Received packets
    std::uint64_t txPackets{0}; // Transmitted packets
    std::uint64_t rxErrors{0};  // Receive errors
    std::uint64_t txErrors{0};  // Transmit errors
};

// ── Interface list ──────────────────────────────────────────────────────────
struct InterfaceInfo {
    QString name;
    Stats   current;
    Stats   previous;
    double  rxSpeedBps{0.0};   // Receive speed in bytes/s
    double  txSpeedBps{0.0};   // Transmit speed in bytes/s
};

// ═══════════════════════════════════════════════════════════════════════════════
class TelemetryModule : public QObject
{
    Q_OBJECT

public:
    explicit TelemetryModule(QObject *parent = nullptr);
    ~TelemetryModule() override = default;

    // ── Public API ──────────────────────────────────────────────────────────
    [[nodiscard]] std::expected<Stats, std::string> fetchStats(std::string_view interface) noexcept;
    [[nodiscard]] QStringList                       availableInterfaces() const noexcept;

    void startMonitoring(int intervalMs = 2000) noexcept;
    void stopMonitoring()                         noexcept;
    [[nodiscard]] bool isMonitoring() const       noexcept;

    [[nodiscard]] QList<InterfaceInfo> getAllInterfaces() const noexcept;

signals:
    // Emitted whenever new telemetry data is available
    void telemetryUpdated(const QList<IPView::Telemetry::InterfaceInfo> &interfaces);
    void errorOccurred(const QString &message);

private slots:
    void onTick() noexcept;

private:
    [[nodiscard]] Stats parseProcNetDev(const QString &content, std::string_view interface) noexcept;
    [[nodiscard]] bool  isValidInterface(std::string_view name) const noexcept;

    // ── State ────────────────────────────────────────────────────────────────
    QTimer               *mTimer{nullptr};
    QList<InterfaceInfo>  mInterfaces;
    mutable QStringList   mCachedInterfaces;
    mutable bool          mCacheValid{false};
    bool                  mMonitoring{false};
};

} // namespace IPView::Telemetry

#endif // TELEMETRYMODULE_H
