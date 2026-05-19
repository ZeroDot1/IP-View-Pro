// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.1 — Logger.h
//  C++26: std::format, std::source_location, consteval
//  Zentrales Logging-System — einheitlich, performant, format-basiert.
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

// ═══════════════════════════════════════════════════════════════════════════════
//  IPView::Logger — statische Logging-Helfer
// ═══════════════════════════════════════════════════════════════════════════════

namespace IPView {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Critical
};

class Logger
{
public:
    Logger() = delete; // Nur statische Nutzung

    /// Formatierten Info-Log ausgeben (ersetzt qInfo).
    template <typename... Args>
    static void info(std::format_string<Args...> fmt,
                     Args&&... args) noexcept
    {
        log(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void warn(std::format_string<Args...> fmt,
                     Args&&... args) noexcept
    {
        log(LogLevel::Warning, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void critical(std::format_string<Args...> fmt,
                         Args&&... args) noexcept
    {
        log(LogLevel::Critical, fmt, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void debug(std::format_string<Args...> fmt,
                      Args&&... args) noexcept
    {
        log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }

private:
    template <typename... Args>
    static void log(LogLevel level,
                    std::format_string<Args...> fmt,
                    Args&&... args) noexcept
    {
        std::string const formatted = std::format(fmt, std::forward<Args>(args)...);
        QString const message = QString::fromStdString(formatted);

        switch (level) {
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
    }
};

} // namespace IPView

#endif // LOGGER_H
