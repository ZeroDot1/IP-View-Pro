// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — ConfigManager.h
//  C++26: [[nodiscard]], noexcept, const-correctness
//  Per-User-Konfiguration via QSettings (INI-Format).
//  Speichert in ~/.config/IPView/IPView.conf (XDG Base Directory).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QSettings>
#include <QString>
#include <QByteArray>
#include <QPoint>
#include <QSize>

#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════════
//  ConfigManager — zentrale Konfigurationsverwaltung
//
//  Verwendet QSettings mit INI-Format → ~/.config/IPView/IPView.conf
//  Jeder Benutzer hat seine eigene Konfigurationsdatei.
//  Thread-safe durch QSettings-internes Locking.
//
//  XDG Base Directory Specification:
//    Linux:   ~/.config/IPView/IPView.conf
//    Windows: %APPDATA%/IPView/IPView.ini
//    macOS:   ~/Library/Preferences/com.IPView.plist
// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Config {

// ── Konfigurationsschlüssel (als String-Konstanten) ─────────────────────────
namespace Key {
    inline constexpr auto WINDOW_GEOMETRY   = "Window/Geometry";
    inline constexpr auto WINDOW_STATE      = "Window/State";
    inline constexpr auto LAST_TAB          = "Window/LastTab";

    inline constexpr auto API_INDEX         = "Network/SelectedApiIndex";
    inline constexpr auto IPV6_MODE         = "Network/IPv6Mode";
    inline constexpr auto AUTO_REFRESH      = "Network/AutoRefresh";

    inline constexpr auto DB_PATH           = "Database/CustomPath";
    inline constexpr auto TELEMETRY_INTERVAL = "Telemetry/IntervalMs";
    inline constexpr auto SCAN_TIMEOUT      = "Scanner/TimeoutMs";
    inline constexpr auto SCAN_CONCURRENCY  = "Scanner/MaxConcurrent";
}

// ═══════════════════════════════════════════════════════════════════════════════
class Manager
{
public:
    // ── Singleton-Zugriff ──────────────────────────────────────────────────
    [[nodiscard]] static Manager& instance() noexcept;

    // ── Lebenszyklus ───────────────────────────────────────────────────────
    static void initialize() noexcept;
    static void sync()       noexcept;

    // ── Window ─────────────────────────────────────────────────────────────
    static void saveWindowGeometry(const QByteArray &geometry) noexcept;
    static void saveWindowState(const QByteArray &state) noexcept;
    [[nodiscard]] static QByteArray loadWindowGeometry() noexcept;
    [[nodiscard]] static QByteArray loadWindowState() noexcept;

    static void saveLastTab(int index) noexcept;
    [[nodiscard]] static int loadLastTab(int defaultIndex = 0) noexcept;

    // ── Network ────────────────────────────────────────────────────────────
    static void saveApiIndex(int index) noexcept;
    [[nodiscard]] static int loadApiIndex(int defaultIndex = 0) noexcept;

    static void saveIPv6Mode(bool enabled) noexcept;
    [[nodiscard]] static bool loadIPv6Mode(bool defaultEnabled = false) noexcept;

    static void saveAutoRefresh(bool enabled) noexcept;
    [[nodiscard]] static bool loadAutoRefresh(bool defaultEnabled = false) noexcept;

    // ── Database ──────────────────────────────────────────────────────────
    static void saveCustomDbPath(const QString &path) noexcept;
    [[nodiscard]] static QString loadCustomDbPath() noexcept;

    // ── Telemetry ─────────────────────────────────────────────────────────
    static void saveTelemetryInterval(int ms) noexcept;
    [[nodiscard]] static int loadTelemetryInterval(int defaultMs = 2000) noexcept;

    // ── Scanner ────────────────────────────────────────────────────────────
    static void saveScanTimeout(int ms) noexcept;
    [[nodiscard]] static int loadScanTimeout(int defaultMs = 500) noexcept;

    static void saveScanConcurrency(int count) noexcept;
    [[nodiscard]] static int loadScanConcurrency(int defaultCount = 100) noexcept;

    // ── Convenience: Alle Einstellungen auf einmal speichern ──────────────
    static void saveNetworkSettings(int apiIndex, bool ipv6, bool autoRefresh) noexcept;

private:
    Manager() noexcept;
    ~Manager() = default;

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    // ── Interner QSettings-Zugriff ─────────────────────────────────────────
    [[nodiscard]] static QSettings& settings() noexcept;

    // ── State ──────────────────────────────────────────────────────────────
    static QSettings sSettings;
    static bool      sInitialized;
};

} // namespace IPView::Config

#endif // CONFIGMANAGER_H
