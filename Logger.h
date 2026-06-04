// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — Logger.h
//  C++26: std::format, std::source_location, consteval
//  Centralized logging system — consistent, performant, format-based.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QDebug>

#include <format>       // C++26: std::format
#include <source_location> // C++26: std::source_location
#include <string_view>
#include <string>
#include <utility>
#include <initializer_list>

// ═══════════════════════════════════════════════════════════════════════════════
//  IPView::Logger — statische Logging-Helfer
// ═══════════════════════════════════════════════════════════════════════════════

namespace IPView {

enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Critical
};

// ── Source-location wrapper so call sites can pass a std::source_location
//    without having to write the full type at each call site.
struct LogSite {
    std::string_view     file{};
    std::string_view     function{};
    std::uint_least32_t  line{0};
    std::uint_least32_t  column{0};
    constexpr LogSite(std::source_location loc = std::source_location::current()) noexcept
        : file{loc.file_name()}
        , function{loc.function_name()}
        , line{loc.line()}
        , column{loc.column()}
    {}
};

class Logger
{
public:
    Logger() = delete; // Nur statische Nutzung

    /// Emit formatted trace log (very verbose, default off).
    template <typename... Args>
    static void trace(std::format_string<Args...> fmt,
                      Args&&... args) noexcept
    {
        log(LogLevel::Trace, fmt, LogSite{}, std::forward<Args>(args)...);
    }

    /// Output a formatted info log (replaces qInfo).
    template <typename... Args>
    static void info(std::format_string<Args...> fmt,
                     Args&&... args) noexcept
    {
        log(LogLevel::Info, fmt, LogSite{}, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(std::format_string<Args...> fmt,
                     Args&&... args) noexcept
    {
        log(LogLevel::Warning, fmt, LogSite{}, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void critical(std::format_string<Args...> fmt,
                         Args&&... args) noexcept
    {
        log(LogLevel::Critical, fmt, LogSite{}, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug(std::format_string<Args...> fmt,
                      Args&&... args) noexcept
    {
        log(LogLevel::Debug, fmt, LogSite{}, std::forward<Args>(args)...);
    }

    // ── Structured logging (Phase 3-F — minimal viable version) ───────────
    //  Pass an initializer list of {key, value} pairs that are appended
    //  to the formatted message as "key=value key=value".
    template <typename... Args>
    static void infoKv(std::format_string<Args...> fmt,
                       std::initializer_list<std::pair<std::string_view, std::string_view>> kv,
                       Args&&... args) noexcept
    {
        logKv(LogLevel::Info, fmt, kv, LogSite{}, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warnKv(std::format_string<Args...> fmt,
                       std::initializer_list<std::pair<std::string_view, std::string_view>> kv,
                       Args&&... args) noexcept
    {
        logKv(LogLevel::Warning, fmt, kv, LogSite{}, std::forward<Args>(args)...);
    }

private:
    template <typename... Args>
    static void log(LogLevel level,
                    std::format_string<Args...> fmt,
                    LogSite site,
                    Args&&... args) noexcept
    {
        std::string const formatted = std::format(fmt, std::forward<Args>(args)...);
        QString const message = QString::fromStdString(formatted);

        switch (level) {
        case LogLevel::Trace:
            qDebug().noquote() << QStringLiteral("[trace] ") << message;
            break;
        case LogLevel::Debug:
            qDebug().noquote() << message;
            break;
        case LogLevel::Info:
            qInfo().noquote() << message;
            break;
        case LogLevel::Warning:
            qWarning().noquote() << message;
            break;
        case LogLevel::Critical:
            qCritical().noquote() << message;
            break;
        }
        (void)site;  // logged only at verbose levels, see logKv for use
    }

    template <typename... Args>
    static void logKv(LogLevel level,
                      std::format_string<Args...> fmt,
                      std::initializer_list<std::pair<std::string_view, std::string_view>> kv,
                      LogSite site,
                      Args&&... args) noexcept
    {
        std::string body = std::format(fmt, std::forward<Args>(args)...);
        if (kv.size() != 0) {
            body += "  ";
            for (auto const &kvPair : kv) {
                body += std::format("{}={} ", kvPair.first, kvPair.second);
            }
        }
        (void)site;  // accepted for future "show call site" mode
        QString const message = QString::fromStdString(body);
        switch (level) {
        case LogLevel::Trace:
            qDebug().noquote() << QStringLiteral("[trace] ") << message;
            break;
        case LogLevel::Debug:
        case LogLevel::Info:
            qInfo().noquote() << message;
            break;
        case LogLevel::Warning:
            qWarning().noquote() << message;
            break;
        case LogLevel::Critical:
            qCritical().noquote() << message;
            break;
        }
    }
};

} // namespace IPView

#endif // LOGGER_H
