# Changelog — IPView Pro

All notable changes to this project are documented here.

## [2.8.1] — 2026-05-19

### Changed
- **Theme.h — Tab design overhauled:** `PADDING_TAB` reduced from `12px 25px` to `6px 12px`,
  tab font-size set to `11px`. All 8 tabs now fit in the tab bar without scrolling.
  Gradient hover/selected effects with `cubic-bezier` transition.
- **MainWindow — Tab bar configuration:** `setUsesScrollButtons(false)` and
  `setExpanding(true)` on QTabBar, icon size set to `14×14`.
- **New constant:** `PADDING_TAB_L = "6px 18px"` for optional wider tabs.

### Visual
- Tab hover: Gradient from `C_BG_HOVER` → `C_BG_ELEVATED` for depth effect.
- Tab selected: Gradient from `C_ACCENT` → `C_ACCENT_ACT` for bright accent color.
- Tab bar: `qproperty-drawBase: false` removes the default underline line.
- `min-width: 0` on tabs allows compact layout without scroll buttons.

---

## [2.8.0] — 2026-05-19

### Added
- **`ScannerTab`** — GUI tab for `ScannerModule`: Quick scan (28 ports), custom
  port ranges, result table with port/state/service/latency. Displayed as its own
  tab `Port Scanner` under the `compass.svg` icon.
- **`TelemetryTab`** — GUI tab for `TelemetryModule`: Real-time monitoring of network
  interfaces. Shows total download/upload (cards at top) and per-interface table with
  RX/TX rates, packets, and errors. Displayed as its own tab `Telemetry` under `graph.svg`.
- **`HistoryTab` persistence:** Loads IP history from SQLite on startup (`DatabaseModule::getHistory()`).
  New `SQLite persist` checkbox controls whether new entries are written to the DB.
  `Clear` also deletes the SQLite table.
- **`DashboardView::setApiIndex()`** — New method to programmatically set the
  API combo box index (important for config restoration).
- **SVG icons:** `svgs/compass.svg` (scanner), `svgs/graph.svg` (telemetry) created
  and registered in `resources.qrc`.
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
- **`ConfigManager`** — Per-user configuration via `QSettings` (INI format).
  Saves to `~/.config/IPView/IPView.conf` (XDG Base Directory Specification).
  Each user has their own configuration file with window geometry, API selection,
  IPv6 mode, auto-refresh settings.
  Methods: `saveWindowGeometry/State`, `saveLastTab`, `saveApiIndex`,
  `saveIPv6Mode`, `saveAutoRefresh`, `saveNetworkSettings`, and many more.
- **`main.cpp`:** Database initialized at startup via `DatabaseModule::init()`.
  ConfigManager initialized at startup via `IPView::Config::Manager::initialize()`.

### Changed
- **MainWindow refactored:** Now uses `DashboardView` as the Overview tab instead of
  inline widget creation. All overview-related widgets (apiCombo, ipLabel, ipTable,
  flagLabel, buttons) moved to `DashboardView`. Data display delegated to
  `DashboardView::updateDisplay()`. Flag loading uses `flagLabelWidget()` getter.
- **Config persistence:** MainWindow saves/loads window geometry, API index,
  IPv6 mode, and auto-refresh automatically via `saveSettings()`/`loadSettings()`.
  The config is stored per user in `~/.config/IPView/IPView.conf`.
- **Database path:** `DatabaseModule::defaultDbPath()` now uses
  `QStandardPaths::AppConfigLocation` → `~/.config/IPView/ipview_history.db`
  (previously: `~/.local/share/`). Config and database now reside in the same
  directory for each user.
- **`DashboardView`:** New methods `setIPv6Mode(bool)` and `setAutoRefresh(bool)`
  for restoring saved settings.
- **`NetworkManager`:** New method `getSelectedApiIndex()` for config persistence.
- **MainWindow tabs:** `ScannerTab` and `TelemetryTab` registered as new tabs.
  `ToolsTab` remains as the Ping/Traceroute tab; scanner gets its own tab.
- **`HistoryTab` with SQLite persistence:** On startup, `loadPersistedHistory()`
  is called to load saved entries from `ipview_history.db`.
  New methods: `loadPersistedHistory()`, persistCheckBox.
- **Build system:** `CMakeLists.txt` supplemented with `ScannerTab.cpp` and `TelemetryTab.cpp`.
  `resources.qrc` extended with `compass.svg` and `graph.svg`.
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
