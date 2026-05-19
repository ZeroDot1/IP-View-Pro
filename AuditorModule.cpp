// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — AuditorModule.cpp
//  C++26: std::expected, std::from_chars, noexcept, [[nodiscard]]
//  TLS/SSL Certificate Auditor — QSslSocket-basierte Zertifikatsprüfung.
//  Item 48: TLS-Auditor — prüft Gültigkeit, Kette, Ablauf und SANs.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "AuditorModule.h"

#include <QSslSocket>
#include <QSslConfiguration>
#include <QSsl>
#include <QUrl>
#include <QElapsedTimer>
#include <QRegularExpression>

#include <algorithm>
#include <ranges>
#include <charconv>
#include <system_error>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Auditor {

// ═══════════════════════════════════════════════════════════════════════════════
AuditorModule::AuditorModule(QObject *parent)
    : QObject(parent)
{
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════════

std::expected<AuditResult, QString>
AuditorModule::auditHost(const QString &host, int port, int timeoutMs) noexcept
{
    if (host.trimmed().isEmpty()) {
        return std::unexpected(QStringLiteral("Empty hostname provided"));
    }

    emit auditStarted(host);
    AuditResult result = performAudit(host.trimmed(), port, timeoutMs);
    emit auditFinished(host, result);
    return result;
}

std::vector<AuditResult>
AuditorModule::auditHosts(const QStringList &hosts, int port, int timeoutMs) noexcept
{
    std::vector<AuditResult> results;
    results.reserve(static_cast<std::size_t>(hosts.size()));

    int const total = static_cast<int>(hosts.size());
    for (int i = 0; i < total; ++i) {
        QString const &h = hosts[i].trimmed();
        if (h.isEmpty()) continue;

        emit batchProgress(i + 1, total);

        auto r = auditHost(h, port, timeoutMs);
        if (r.has_value()) {
            results.push_back(std::move(*r));
        }
    }

    return results;
}

std::vector<AuditResult>
AuditorModule::auditHostList(const QString &rawList, int defaultPort, int timeoutMs) noexcept
{
    QStringList const lines = rawList.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    std::vector<AuditResult> results;
    results.reserve(static_cast<std::size_t>(lines.size()));

    static QRegularExpression const portRe(QStringLiteral(R"(^(.+):(\d+)$)"));

    auto const total = static_cast<int>(lines.size());
    for (int i = 0; i < total; ++i) {
        QString const line = lines[i].trimmed();
        if (line.isEmpty()) continue;

        emit batchProgress(i + 1, total);

        QString host;
        int     port = defaultPort;

        auto m = portRe.match(line);
        if (m.hasMatch()) {
            host = m.captured(1).trimmed();
            port = m.captured(2).toInt();
        } else {
            host = line;
        }

        auto r = auditHost(host, port, timeoutMs);
        if (r.has_value()) {
            results.push_back(std::move(*r));
        }
    }

    return results;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private — performAudit
// ═══════════════════════════════════════════════════════════════════════════════

AuditResult
AuditorModule::performAudit(const QString &host, int port, int timeoutMs) noexcept
{
    AuditResult result;
    result.host = host;
    result.port = port;

    QElapsedTimer timer;
    timer.start();

    // ── SSL-Verbindung aufbauen ────────────────────────────────────────────
    auto *socket = new QSslSocket();

    // Ignore self-signed for reporting purposes (we detect it ourselves)
    QSslConfiguration sslConfig = socket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::QueryPeer);
    socket->setSslConfiguration(sslConfig);

    socket->connectToHostEncrypted(host, static_cast<std::uint16_t>(port));

    if (!socket->waitForEncrypted(timeoutMs)) {
        QString const err = socket->errorString();
        delete socket;

        result.isSecure = false;
        result.latencyMs = timer.elapsed();

        CertificateInfo ci;
        ci.isValid = false;
        ci.errorMessage = err;
        result.chain.push_back(std::move(ci));

        return result;
    }

    result.latencyMs = timer.elapsed();

    // ── Zertifikatskette abrufen ───────────────────────────────────────────
    QList<QSslCertificate> const certChain = socket->peerCertificateChain();
    QSslCertificate const peerCert        = socket->peerCertificate();

    if (peerCert.isNull()) {
        delete socket;
        result.isSecure = false;
        return result;
    }

    // Jedes Zertifikat in der Kette validieren
    for (QSslCertificate const &cert : certChain) {
        result.chain.push_back(validateCertificate(cert));
    }

    // Falls leere Kette, zumindest das Peer-Zertifikat einfügen
    if (result.chain.empty()) {
        result.chain.push_back(validateCertificate(peerCert));
    }

    // ── Ablauf-Tage berechnen ──────────────────────────────────────────────
    QDateTime const expiryDate = peerCert.expiryDate();
    if (expiryDate.isValid()) {
        result.daysRemaining = static_cast<int>(QDateTime::currentDateTimeUtc().secsTo(expiryDate) / 86400);
    }

    // ── Gesamtbewertung ────────────────────────────────────────────────────
    result.isSecure = std::ranges::all_of(result.chain, [](const CertificateInfo &ci) noexcept {
        return ci.isValid;
    });

    // Wenn das Zertifikat selbstsigniert ist, aber sonst gültig → tolerant
    if (result.chain.size() == 1 && result.chain.front().isSelfSigned && result.chain.front().isValid) {
        result.isSecure = false; // Selbstsigniert = nicht vertrauenswürdig
    }

    socket->disconnectFromHost();
    socket->deleteLater();

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private — validateCertificate
// ═══════════════════════════════════════════════════════════════════════════════

CertificateInfo
AuditorModule::validateCertificate(const QSslCertificate &cert) noexcept
{
    CertificateInfo ci;

    ci.subject   = cert.subjectInfo(QSslCertificate::CommonName).value(0);
    ci.issuer    = cert.issuerInfo(QSslCertificate::CommonName).value(0);
    ci.validFrom = cert.effectiveDate();
    ci.validTo   = cert.expiryDate();

    // ── Selbstsigniert? ───────────────────────────────────────────────────
    ci.isSelfSigned = (ci.subject == ci.issuer);

    // ── Abgelaufen? ───────────────────────────────────────────────────────
    QDateTime const now = QDateTime::currentDateTimeUtc();
    ci.isExpired = (ci.validTo.isValid() && ci.validTo < now);

    // ── SANs (SubjectAltNames) ────────────────────────────────────────────
    QStringList const altNames = cert.subjectAlternativeNames().values(QSsl::DnsEntry);
    ci.subjectAltNames = altNames;

    // ── Gesamt-Validierung ─────────────────────────────────────────────────
    ci.isValid = (!ci.isExpired && !ci.subject.isEmpty());

    if (ci.isExpired) {
        ci.errorMessage = QStringLiteral("Certificate expired on %1")
                              .arg(ci.validTo.toString(Qt::ISODate));
    }

    return ci;
}

} // namespace IPView::Auditor
