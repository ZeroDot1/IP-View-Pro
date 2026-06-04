// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — Error.hpp
//  Single source of truth for the error model used across the project.
//
//  Prior to v2.15.0 each module invented its own error type: some returned
//  std::expected<T, QString>, some std::expected<T, std::string>, some
//  packed a QString into a member, and some used raw bool + out-parameter.
//  The new IPView::Error / IPView::ErrorInfo / IPView::Result<T> triplet
//  replaces all of them so callers can branch on a single enum code and
//  the GUI layer can show a uniform dialog.
//
//  Migration is gradual: existing std::expected<T, QString> signatures keep
//  compiling because Result<T> is just a type alias. New code should write
//  Result<T> directly; legacy QString errors get bridged by the
//  unexpectedFrom helper until each module is refactored.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef IPVIEW_ERROR_HPP
#define IPVIEW_ERROR_HPP

#include <cstdint>
#include <expected>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

#include <QString>

namespace IPView {

// ── Stable, library-wide error code enum ────────────────────────────────
enum class Error : std::uint8_t {
    None                  = 0,

    // Network
    NetworkTimeout        = 10,
    NetworkUnreachable    = 11,
    NetworkRefused        = 12,
    DnsFailure            = 13,
    TlsFailure            = 14,
    HttpError             = 15,
    TransferError         = 16,

    // Input
    InvalidInput          = 20,
    InvalidUrl            = 21,
    InvalidIp             = 22,
    InvalidPort           = 23,
    InvalidHostname       = 24,
    InvalidJson           = 25,

    // File / IO
    FileNotFound          = 30,
    FileReadError         = 31,
    FileWriteError        = 32,
    ParseError            = 33,

    // Storage
    DatabaseError         = 40,
    DatabaseBusy          = 41,
    DatabaseCorrupt       = 42,

    // Process
    NotInitialized        = 50,
    AlreadyRunning        = 51,
    ResourceExhausted     = 52,
    PermissionDenied      = 53,
    ShellMetachars        = 54,
    CommandNotFound       = 55,
    CommandFailed         = 56,

    // Fallback
    Unknown               = 255,
};

[[nodiscard]]
constexpr std::string_view errorCodeName(Error e) noexcept
{
    switch (e) {
    case Error::None:               return "None";
    case Error::NetworkTimeout:     return "NetworkTimeout";
    case Error::NetworkUnreachable: return "NetworkUnreachable";
    case Error::NetworkRefused:     return "NetworkRefused";
    case Error::DnsFailure:         return "DnsFailure";
    case Error::TlsFailure:         return "TlsFailure";
    case Error::HttpError:          return "HttpError";
    case Error::TransferError:      return "TransferError";
    case Error::InvalidInput:       return "InvalidInput";
    case Error::InvalidUrl:         return "InvalidUrl";
    case Error::InvalidIp:          return "InvalidIp";
    case Error::InvalidPort:        return "InvalidPort";
    case Error::InvalidHostname:    return "InvalidHostname";
    case Error::InvalidJson:        return "InvalidJson";
    case Error::FileNotFound:       return "FileNotFound";
    case Error::FileReadError:      return "FileReadError";
    case Error::FileWriteError:     return "FileWriteError";
    case Error::ParseError:         return "ParseError";
    case Error::DatabaseError:      return "DatabaseError";
    case Error::DatabaseBusy:       return "DatabaseBusy";
    case Error::DatabaseCorrupt:    return "DatabaseCorrupt";
    case Error::NotInitialized:     return "NotInitialized";
    case Error::AlreadyRunning:     return "AlreadyRunning";
    case Error::ResourceExhausted:  return "ResourceExhausted";
    case Error::PermissionDenied:   return "PermissionDenied";
    case Error::ShellMetachars:     return "ShellMetachars";
    case Error::CommandNotFound:    return "CommandNotFound";
    case Error::CommandFailed:      return "CommandFailed";
    case Error::Unknown:            return "Unknown";
    }
    return "Unknown";
}

// ── ErrorInfo: the payload stored inside std::expected ────────────────────
struct ErrorInfo {
    Error                  code{Error::None};
    std::string            message{};
    std::source_location   loc{};

    constexpr ErrorInfo() noexcept = default;

    constexpr ErrorInfo(Error c,
                        std::string_view m = {},
                        std::source_location l = std::source_location::current()) noexcept
        : code{c}, message{m}, loc{l}
    {}

    [[nodiscard]] std::string format() const
    {
        return std::string{loc.file_name()} + ":" + std::to_string(loc.line())
             + " [" + std::string{errorCodeName(code)} + "] " + message;
    }

    [[nodiscard]] QString toQString() const noexcept
    {
        return QString::fromStdString(format());
    }
};

// ── Result<T> — the canonical success-or-error return type ───────────────
template <typename T>
using Result = std::expected<T, ErrorInfo>;

// ── Bridge helpers for migrating legacy QString errors without breaking
//    call sites ─────────────────────────────────────────────────────────
template <typename T = void>
inline auto unexpected(Error code,
                       std::string_view message = {},
                       std::source_location loc = std::source_location::current())
{
    if constexpr (std::is_void_v<T>) {
        return std::unexpected(ErrorInfo{code, message, loc});
    } else {
        return std::unexpected<Result<T>>(
            std::in_place, ErrorInfo{code, message, loc});
    }
}

inline auto unexpectedFromQString(
    QString legacyMessage,
    Error fallbackCode = Error::Unknown,
    std::source_location loc = std::source_location::current())
{
    return std::unexpected(
        ErrorInfo{fallbackCode, legacyMessage.toStdString(), loc});
}

} // namespace IPView

#endif // IPVIEW_ERROR_HPP
