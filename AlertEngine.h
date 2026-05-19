// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — AlertEngine.h
//  C++26: std::expected, std::chrono, [[nodiscard]], noexcept
//  Alert/Notification Engine — monitors conditions and fires events.
//  Item 49: Alert Engine — rule-based automatic notifications.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef ALERTENGINE_H
#define ALERTENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QTimer>

#include <vector>
#include <chrono>
#include <cstdint>

// ── Forward Declarations ──────────────────────────────────────────────────
namespace IPView::Telemetry {
    struct InterfaceInfo;
}

namespace IPView::Auditor {
    struct AuditResult;
}

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Alert {

// ── Severity ─────────────────────────────────────────────────────────
enum class Severity : std::uint8_t {
    Info,
    Warning,
    Critical
};

// ── Category ──────────────────────────────────────────────────────────
enum class Category : std::uint8_t {
    Telemetry,    // bandwidth, error rates
    Security,     // TLS certificates
    System,       // system resources
    Custom
};

// ── A single Alert ─────────────────────────────────────────────────
struct Alert {
    QString     id;            // Unique ID (e.g. "high_bandwidth")
    QString     title;         // Short title
    QString     message;       // Detail message
    Severity    severity{Severity::Info};
    Category    category{Category::Custom};
    QDateTime   timestamp{QDateTime::currentDateTimeUtc()};
    bool        acknowledged{false};
    QString     source;        // Trigger source (e.g. interface name, host)
    double      thresholdValue{0.0};  // Threshold value
    double      currentValue{0.0};    // Current value
};

// ── Rule Definition ───────────────────────────────────────────────────
struct Rule {
    QString     name;          // Rule name
    QString     description;   // Description
    Category    category{Category::Custom};
    Severity    severity{Severity::Warning};
    double      warningThreshold{0.0};   // Lower threshold
    double      criticalThreshold{0.0};  // Upper threshold
    int         cooldownSec{300};        // Cooldown (seconds)
    bool        enabled{true};
};

// ═══════════════════════════════════════════════════════════════════════════════
class AlertEngine : public QObject
{
    Q_OBJECT

public:
    explicit AlertEngine(QObject *parent = nullptr);
    ~AlertEngine() override = default;

    // ── Rule Management ─────────────────────────────────────────────────
    void addRule(const Rule &rule) noexcept;
    void removeRule(const QString &name) noexcept;
    void clearRules() noexcept;
    [[nodiscard]] std::vector<Rule> rules() const noexcept;

    // ── Alert Management ─────────────────────────────────────────────────
    [[nodiscard]] std::vector<Alert> activeAlerts() const noexcept;
    [[nodiscard]] std::vector<Alert> allAlerts() const noexcept;
    void acknowledgeAlert(const QString &id) noexcept;
    void clearAcknowledged() noexcept;

    // ── Data Input (from other modules) ──────────────────────────────
    /// Check telemetry data (Item 49).
    void feedTelemetry(const QList<IPView::Telemetry::InterfaceInfo> &interfaces) noexcept;

    /// Check TLS audit result.
    void feedAuditResult(const IPView::Auditor::AuditResult &result) noexcept;

    /// Trigger a manual alert.
    void triggerAlert(const QString &title, const QString &message,
                      Severity sev = Severity::Warning,
                      Category cat = Category::Custom) noexcept;

signals:
    /// A new alert has been triggered.
    void alertTriggered(const IPView::Alert::Alert &alert);

    /// An alert has been acknowledged.
    void alertAcknowledged(const QString &id);

    /// All active alerts changed (for UI update).
    void alertsChanged();

private:
    [[nodiscard]] bool isOnCooldown(const QString &ruleName) const noexcept;

    /// Fire an alert (cooldown check + emission).
    void fireAlert(Alert &&alert) noexcept;

    // ── Built-in Default Rules ──────────────────────────────────────
    void registerDefaultRules() noexcept;

    std::vector<Rule>   mRules;
    std::vector<Alert>  mAlerts;
    std::vector<Alert>  mHistory;        // All alerts (incl. acknowledged)

    // Cooldown tracking: rule_name → last fire time
    struct CooldownEntry {
        QString     ruleName;
        QDateTime   lastFired;
    };
    std::vector<CooldownEntry> mCooldowns;
};

} // namespace IPView::Alert

#endif // ALERTENGINE_H
