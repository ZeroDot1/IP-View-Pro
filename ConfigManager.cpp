// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — ConfigManager.cpp
//  C++26: [[nodiscard]], noexcept, const-correctness
//  Per-user configuration via QSettings (INI format).
//  Storage location (XDG Base Directory):
//    Linux:   ~/.config/IPView/IPView.conf
//    Windows: ~/AppData/Roaming/IPView/IPView.ini
//    macOS:   ~/Library/Preferences/com.IPView.plist
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "ConfigManager.h"
#include "Logger.h"
#include "Timeouts.hpp"

#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>

#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════════════
//  Static members
// ═══════════════════════════════════════════════════════════════════════════════

QSettings IPView::Config::Manager::sSettings(
    QStringLiteral("IPView"),   // Organisation
    QStringLiteral("IPView")    // Application
);
bool IPView::Config::Manager::sInitialized{false};

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Config {

// ═══════════════════════════════════════════════════════════════════════════════
//  Singleton & Init
// ═══════════════════════════════════════════════════════════════════════════════

Manager& Manager::instance() noexcept
{
    static Manager inst;
    return inst;
}

Manager::Manager() noexcept
{
    // QSettings automatically uses the INI format
    // when no format is specified, Qt uses the native format
    // (Windows: Registry, Linux: INI file).
    // We force INI for platform-independent behavior.
    sSettings.setFallbacksEnabled(false);
}

void Manager::initialize() noexcept
{
    if (sInitialized) return;

    // Ensure the config directory exists
    QString const configDir = QFileInfo(sSettings.fileName()).absolutePath();
    QDir().mkpath(configDir);

    IPView::Logger::info("ConfigManager: Initialized ({})",
          sSettings.fileName().toStdString());

    sInitialized = true;

    // Automatic validation on startup
    static_cast<void>(validateAll());
}

void Manager::sync() noexcept
{
    sSettings.sync();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Internal access
// ═══════════════════════════════════════════════════════════════════════════════

QSettings& Manager::settings() noexcept
{
    return sSettings;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Window
// ═══════════════════════════════════════════════════════════════════════════════

void Manager::saveWindowGeometry(const QByteArray &geometry) noexcept
{
    settings().setValue(QLatin1StringView(Key::WINDOW_GEOMETRY), geometry);
}

void Manager::saveWindowState(const QByteArray &state) noexcept
{
    settings().setValue(QLatin1StringView(Key::WINDOW_STATE), state);
}

QByteArray Manager::loadWindowGeometry() noexcept
{
    return settings().value(QLatin1StringView(Key::WINDOW_GEOMETRY)).toByteArray();
}

QByteArray Manager::loadWindowState() noexcept
{
    return settings().value(QLatin1StringView(Key::WINDOW_STATE)).toByteArray();
}

void Manager::saveLastTab(int index) noexcept
{
    settings().setValue(QLatin1StringView(Key::LAST_TAB), index);
}

int Manager::loadLastTab(int defaultIndex) noexcept
{
    return settings().value(QLatin1StringView(Key::LAST_TAB), defaultIndex).toInt();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Network
// ═══════════════════════════════════════════════════════════════════════════════

void Manager::saveApiIndex(int index) noexcept
{
    settings().setValue(QLatin1StringView(Key::API_INDEX), index);
}

int Manager::loadApiIndex(int defaultIndex) noexcept
{
    return settings().value(QLatin1StringView(Key::API_INDEX), defaultIndex).toInt();
}

void Manager::saveIPv6Mode(bool enabled) noexcept
{
    settings().setValue(QLatin1StringView(Key::IPV6_MODE), enabled);
}

bool Manager::loadIPv6Mode(bool defaultEnabled) noexcept
{
    return settings().value(QLatin1StringView(Key::IPV6_MODE), defaultEnabled).toBool();
}

void Manager::saveAutoRefresh(bool enabled) noexcept
{
    settings().setValue(QLatin1StringView(Key::AUTO_REFRESH), enabled);
}

bool Manager::loadAutoRefresh(bool defaultEnabled) noexcept
{
    return settings().value(QLatin1StringView(Key::AUTO_REFRESH), defaultEnabled).toBool();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Database
// ═══════════════════════════════════════════════════════════════════════════════

void Manager::saveCustomDbPath(const QString &path) noexcept
{
    settings().setValue(QLatin1StringView(Key::DB_PATH), path);
}

QString Manager::loadCustomDbPath() noexcept
{
    return settings().value(QLatin1StringView(Key::DB_PATH)).toString();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Telemetry
// ═══════════════════════════════════════════════════════════════════════════════

void Manager::saveTelemetryInterval(int ms) noexcept
{
    settings().setValue(QLatin1StringView(Key::TELEMETRY_INTERVAL), ms);
}

int Manager::loadTelemetryInterval(int defaultMs) noexcept
{
    return settings().value(QLatin1StringView(Key::TELEMETRY_INTERVAL), defaultMs).toInt();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Telemetry — Auto-Start & Window Size
// ═══════════════════════════════════════════════════════════════════════════════

void Manager::saveTelemetryAutoStart(bool enabled) noexcept
{
    settings().setValue(QLatin1StringView(Key::TELEMETRY_AUTO_START), enabled);
}

bool Manager::loadTelemetryAutoStart(bool defaultEnabled) noexcept
{
    return settings().value(QLatin1StringView(Key::TELEMETRY_AUTO_START), defaultEnabled).toBool();
}

void Manager::saveTelemetryWindowSize(int seconds) noexcept
{
    settings().setValue(QLatin1StringView(Key::TELEMETRY_WINDOW_SIZE), seconds);
}

int Manager::loadTelemetryWindowSize(int defaultSeconds) noexcept
{
    return settings().value(QLatin1StringView(Key::TELEMETRY_WINDOW_SIZE), defaultSeconds).toInt();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Scanner
// ═══════════════════════════════════════════════════════════════════════════════

void Manager::saveScanTimeout(int ms) noexcept
{
    settings().setValue(QLatin1StringView(Key::SCAN_TIMEOUT), ms);
}

int Manager::loadScanTimeout(int defaultMs) noexcept
{
    return settings().value(QLatin1StringView(Key::SCAN_TIMEOUT), defaultMs).toInt();
}

void Manager::saveScanConcurrency(int count) noexcept
{
    settings().setValue(QLatin1StringView(Key::SCAN_CONCURRENCY), count);
}

int Manager::loadScanConcurrency(int defaultCount) noexcept
{
    return settings().value(QLatin1StringView(Key::SCAN_CONCURRENCY), defaultCount).toInt();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Validation (Item 15)
// ═══════════════════════════════════════════════════════════════════════════════

bool Manager::validateAll() noexcept
{
    if (!sInitialized) {
        IPView::Logger::warn("ConfigManager: validateAll() called before initialize()");
        return false;
    }

    bool valid = true;

    // Window/LastTab — must be within tab range (0..7)
    if (loadClampedInt(QLatin1StringView(Key::LAST_TAB), 0, 0, 7) !=
        loadLastTab()) {
        valid = false;
    }

    // Network/SelectedApiIndex — max 4 APIs vorhanden
    if (loadClampedInt(QLatin1StringView(Key::API_INDEX), 0, 0, 3) !=
        loadApiIndex()) {
        valid = false;
    }

    // Telemetry/IntervalMs — mindestens 500 ms
    if (loadClampedInt(QLatin1StringView(Key::TELEMETRY_INTERVAL),
                       2000, 500, 3'600'000) != loadTelemetryInterval()) {
        valid = false;
    }

    // Telemetry/AggregationWindowSec — mindestens 60 s
    if (loadClampedInt(QLatin1StringView(Key::TELEMETRY_WINDOW_SIZE),
                       60, 60, 86'400) != loadTelemetryWindowSize()) {
        valid = false;
    }

    // Scanner/TimeoutMs — mindestens 50 ms
    if (loadClampedInt(QLatin1StringView(Key::SCAN_TIMEOUT),
                       1000, 50, 30'000) != loadScanTimeout()) {
        valid = false;
    }

    // Scanner/MaxConcurrent — 1..1000
    if (loadClampedInt(QLatin1StringView(Key::SCAN_CONCURRENCY),
                       100, 1, 1000) != loadScanConcurrency()) {
        valid = false;
    }

    if (valid) {
        IPView::Logger::info("ConfigManager: Validation passed — all settings in range");
    }

    sync();
    return valid;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Convenience
// ═══════════════════════════════════════════════════════════════════════════════

void Manager::saveNetworkSettings(int apiIndex, bool ipv6, bool autoRefresh) noexcept
{
    saveApiIndex(apiIndex);
    saveIPv6Mode(ipv6);
    saveAutoRefresh(autoRefresh);
    sync();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Range-validated getter (Phase 4-B)
//
//  Reads a numeric setting, falling back to defaultValue if the
//  key is missing or unparseable, and clamping the result into
//  [lo, hi]. When the stored value lands outside the allowed
//  range the clamped value is written back so the .ini file
//  heals itself on the next sync() — useful after a code
//  change that tightens a range (e.g. telemetry interval
//  dropping from 2000 ms to 500 ms minimum).
// ═══════════════════════════════════════════════════════════════════════════════

int Manager::loadClampedInt(QLatin1StringView key,
                            int defaultValue,
                            int lo,
                            int hi) noexcept
{
    const QVariant raw = settings().value(key);
    if (!raw.isValid() || !raw.canConvert<int>()) {
        return defaultValue;
    }
    const int stored = raw.toInt();
    if (stored < lo || stored > hi) {
        const int clamped = std::clamp(stored, lo, hi);
        IPView::Logger::warn(
            "ConfigManager: clamping {} from {} to {} (allowed [{}..{}])",
            std::string_view(key.data(), static_cast<std::size_t>(key.size())),
            stored, clamped, lo, hi);
        settings().setValue(key, clamped);
        return clamped;
    }
    return stored;
}

} // namespace IPView::Config
