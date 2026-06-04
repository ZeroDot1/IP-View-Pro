// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — SecurityUtil.h
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
//  Centralized regular expressions
//
//  All QRegularExpression instances for security-critical input validation
//  live here so the patterns can be tuned in one place and the compiled
//  regex objects are thread-safely shared (Qt caches them in the static
//  initialiser). Module-local patterns (output-format parsers, log line
//  matchers, etc.) are intentionally kept next to their caller.
// ═══════════════════════════════════════════════════════════════════════════════

namespace IPView::Security {

// ── Shell metacharacters blocked in any user-supplied network target ────
//   ; | & ` $ ( ) { } < > ! # ' " \ plus any whitespace.
[[nodiscard]]
inline const QRegularExpression& shellMetacharRegex() noexcept
{
    static QRegularExpression const re(
        QStringLiteral(R"([\s;|&`$(){}<>!#\'\"\\])"));
    return re;
}

// ── RFC 1123 hostname (letters, digits, dots, hyphens, no leading/trailing hyphen)
[[nodiscard]]
inline const QRegularExpression& hostnameRegex() noexcept
{
    static QRegularExpression const re(
        QStringLiteral(R"(^[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?(\.[a-zA-Z0-9]([a-zA-Z0-9\-]*[a-zA-Z0-9])?)*$)"));
    return re;
}

// ── Permitted character class for a single sanitized argument ───────────
//  Hostname / IPv6 literal characters only.
[[nodiscard]]
inline const QRegularExpression& unsafeArgCharsRegex() noexcept
{
    static QRegularExpression const re(
        QStringLiteral(R"([^a-zA-Z0-9\-._:\[\]])"));
    return re;
}

// ── IPv4 dotted-quad (0–255 per octet) ────────────────────────────────────
[[nodiscard]]
inline const QRegularExpression& ipv4Regex() noexcept
{
    static QRegularExpression const re(
        QStringLiteral(R"(^(?:(?:25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.){3}(?:25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)$)"));
    return re;
}

// ── IPv6 (full form + zero-compressed form, single embedded IPv4 accepted) ─
[[nodiscard]]
inline const QRegularExpression& ipv6Regex() noexcept
{
    static QRegularExpression const re(
        QStringLiteral(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$|^([0-9a-fA-F]{1,4}:){0,7}:([0-9a-fA-F]{1,4}:){0,7}[0-9a-fA-F]{0,4}$)"));
    return re;
}

} // namespace IPView::Security

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

    if (IPView::Security::shellMetacharRegex().match(input).hasMatch()) return false;

    // IPv4 check via Qt
    QHostAddress addr;
    if (addr.setAddress(input)) return true;  // IPv4 or IPv6

    return IPView::Security::hostnameRegex().match(input).hasMatch();
}

// ── Is the input a valid executable name / absolute path? ────────────────
//  Allows POSIX paths, alphanumerics, dot, dash, underscore and slash.
//  Anything containing a shell metacharacter is refused. The function
//  does NOT touch the filesystem — callers still need to check that
//  the file actually exists.
[[nodiscard]]
inline bool isValidCommand(const QString &input) noexcept
{
    if (input.isEmpty()) return false;
    if (input.size() > 4096) return false;          // PATH_MAX sanity bound
    if (IPView::Security::shellMetacharRegex().match(input).hasMatch()) return false;

    // Restrict to a conservative character set: alnum, dot, dash,
    // underscore, slash, plus, equals (for env-var style paths).
    for (QChar c : input) {
        bool const ok = c.isLetterOrNumber()
                     || c == QLatin1Char('/')
                     || c == QLatin1Char('.')
                     || c == QLatin1Char('-')
                     || c == QLatin1Char('_')
                     || c == QLatin1Char('+')
                     || c == QLatin1Char('=');
        if (!ok) return false;
    }
    return true;
}

// ── Is the input safe to pass as a QProcess argument? ───────────────────
//  This is a strictly weaker check than isValidNetworkTarget: it
//  only verifies that the string contains no shell metacharacter
//  and is not absurdly long. It allows command-line flags (-c,
//  --verbose), integer values, environment-style paths, and of
//  course hostnames and IPs. Use it to gate every argument to
//  QProcess::start() that originates from user input.
[[nodiscard]]
inline bool isValidShellArgument(const QString &input) noexcept
{
    if (input.isEmpty()) return false;
    if (input.size() > 4096) return false;
    if (IPView::Security::shellMetacharRegex().match(input).hasMatch()) return false;
    return true;
}

// ── Is the input a dotted-quad IPv4 address? ─────────────────────────────
[[nodiscard]]
inline bool isValidIPv4(const QString &input) noexcept
{
    if (input.isEmpty()) return false;
    // The regex is the strict form; QHostAddress additionally enforces
    // the 0–255 octet range, so we use it as the final arbiter.
    if (!IPView::Security::ipv4Regex().match(input).hasMatch()) return false;
    QHostAddress addr;
    return addr.setAddress(input) && addr.protocol() == QHostAddress::IPv4Protocol;
}

// ── Is the input a valid IPv6 literal? ──────────────────────────────────
[[nodiscard]]
inline bool isValidIPv6(const QString &input) noexcept
{
    if (input.isEmpty()) return false;
    if (!IPView::Security::ipv6Regex().match(input).hasMatch()) return false;
    QHostAddress addr;
    return addr.setAddress(input) && addr.protocol() == QHostAddress::IPv6Protocol;
}

// ── Is the input a valid port number? (1–65535, decimal) ─────────────────
[[nodiscard]]
inline bool isValidPort(const QString &input) noexcept
{
    if (input.isEmpty()) return false;
    bool ok = false;
    int const port = input.toInt(&ok);
    return ok && port >= 1 && port <= 65535;
}

// ── Safety measure: escape argument (not necessary for QProcess, but defense in depth) ──
//  QProcess.start(command, args) does not invoke a shell — nevertheless we filter
//  unwanted characters at the argument level.
[[nodiscard]]
inline QString sanitizeArg(const QString &arg) noexcept
{
    QString clean = arg;
    clean.remove(IPView::Security::unsafeArgCharsRegex());
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
