// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — AlertEngine.h
//  C++26: std::expected, std::chrono, [[nodiscard]], noexcept
//  Alert/Notification Engine — überwacht Bedingungen und löst Events aus.
//  Item 49: Alert-Engine — Regelwerk für automatische Benachrichtigungen.
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

// ── Forward-Deklarationen ──────────────────────────────────────────────────
namespace IPView::Telemetry {
    struct InterfaceInfo;
}

namespace IPView::Auditor {
    struct AuditResult;
}

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Alert {

// ── Schweregrad ─────────────────────────────────────────────────────────
enum class Severity : std::uint8_t {
    Info,
    Warning,
    Critical
};

// ── Kategorie ──────────────────────────────────────────────────────────
enum class Category : std::uint8_t {
    Telemetry,    // Bandbreite, Fehlerraten
    Security,     // TLS-Zertifikate
    System,       // System-Ressourcen
    Custom
};

// ── Ein einzelner Alert ─────────────────────────────────────────────────
struct Alert {
    QString     id;            // Eindeutige ID (z.B. "high_bandwidth")
    QString     title;         // Kurztitel
    QString     message;       // Detailnachricht
    Severity    severity{Severity::Info};
    Category    category{Category::Custom};
    QDateTime   timestamp{QDateTime::currentDateTimeUtc()};
    bool        acknowledged{false};
    QString     source;        // Auslöser (z.B. Interface-Name, Host)
    double      thresholdValue{0.0};  // Schwellwert
    double      currentValue{0.0};    // Aktueller Wert
};

// ── Regel-Definition ───────────────────────────────────────────────────
struct Rule {
    QString     name;          // Regelname
    QString     description;   // Beschreibung
    Category    category{Category::Custom};
    Severity    severity{Severity::Warning};
    double      warningThreshold{0.0};   // Untere Schwelle
    double      criticalThreshold{0.0};  // Obere Schwelle
    int         cooldownSec{300};        // Wiederholungssperre (Sekunden)
    bool        enabled{true};
};

// ═══════════════════════════════════════════════════════════════════════════════
class AlertEngine : public QObject
{
    Q_OBJECT

public:
    explicit AlertEngine(QObject *parent = nullptr);
    ~AlertEngine() override = default;

    // ── Regel-Verwaltung ─────────────────────────────────────────────────
    void addRule(const Rule &rule) noexcept;
    void removeRule(const QString &name) noexcept;
    void clearRules() noexcept;
    [[nodiscard]] std::vector<Rule> rules() const noexcept;

    // ── Alert-Management ─────────────────────────────────────────────────
    [[nodiscard]] std::vector<Alert> activeAlerts() const noexcept;
    [[nodiscard]] std::vector<Alert> allAlerts() const noexcept;
    void acknowledgeAlert(const QString &id) noexcept;
    void clearAcknowledged() noexcept;

    // ── Daten-Eingang (von anderen Modulen) ──────────────────────────────
    /// Telemetry-Daten prüfen (Item 49).
    void feedTelemetry(const QList<IPView::Telemetry::InterfaceInfo> &interfaces) noexcept;

    /// TLS-Audit-Ergebnis prüfen.
    void feedAuditResult(const IPView::Auditor::AuditResult &result) noexcept;

    /// Manuellen Alert auslösen.
    void triggerAlert(const QString &title, const QString &message,
                      Severity sev = Severity::Warning,
                      Category cat = Category::Custom) noexcept;

signals:
    /// Ein neuer Alert wurde ausgelöst.
    void alertTriggered(const IPView::Alert::Alert &alert);

    /// Ein Alert wurde quittiert.
    void alertAcknowledged(const QString &id);

    /// Alle aktiven Alerts (für UI-Aktualisierung).
    void alertsChanged();

private:
    [[nodiscard]] bool isOnCooldown(const QString &ruleName) const noexcept;

    /// Einen Alert feuern (Cooldown-Prüfung + Emission).
    void fireAlert(Alert &&alert) noexcept;

    // ── Eingebaute Regeln (Default) ──────────────────────────────────────
    void registerDefaultRules() noexcept;

    std::vector<Rule>   mRules;
    std::vector<Alert>  mAlerts;
    std::vector<Alert>  mHistory;        // Alle Alerts (inkl. quittierte)

    // Cooldown-Tracking: rule_name → letzte Auslösung
    struct CooldownEntry {
        QString     ruleName;
        QDateTime   lastFired;
    };
    std::vector<CooldownEntry> mCooldowns;
};

} // namespace IPView::Alert

#endif // ALERTENGINE_H
