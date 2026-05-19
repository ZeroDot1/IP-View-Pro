# Changelog — IPView Pro

All notable changes to this project are documented here.

## [2.10.0] — 2026-05-19

### Added

- **Item 46 — TopologyTab (QGraphicsView network topology):** New tab that runs
  `traceroute` to a target host and visualizes each hop as a color-coded node on a
  QGraphicsView canvas. Nodes are colored by latency: green (<10 ms), blue (<50 ms),
  orange (<150 ms), red (≥150 ms or timeout). Interactive zoom (scroll wheel) and
  pan (click-drag). Tooltips show hop IP, hostname, and exact latency. Destination
  node is highlighted in green and enlarged.
  - Hop count label above each node; IP/hostname and latency below.
  - Dashed connecting lines with direction arrow between consecutive hops.
  - Input validation via `isValidNetworkTarget()` and system tool resolution via
    `findSystemTool("traceroute")` for PATH-hijack prevention.
  - New SVG icon: `svgs/topology.svg` (network-graph design with connected nodes).

- **Item 47 — PacketModule (/proc/net connection parser):** Background service that
  parses `/proc/net/tcp`, `/proc/net/tcp6`, `/proc/net/udp`, and `/proc/net/udp6`
  into structured `ConnectionEntry` objects. Converts kernel hex-encoded addresses
  (little-endian byte order) to standard dotted-decimal IPv4 / colon-hex IPv6 format.
  Timer-based polling at 5-second intervals. Emits `connectionsUpdated` signal with
  a full `ConnectionSnapshot` containing timestamped TCP and UDP connection lists.
  - World-readable procfs entries — no root privileges required.
  - Proper TCP state decoding (Established, Listen, TimeWait, etc.).
  - `pollNow()` for on-demand snapshots; `startPolling()`/`stopPolling()` for lifecycle.

- **FullHD window default:** Main window now defaults to 1920×1080 with a minimum
  of 1280×720.

- **Tab bar expanded to 10 tabs:** All tabs (Overview, Whois, Scanner, Tools, Speedtest,
  TLS Auditor, Topology, Telemetry, History, About) always visible without scrolling.
  Added `Topology Tab` between TLS Auditor and Telemetry.

### Changed

- **Version bumped:** `2.9.1` → `2.10.0` (`CMakeLists.txt`).
- **Tab insertion order:** Topology tab placed at position 7 (after TLS Auditor,
  before Telemetry).
- **`MainWindow` extended:** New `TopologyTab *topologyTab` member, `setupPacketModule()`
  method, `IPView::Packet::PacketModule` background service.

### Build

- `CMakeLists.txt` supplemented with `TopologyTab.cpp` and `PacketModule.cpp`.
- `resources.qrc` registered `svgs/topology.svg`.

---

## [2.9.1] — 2026-05-19

### Fixed

- **FlagLoader — Async race condition (CRITICAL):** `loadFlag()` previously stored the
  target label and country code as member variables, causing a data race when multiple
  requests were dispatched before the first one completed. The first reply would then
  overwrite the flag on the wrong label. Fixed by attaching per-request context
  (`flagCountry`, `flagLabel`) directly to each `QNetworkReply` via `setProperty()`.
  The `currentLabel` and `currentCountry` member fields have been removed.

- **TracerouteTab — Wrong I/O channel on stderr signal (CRITICAL):** `onDataReceived()`
  was connected to both `readyReadStandardOutput` and `readyReadStandardError`, but
  called `QProcess::readAll()` which only reads stdout. When the stderr signal fired,
  no output was displayed. Fixed by calling `readAllStandardOutput()` and
  `readAllStandardError()` separately, with stderr text prefixed by `[stderr]`.

- **Iperf3Window — Missing host validation (HIGH):** User-supplied host input was
  passed directly to the iperf3 process without validation. Added
  `isValidNetworkTarget()` check before launching. The binary path is now resolved
  via `findSystemTool("iperf3")` instead of relying on a hardcoded command name,
  preventing PATH hijacking attacks.

- **WhoisTab — Inconsistent destructor:** Added explicit `override` destructor with
  `= default` to match the RAII pattern used by all other tabs.

### Changed

- **Iperf3Window — Hardcoded styles → Theme.h tokens:** Replaced all inline color
  literals (`#e94560`, `#1a1a2e`, `#00ff88`, etc.) with preprocessor constants from
  `Theme.h` (`C_ACCENT`, `C_BG_ELEVATED`, `C_SUCCESS`, etc.). The start button now
  uses the standard `btnAccentStyle()` helper. The output text edit received a
  proper border and background using the unified token system.

- **WhoisTab — Code-style alignment:** Refactored to use the `setupUI()` pattern
  consistent with all other tabs. Methods annotated with `noexcept`. All string
  literals now use `QStringLiteral`. Label widgets are explicitly styled with
  `C_TEXT` for color consistency. Member pointers initialize to `nullptr`.

- **MainWindow — Comment language:** One remaining German inline comment (`5 Minuten`)
  translated to professional US English (`5 minutes`).

### Security

- **Iperf3 binary path resolution:** `findSystemTool("iperf3")` now resolves the
  absolute path to the iperf3 binary before execution, eliminating reliance on
  `PATH` environment variable and preventing privilege-escalation vectors through
  PATH hijacking.

---

## [2.9.0] — 2026-05-19

### Added
- **`ServerSelectionModule`** — Modular server selection for Speedtest with `ServerInfo` struct,
  `getAvailableServers()` method, filtering (`filterByDistance`, `filterByQuery`),
  sorting (`sortServers`), and lookup helpers (`findClosestServer`, `findServerById`).
  Extracted from SpeedtestTab for modular reuse. Uses `std::expected` for error handling.
  Emits `serverFetchStarted`, `serverFetchProgress`, `serverFetchFinished`, `serverFetchError` signals.
- **`TelemetryPersistenceModule`** — Periodic telemetry aggregation engine that stores interface
  statistics into the new `telemetry_aggregated` SQLite table. Configurable aggregation interval
  (default 60 s). Supports historical queries (`getHistory`, `getHistoryForInterface`,
  `getLatestAggregation`, `getStatsForWindow`). Auto-start configurable via QSettings.
  Emits `aggregationStored`, `aggregationError`, `aggregationStarted`, `aggregationCompleted` signals.
- **`DatabaseModule` — Aggregated telemetry table:** New `telemetry_aggregated` table with
  columns for min/max/avg RX/TX speeds, total bytes, and time window. New methods:
  `storeTelemetryAggregated()`, `getTelemetryAggregated()`, `getTelemetryAggregatedForInterface()`,
  `getLatestTelemetryAggregated()`, `getTelemetryStatsForWindow()`, `clearTelemetryAggregated()`.
- **`ConfigManager` — Telemetry auto-start keys:** `TELEMETRY_AUTO_START` and
  `TELEMETRY_WINDOW_SIZE` keys added. New methods `saveTelemetryAutoStart()`,
  `loadTelemetryAutoStart()`, `saveTelemetryWindowSize()`, `loadTelemetryWindowSize()`.
- **Auto-start at launch:** `MainWindow` now checks `ConfigManager::loadTelemetryAutoStart()`
  on startup and auto-starts `TelemetryPersistenceModule` if previously enabled.
- **Build system:** `CMakeLists.txt` updated with `ServerSelectionModule.cpp` and
  `TelemetryPersistenceModule.cpp`.

### Changed
- **Version bumped:** `2.8.1` → `2.9.0`
- **ServerInfo struct** moved from `SpeedtestTab.h` into `IPView::Speedtest::ServerInfo`
  with additional `latencyMs` field.
- **MainWindow** saves/loads telemetry persistence auto-start state.

### Maintenance
- **German→English comment cleanup:** All remaining German-language comments in source
  files translated to professional US English (14 files, ~40 comment lines).
  This includes `ScannerModule`, `MainWindow`, `ConfigManager`, `DatabaseModule`,
  `HistoryTab`, `TelemetryModule`, `TelemetryTab`, `TracerouteTab`, `ToolsTab`,
  `SpeedtestTab`, `DataNormalizer`, and `DashboardView`.
- **`Q_UNUSED` replaced:** `DashboardView::setStatusMessage` now uses `(void)msg` cast
  instead of `Q_UNUSED` macro.
- **README dependencies expanded:** Added dependency table with package-to-module mapping,
  optional tools table, and explicit `qt6-svg` documentation.

---

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
