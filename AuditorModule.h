// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — AuditorModule.h
//  C++26: std::expected, [[nodiscard]], noexcept, std::string_view
//  TLS/SSL Certificate Auditor — validates certificates for given hosts.
//  Item 48: TLS-Auditor — prüft Zertifikatsgültigkeit, Chain, Ablaufdatum.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef AUDITORMODULE_H
#define AUDITORMODULE_H

#include <QObject>
#include <QSslCertificate>
#include <QSslConfiguration>
#include <QString>
#include <QStringList>
#include <QDateTime>

#include <expected>
#include <vector>
#include <string>
#include <optional>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Auditor {

// ── Ergebnis einer einzelnen Zertifikatsprüfung ──────────────────────────
struct CertificateInfo {
    QString subject;               // CN / Subject
    QString issuer;                // Aussteller
    QDateTime validFrom;           // Gültig ab
    QDateTime validTo;             // Gültig bis
    bool     isSelfSigned{false};  // Selbstsigniert?
    bool     isExpired{false};     // Abgelaufen?
    bool     isValid{false};       // Gesamt-Validierung bestanden?
    QStringList subjectAltNames;   // SANs
    QString errorMessage;          // Fehlerdetails falls ungültig
};

// ── Ergebnis pro geprüftem Host ─────────────────────────────────────────
struct AuditResult {
    QString                host;           // Geprüfter Hostname
    int                    port{443};      // Port
    std::vector<CertificateInfo> chain;    // Zertifikatskette (End-→Root)
    int                    daysRemaining{0}; // Tage bis Ablauf
    bool                   isSecure{false}; // Gesamtbewertung
    qint64                 latencyMs{0};    // Verbindungslatenz
};

// ═══════════════════════════════════════════════════════════════════════════════
class AuditorModule : public QObject
{
    Q_OBJECT

public:
    explicit AuditorModule(QObject *parent = nullptr);
    ~AuditorModule() override = default;

    // ── Public API ──────────────────────────────────────────────────────────
    /// Einzelnen Host prüfen (blockierend, mit Timeout).
    [[nodiscard]] std::expected<AuditResult, QString>
    auditHost(const QString &host, int port = 443, int timeoutMs = 10000) noexcept;

    /// Mehrere Hosts nacheinander prüfen.
    [[nodiscard]] std::vector<AuditResult>
    auditHosts(const QStringList &hosts, int port = 443, int timeoutMs = 10000) noexcept;

    /// Batch aus einer Textliste (ein Host pro Zeile, optional "host:port").
    [[nodiscard]] std::vector<AuditResult>
    auditHostList(const QString &rawList, int defaultPort = 443, int timeoutMs = 10000) noexcept;

signals:
    void auditStarted(const QString &host);
    void auditFinished(const QString &host, const AuditResult &result);
    void auditError(const QString &host, const QString &error);
    void batchProgress(int current, int total);

private:
    // ── Hilfsfunktionen ────────────────────────────────────────────────────
    [[nodiscard]] CertificateInfo validateCertificate(const QSslCertificate &cert) noexcept;
    [[nodiscard]] AuditResult      performAudit(const QString &host, int port,
                                                  int timeoutMs) noexcept;
};

} // namespace IPView::Auditor

#endif // AUDITORMODULE_H
