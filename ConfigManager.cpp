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
    {
        int const tab = loadLastTab(-1);
        if (tab < 0 || tab > 7) {
            IPView::Logger::warn("ConfigManager: Clamping invalid lastTab={} to 0", tab);
            saveLastTab(0);
            valid = false;
        }
    }

    // Network/SelectedApiIndex — max 4 APIs vorhanden
    {
        int const idx = loadApiIndex(-1);
        if (idx < 0 || idx > 3) {
            IPView::Logger::warn("ConfigManager: Clamping invalid apiIndex={} to 0", idx);
            saveApiIndex(0);
            valid = false;
        }
    }

    // Telemetry/IntervalMs — mindestens 500 ms
    {
        int const interval = loadTelemetryInterval(0);
        int const clamped  = std::clamp(interval, 500, 3600000);
        if (interval != clamped) {
            IPView::Logger::warn("ConfigManager: Clamping telemetryInterval={} to {}", interval, clamped);
            saveTelemetryInterval(clamped);
            valid = false;
        }
    }

    // Telemetry/AggregationWindowSec — mindestens 60 s
    {
        int const win = loadTelemetryWindowSize(0);
        int const clamped = std::clamp(win, 60, 86400);
        if (win != clamped) {
            IPView::Logger::warn("ConfigManager: Clamping telemetryWindowSize={} to {}", win, clamped);
            saveTelemetryWindowSize(clamped);
            valid = false;
        }
    }

    // Scanner/TimeoutMs — mindestens 50 ms
    {
        int const timeout = loadScanTimeout(0);
        int const clamped = std::clamp(timeout, 50, 30000);
        if (timeout != clamped) {
            IPView::Logger::warn("ConfigManager: Clamping scanTimeout={} to {}", timeout, clamped);
            saveScanTimeout(clamped);
            valid = false;
        }
    }

    // Scanner/MaxConcurrent — 1..1000
    {
        int const conc = loadScanConcurrency(0);
        int const clamped = std::clamp(conc, 1, 1000);
        if (conc != clamped) {
            IPView::Logger::warn("ConfigManager: Clamping scanConcurrency={} to {}", conc, clamped);
            saveScanConcurrency(clamped);
            valid = false;
        }
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

} // namespace IPView::Config
