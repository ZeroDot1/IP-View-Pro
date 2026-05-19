// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — AlertEngine.cpp
//  C++26: std::ranges, std::erase_if, noexcept, [[nodiscard]]
//  Alert/Notification Engine — Regel-basierte Überwachung.
//  Item 49: Alert-Engine mit Default-Regeln für Bandbreite + TLS.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "AlertEngine.h"
#include "TelemetryModule.h"   // InterfaceInfo
#include "AuditorModule.h"     // AuditResult

#include <algorithm>
#include <ranges>
#include <numeric>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Alert {

// ── Helper: Bytes formatieren ────────────────────────────────────────────
static QString formatBytes(double bytes) noexcept
{
    if (bytes < 1024.0)
        return QStringLiteral("%1 B").arg(bytes, 0, 'f', 1);
    if (bytes < 1024.0 * 1024.0)
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024.0 * 1024.0 * 1024.0)
        return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    return QStringLiteral("%1 GB").arg(bytes / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
}

// ═══════════════════════════════════════════════════════════════════════════════
AlertEngine::AlertEngine(QObject *parent)
    : QObject(parent)
{
    registerDefaultRules();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Regel-Verwaltung
// ═══════════════════════════════════════════════════════════════════════════════

void AlertEngine::addRule(const Rule &rule) noexcept
{
    // Doppelte Regeln vermeiden
    auto it = std::ranges::find_if(mRules, [&](const Rule &r) { return r.name == rule.name; });
    if (it != mRules.end()) {
        *it = rule; // Aktualisieren
    } else {
        mRules.push_back(rule);
    }
}

void AlertEngine::removeRule(const QString &name) noexcept
{
    std::erase_if(mRules, [&](const Rule &r) { return r.name == name; });
}

void AlertEngine::clearRules() noexcept
{
    mRules.clear();
    mCooldowns.clear();
}

std::vector<Rule> AlertEngine::rules() const noexcept
{
    return mRules;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Alert-Management
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<Alert> AlertEngine::activeAlerts() const noexcept
{
    std::vector<Alert> result;
    std::ranges::copy_if(mAlerts, std::back_inserter(result),
                         [](const Alert &a) { return !a.acknowledged; });
    return result;
}

std::vector<Alert> AlertEngine::allAlerts() const noexcept
{
    return mAlerts;
}

void AlertEngine::acknowledgeAlert(const QString &id) noexcept
{
    auto it = std::ranges::find_if(mAlerts, [&](const Alert &a) { return a.id == id; });
    if (it != mAlerts.end()) {
        it->acknowledged = true;
        emit alertAcknowledged(id);
        emit alertsChanged();
    }
}

void AlertEngine::clearAcknowledged() noexcept
{
    std::erase_if(mAlerts, [](const Alert &a) { return a.acknowledged; });
    emit alertsChanged();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Cooldown-Prüfung
// ═══════════════════════════════════════════════════════════════════════════════

bool AlertEngine::isOnCooldown(const QString &ruleName) const noexcept
{
    auto it = std::ranges::find_if(mCooldowns,
                                   [&](const CooldownEntry &e) { return e.ruleName == ruleName; });
    if (it == mCooldowns.end()) return false;

    auto const elapsed = it->lastFired.secsTo(QDateTime::currentDateTimeUtc());
    return elapsed < 0; // negativ = noch in Cooldown (wird beim Fire aktualisiert)
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Telemetry-Feed
// ═══════════════════════════════════════════════════════════════════════════════

void AlertEngine::feedTelemetry(
    const QList<IPView::Telemetry::InterfaceInfo> &interfaces) noexcept
{
    if (interfaces.isEmpty()) return;

    // ── Regel: Hohe Bandbreitenauslastung ────────────────────────────────
    auto const bwRule = std::ranges::find_if(mRules,
        [](const Rule &r) { return r.name == QStringLiteral("high_bandwidth"); });

    if (bwRule != mRules.end() && bwRule->enabled) {
        for (auto const &iface : interfaces) {
            double const totalSpeed = iface.rxSpeedBps + iface.txSpeedBps;

            if (totalSpeed >= bwRule->criticalThreshold) {
                // Critical: Bandbreite überschreitet kritische Schwelle
                Alert alert;
                alert.id = QStringLiteral("bw_critical_%1").arg(iface.name);
                alert.title = QStringLiteral("Critical Bandwidth: %1").arg(iface.name);
                alert.message = QStringLiteral(
                    "Interface %1: %2/s (threshold: %3/s)")
                    .arg(iface.name,
                         formatBytes(totalSpeed),
                         formatBytes(bwRule->criticalThreshold));
                alert.severity = Severity::Critical;
                alert.category = Category::Telemetry;
                alert.source = iface.name;
                alert.thresholdValue = bwRule->criticalThreshold;
                alert.currentValue = totalSpeed;

                fireAlert(std::move(alert));
            } else if (totalSpeed >= bwRule->warningThreshold) {
                Alert alert;
                alert.id = QStringLiteral("bw_warning_%1").arg(iface.name);
                alert.title = QStringLiteral("High Bandwidth: %1").arg(iface.name);
                alert.message = QStringLiteral(
                    "Interface %1: %2/s exceeds warning threshold %3/s")
                    .arg(iface.name,
                         formatBytes(totalSpeed),
                         formatBytes(bwRule->warningThreshold));
                alert.severity = Severity::Warning;
                alert.category = Category::Telemetry;
                alert.source = iface.name;
                alert.thresholdValue = bwRule->warningThreshold;
                alert.currentValue = totalSpeed;

                fireAlert(std::move(alert));
            }
        }
    }

    // ── Regel: Interface-Fehler ──────────────────────────────────────────
    auto const errRule = std::ranges::find_if(mRules,
        [](const Rule &r) { return r.name == QStringLiteral("interface_errors"); });

    if (errRule != mRules.end() && errRule->enabled) {
        for (auto const &iface : interfaces) {
            std::uint64_t const totalErrors = iface.current.rxErrors + iface.current.txErrors;
            if (totalErrors > static_cast<std::uint64_t>(errRule->criticalThreshold)) {
                Alert alert;
                alert.id = QStringLiteral("err_%1").arg(iface.name);
                alert.title = QStringLiteral("Interface Errors: %1").arg(iface.name);
                alert.message = QStringLiteral(
                    "Interface %1 has %2 errors (threshold: %3)")
                    .arg(iface.name)
                    .arg(totalErrors)
                    .arg(static_cast<std::uint64_t>(errRule->criticalThreshold));
                alert.severity = Severity::Warning;
                alert.category = Category::Telemetry;
                alert.source = iface.name;
                alert.thresholdValue = errRule->criticalThreshold;
                alert.currentValue = static_cast<double>(totalErrors);

                fireAlert(std::move(alert));
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Audit-Feed
// ═══════════════════════════════════════════════════════════════════════════════

void AlertEngine::feedAuditResult(const IPView::Auditor::AuditResult &result) noexcept
{
    // ── Regel: Zertifikatsablauf ─────────────────────────────────────────
    auto const certRule = std::ranges::find_if(mRules,
        [](const Rule &r) { return r.name == QStringLiteral("cert_expiry"); });

    if (certRule != mRules.end() && certRule->enabled) {
        if (result.daysRemaining <= 0) {
            Alert alert;
            alert.id = QStringLiteral("cert_expired_%1").arg(result.host);
            alert.title = QStringLiteral("TLS Certificate Expired: %1").arg(result.host);
            alert.message = QStringLiteral(
                "Certificate for %1:%2 has expired!")
                .arg(result.host).arg(result.port);
            alert.severity = Severity::Critical;
            alert.category = Category::Security;
            alert.source = result.host;
            alert.currentValue = static_cast<double>(result.daysRemaining);

            fireAlert(std::move(alert));
        } else if (result.daysRemaining <= static_cast<int>(certRule->warningThreshold)) {
            Alert alert;
            alert.id = QStringLiteral("cert_expiring_%1").arg(result.host);
            alert.title = QStringLiteral("TLS Certificate Expiring Soon: %1").arg(result.host);
            alert.message = QStringLiteral(
                "Certificate for %1:%2 expires in %3 days!")
                .arg(result.host).arg(result.port).arg(result.daysRemaining);
            alert.severity = Severity::Warning;
            alert.category = Category::Security;
            alert.source = result.host;
            alert.thresholdValue = certRule->warningThreshold;
            alert.currentValue = static_cast<double>(result.daysRemaining);

            fireAlert(std::move(alert));
        }
    }

    // ── Regel: Unsicheres Zertifikat ─────────────────────────────────────
    auto const secureRule = std::ranges::find_if(mRules,
        [](const Rule &r) { return r.name == QStringLiteral("insecure_cert"); });

    if (secureRule != mRules.end() && secureRule->enabled) {
        if (!result.isSecure && result.daysRemaining > 0) {
            Alert alert;
            alert.id = QStringLiteral("insecure_%1").arg(result.host);
            alert.title = QStringLiteral("Insecure TLS: %1").arg(result.host);
            alert.message = QStringLiteral(
                "Host %1:%2 returned an untrusted certificate (self-signed or invalid chain).")
                .arg(result.host).arg(result.port);
            alert.severity = Severity::Warning;
            alert.category = Category::Security;
            alert.source = result.host;

            fireAlert(std::move(alert));
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Manueller Alert
// ═══════════════════════════════════════════════════════════════════════════════

void AlertEngine::triggerAlert(const QString &title, const QString &message,
                                Severity sev, Category cat) noexcept
{
    Alert alert;
    alert.id = QStringLiteral("manual_%1").arg(
        QString::number(QDateTime::currentMSecsSinceEpoch()));
    alert.title = title;
    alert.message = message;
    alert.severity = sev;
    alert.category = cat;

    fireAlert(std::move(alert));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private
// ═══════════════════════════════════════════════════════════════════════════════

void AlertEngine::fireAlert(Alert &&alert) noexcept
{
    // Cooldown-Prüfung: Gleiche ID innerhalb der Sperrfrist?
    auto cdIt = std::ranges::find_if(mCooldowns,
        [&](const CooldownEntry &e) { return e.ruleName == alert.id; });

    QDateTime const now = QDateTime::currentDateTimeUtc();
    if (cdIt != mCooldowns.end()) {
        auto const elapsed = cdIt->lastFired.secsTo(now);
        if (elapsed < 300) return; // 5 Minuten Default-Cooldown
        cdIt->lastFired = now;
    } else {
        mCooldowns.push_back(CooldownEntry{alert.id, now});
    }

    mAlerts.push_back(std::move(alert));
    Alert const &added = mAlerts.back();

    emit alertTriggered(added);
    emit alertsChanged();
}

// ═══════════════════════════════════════════════════════════════════════════════
void AlertEngine::registerDefaultRules() noexcept
{
    // ── Regel: Hohe Bandbreite ─────────────────────────────────────────
    Rule bwRule;
    bwRule.name             = QStringLiteral("high_bandwidth");
    bwRule.description      = QStringLiteral("Alarm bei hoher Bandbreitenauslastung");
    bwRule.category         = Category::Telemetry;
    bwRule.severity         = Severity::Warning;
    bwRule.warningThreshold = 100.0 * 1024.0 * 1024.0;     // 100 MB/s
    bwRule.criticalThreshold= 500.0 * 1024.0 * 1024.0;     // 500 MB/s
    bwRule.cooldownSec      = 120;
    bwRule.enabled          = true;
    mRules.push_back(bwRule);

    // ── Regel: Interface-Fehler ───────────────────────────────────────
    Rule errRule;
    errRule.name             = QStringLiteral("interface_errors");
    errRule.description      = QStringLiteral("Alarm bei vielen Interface-Fehlern");
    errRule.category         = Category::Telemetry;
    errRule.severity         = Severity::Warning;
    errRule.warningThreshold = 100.0;     // 100 Fehler
    errRule.criticalThreshold= 1000.0;    // 1000 Fehler
    errRule.cooldownSec      = 300;
    errRule.enabled          = true;
    mRules.push_back(errRule);

    // ── Regel: Zertifikatsablauf ──────────────────────────────────────
    Rule certRule;
    certRule.name             = QStringLiteral("cert_expiry");
    certRule.description      = QStringLiteral("Alarm bei bald ablaufenden TLS-Zertifikaten");
    certRule.category         = Category::Security;
    certRule.severity         = Severity::Warning;
    certRule.warningThreshold = 30.0;     // 30 Tage vor Ablauf
    certRule.cooldownSec      = 86400;    // 1x pro Tag
    certRule.enabled          = true;
    mRules.push_back(certRule);

    // ── Regel: Unsicheres Zertifikat ──────────────────────────────────
    Rule insecureRule;
    insecureRule.name        = QStringLiteral("insecure_cert");
    insecureRule.description = QStringLiteral("Alarm bei unsicheren/self-signed Zertifikaten");
    insecureRule.category    = Category::Security;
    insecureRule.severity    = Severity::Warning;
    insecureRule.cooldownSec = 3600;      // 1x pro Stunde
    insecureRule.enabled     = true;
    mRules.push_back(insecureRule);
}

} // namespace IPView::Alert
