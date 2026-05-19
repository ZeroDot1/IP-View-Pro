// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — AuditorModule.h
//  C++26: std::expected, [[nodiscard]], noexcept, std::string_view
//  TLS/SSL Certificate Auditor — validates certificates for given hosts.
//  Item 48: TLS Auditor — validates certificate validity, chain, expiry.
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

// ── Result of a single certificate check ────────────────────────────────
struct CertificateInfo {
    QString subject;               // CN / Subject
    QString issuer;                // Aussteller
    QDateTime validFrom;           // Valid from
    QDateTime validTo;             // Valid to
    bool     isSelfSigned{false};  // Self-signed?
    bool     isExpired{false};     // Expired?
    bool     isValid{false};       // Gesamt-Validierung bestanden?
    QStringList subjectAltNames;   // SANs
    QString errorMessage;          // Error details if invalid
};

// ── Result per audited host ────────────────────────────────────────────
struct AuditResult {
    QString                host;           // Audited hostname
    int                    port{443};      // Port
    std::vector<CertificateInfo> chain;    // Certificate chain (leaf → root)
    int                    daysRemaining{0}; // Days until expiry
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
    /// Audit a single host (blocking, with timeout).
    [[nodiscard]] std::expected<AuditResult, QString>
    auditHost(const QString &host, int port = 443, int timeoutMs = 10000) noexcept;

    /// Audit multiple hosts sequentially.
    [[nodiscard]] std::vector<AuditResult>
    auditHosts(const QStringList &hosts, int port = 443, int timeoutMs = 10000) noexcept;

    /// Batch audit from a text list (one host per line, optionally "host:port").
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
