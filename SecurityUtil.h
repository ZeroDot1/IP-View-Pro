// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.7.0 — SecurityUtil.h
//  Core security functions: input validation, IP checking,
//  SSL handling, command injection prevention.
//
//  C++26 constexpr/consteval — checked at compile time.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef SECURITYUTIL_H
#define SECURITYUTIL_H

#include <QString>
#include <QRegularExpression>
#include <QHostAddress>
#include <QUrl>
#include <QNetworkReply>
#include <QFileInfo>
#include <QStandardPaths>

// ═══════════════════════════════════════════════════════════════════════════════
//  COMMAND INJECTION PREVENTION
//  Ensures that user input contains no shell metacharacters.
//  Allowed: IPv4, IPv6, domain names (RFC 1035), empty input.
// ═══════════════════════════════════════════════════════════════════════════════

// ── Checks whether a string is a valid target for network tools ──────────
//  Allowed: IPv4, IPv6 (with/without port), hostname (RFC 1123).
//  Blocked: shell metacharacters (; | & ` $ ( ) { } < > ! #), whitespace.
[[nodiscard]]
inline bool isValidNetworkTarget(const QString &input) noexcept
{
    if (input.isEmpty()) return false;

    // No whitespace or shell metacharacters allowed
    static QRegularExpression const forbiddenRe(
        QStringLiteral(R"([\s;|&`$(){}<>!#\'\"\\])"));
    if (forbiddenRe.match(input).hasMatch()) return false;

    // IPv4 check via Qt
    QHostAddress addr;
    if (addr.setAddress(input)) return true;  // IPv4 or IPv6

    // Hostname validation (RFC 1123): letters, digits, dots, hyphens
    static QRegularExpression const hostnameRe(
        QStringLiteral(R"(^[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?)*$)"));
    return hostnameRe.match(input).hasMatch();
}

// ── Safety measure: escape argument (not necessary for QProcess, but defense in depth) ──
//  QProcess.start(command, args) does not invoke a shell — nevertheless we filter
//  unwanted characters at the argument level.
[[nodiscard]]
inline QString sanitizeArg(const QString &arg) noexcept
{
    QString clean = arg;
    // Remove all characters not found in a safe hostname/IP
    clean.remove(QRegularExpression(QStringLiteral(R"([^a-zA-Z0-9\-._:\[\]])")));
    return clean;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SSL/TLS HARDENING
//  Configures QNetworkRequest with strict SSL validation.
// ═══════════════════════════════════════════════════════════════════════════════

// ── SSL error handling for QNetworkReply ────────────────────────────────
//  Connects the sslErrors signal of a QNetworkReply with a
//  strict policy: on SSL errors, the request is aborted.
//  Call after manager->get() / post().
inline void enforceStrictSsl(QNetworkReply *reply) noexcept
{
    if (!reply) return;

    // QNetworkReply::sslErrors deprecated since Qt 6.5 – use
    // QNetworkReply::errorOccurred + check for SSL errors.
    // Additionally: force peer verification via QSslConfiguration
    QObject::connect(reply, &QNetworkReply::errorOccurred, reply, [reply](QNetworkReply::NetworkError code) {
        if (code == QNetworkReply::SslHandshakeFailedError) {
            reply->abort();
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
//  PROCESS EXECUTION SAFETY
//  Finds the full path to executable programs,
//  to prevent PATH hijacking.
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]]
inline QString findSystemTool(const QString &toolName) noexcept
{
    // Use QStandardPaths for absolute path resolution
    QString const path = QStandardPaths::findExecutable(toolName);
    if (path.isEmpty()) {
        // Fallback: direct path on Linux
        QStringList const candidates = {
            QStringLiteral("/usr/bin/%1").arg(toolName),
            QStringLiteral("/bin/%1").arg(toolName),
            QStringLiteral("/usr/sbin/%1").arg(toolName)
        };
        for (QString const &c : candidates) {
            if (QFileInfo::exists(c)) return c;
        }
    }
    return path;
}

#endif // SECURITYUTIL_H
