# Changelog — IPView Pro

All notable changes to this project are documented here.

## [2.8.0] — 2026-05-19

### Added
- **`ScannerTab`** — GUI-Tab für `ScannerModule`: Quick-Scan (28 Ports), benutzerdefinierte
  Port-Bereiche, Ergebnistabelle mit Port/State/Service/Latenz. Wird als eigener Tab
  `Port Scanner` unter dem Icon `compass.svg` angezeigt.
- **`TelemetryTab`** — GUI-Tab für `TelemetryModule`: Echtzeit-Monitoring von Netzwerk-
  interfaces. Zeigt Gesamt-Download/Upload (Karten oben) und pro-Interface-Tabelle mit
  RX/TX-Raten, Paketen und Fehlern. Wird als eigener Tab `Telemetry` unter `graph.svg`
  angezeigt.
- **`HistoryTab`-Persistenz:** Lädt IP-Historie beim Start aus SQLite (`DatabaseModule::getHistory()`).
  Neue `SQLite persist`-Checkbox steuert, ob neue Einträge in die DB geschrieben werden.
  `Clear` löscht auch die SQLite-Tabelle.
- **`DashboardView::setApiIndex()`** — Neue Methode zum programmatischen Setzen des
  API-ComboBox-Index (wichtig für Config-Wiederherstellung).
- **SVG-Icons:** `svgs/compass.svg` (Scanner), `svgs/graph.svg` (Telemetry) erstellt
  und in `resources.qrc` registriert.
- **Modular architecture:** UI components extracted into separate classes.
- **`DashboardView`** — Standalone overview widget (API selection, IP card, data table,
  flag display, copy/export buttons). Replaces the inline overview tab in MainWindow.
  Communicates via signals (`refreshRequested`, `apiChanged`, `ipv6Toggled`,
  `autoRefreshToggled`, `copyAllRequested`, `exportJsonRequested`).
- **`DatabaseModule`** — SQLite persistence layer (singleton pattern, thread-safe via
  `QMutex`). Stores IP history and telemetry data. Two tables: `ip_history` and
  `telemetry`. WAL journal mode for concurrent access.
- **`TelemetryModule`** — Real-time network telemetry via `/proc/net/dev` monitoring.
  Background `QTimer`-driven polling (2s interval). Parses RX/TX bytes, packets, and
  errors per interface. Calculates real-time transfer speeds (bytes/s). Filters out
  loopback, docker, and virtual interfaces. Emits `telemetryUpdated` signal.
- **`ScannerModule`** — Asynchronous port scanner using `QTcpSocket` (non-blocking).
  Batch processing with configurable concurrency (default 100 parallel connections).
  Supports port lists and ranges. Service name resolution for 28+ well-known ports.
  Emits `portFound`, `scanCompleted`, `scanCancelled`, and `scanProgress` signals.
- **`ConfigManager`** — Per-user configuration via `QSettings` (INI-Format).
  Speichert in `~/.config/IPView/IPView.conf` (XDG Base Directory Specification).
  Jeder Benutzer hat seine eigene Konfigurationsdatei mit Fenstergeometrie,
  API-Auswahl, IPv6-Modus, Auto-Refresh-Einstellungen.
  Methoden: `saveWindowGeometry/State`, `saveLastTab`, `saveApiIndex`,
  `saveIPv6Mode`, `saveAutoRefresh`, `saveNetworkSettings` uvm.
- **`main.cpp`:** Database initialized at startup via `DatabaseModule::init()`.
  ConfigManager initialized at startup via `IPView::Config::Manager::initialize()`.

### Changed
- **MainWindow refactored:** Now uses `DashboardView` as the Overview tab instead of
  inline widget creation. All overview-related widgets (apiCombo, ipLabel, ipTable,
  flagLabel, buttons) moved to `DashboardView`. Data display delegated to
  `DashboardView::updateDisplay()`. Flag loading uses `flagLabelWidget()` getter.
- **Config persistence:** MainWindow speichert/lädt Fenstergeometrie, API-Index,
  IPv6-Modus und Auto-Refresh automatisch via `saveSettings()`/`loadSettings()`.
  Die Config liegt pro Benutzer in `~/.config/IPView/IPView.conf`.
- **Database path:** `DatabaseModule::defaultDbPath()` verwendet jetzt
  `QStandardPaths::AppConfigLocation` → `~/.config/IPView/ipview_history.db`
  (vorher: `~/.local/share/`). Config und Datenbank liegen nun im selben
  Verzeichnis für jeden Benutzer.
- **`DashboardView`:** Neue Methoden `setIPv6Mode(bool)` und `setAutoRefresh(bool)`
  zum Wiederherstellen der gespeicherten Einstellungen.
- **`NetworkManager`:** Neue Methode `getSelectedApiIndex()` für Config-Persistence.
- **MainWindow-Tabs:** `ScannerTab` und `TelemetryTab` als neue Tabs registriert.
  `ToolsTab` bleibt als Ping/Traceroute-Tab erhalten, Scanner bekommt eigenen Tab.
- **`HistoryTab` mit SQLite-Persistenz:** Beim Start wird `loadPersistedHistory()`
  aufgerufen, um gespeicherte Einträge aus `ipview_history.db` zu laden.
  Neue Methoden: `loadPersistedHistory()`, persistCheckBox.
- **Build-System:** `CMakeLists.txt` um `ScannerTab.cpp` und `TelemetryTab.cpp` ergänzt.
  `resources.qrc` um `compass.svg` und `graph.svg` erweitert.
- **Version bumped:** `2.7.1` → `2.8.0` (`CMakeLists.txt`).
- **CMake updated:** Added `DashboardView`, `TelemetryModule`, `DatabaseModule`,
  `ScannerModule`, `ConfigManager` source files. Added `Qt6::Sql` dependency.
- **`Theme.h`:** Comments updated to reflect v2.8.0.

### Security
- **`ScannerModule`** validates target IP before scanning (empty/hostname check).
  No shell metacharacters accepted — follows existing `SecurityUtil` injection
  prevention guidelines.
- **`TelemetryModule`** parses `/proc/net/dev` with strict format validation using
  `std::from_chars` — no unsafe C-string parsing.
- **`DatabaseModule`** uses parameterized queries (`QSqlQuery::prepare` + `addBindValue`)
  — no SQL injection possible.

### Maintenance
- Removed dead code from `MainWindow`: inline `FIELD_NAMES`/`FIELD_KEYS` arrays,
  `updateDataDisplay()` method, copy/export implementation moved to signal handlers.
- All new modules use C++26 features: `std::expected`, `std::from_chars`,
  `std::optional`, `[[nodiscard]]`, `noexcept`, structured bindings, constexpr arrays.

---

## [2.7.1] — 2026-05-19

### Changed
- **C++ standard:** Synchronized `CMakeLists.txt` to use `CMAKE_CXX_STANDARD 26`
  (was `23` — the codebase already used C++26 features; the build system now
  matches the documented standard).
- **Refactored `DataNormalizer`:** Converted from a class with deleted constructor
  and all-static methods to an idiomatic C++ `namespace`. Internal helpers reside
  in a `DataNormalizer::detail` sub-namespace. Public API (`normalize()`,
  `formatJsonHtml()`) remains unchanged.

### Security
- **Confirmed input validation:** Both `TracerouteTab` and `ToolsTab` already
  call `isValidNetworkTarget()` from `SecurityUtil.h` before passing user input
  to `QProcess`. No changes required.

### Maintenance
- **Verified Qt parent hierarchy:** All widgets created with `new` are correctly
  parented — either explicitly (`new QWidget(this)`) or implicitly via Qt's
  layout/tab-widget reparenting. No memory leaks detected.

### Added
- **`CHANGELOG.md`:** Version history file added.

---

## [2.7.0] — 2026-03-01

Initial public release.
- C++26 / Qt 6.11 codebase
- Multi-API geolocation (12+ Geo-IP APIs with failover)
- Speedtest, Whois/RDAP, Ping, iPerf3, Traceroute tools
- IP change history, system tray, export JSON
- Full security hardening (input validation, SSL enforcement,
  compiler hardening, ASan/UBSan support)
