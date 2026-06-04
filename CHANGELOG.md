# Changelog — IPView Pro

All notable changes to this project are documented here.

## [2.15.5] — 2026-06-04

### Fixed

- **`Unknown property caret-color` warnings flooded the console.**
  The three QSS strings in `Theme.h` (`appStyleSheetDark()`,
  `appStyleSheetLight()`, `appStyleSheetHighContrast()`) plus the
  per-widget `inputStyle()` helper set the caret colour via the
  W3C `caret-color` property. Qt's QSS engine does **not** support
  that property on QLineEdit / QTextEdit / QSpinBox / QPlainTextEdit
  — it is a QQuickItem-only feature. Every focus event on every
  input field re-parsed the stylesheet and emitted one
  `Unknown property caret-color` warning, so a typical session
  produced several hundred of them. The `caret-color` declarations
  are removed; selection colour is still set via the supported
  `selection-color` property, and the caret now inherits from the
  palette like every other unstyled Qt widget.
- **`qt.svg.draw: The requested buffer size is too big, ignoring`
  warnings when rendering the bundled icons and country flags.**
  Qt 6 caps `QImageReader` / `QSvgRenderer` allocation at 128 MiB
  by default (DoS defence). The bundled `icon.svg` is 512×512 and
  the country flags come from `flagcdn.com`; on a 4K HiDPI display
  the rasterised buffer can easily exceed 128 MiB. `main.cpp` now
  raises the cap to 1 GiB (both via the env var
  `QT_IMAGE_READER_ALLOCATION_LIMIT=1073741824` and the
  `QImageReader::setAllocationLimit(1073741824)` C++ API), which
  is comfortably above what any legitimate icon would ever need
  while still defending against truly absurd payloads.
- **Menu shortcut pointed at a non-existent binary.** `ipview.desktop`
  had `Exec=IPView` (capitalised, matching the in-AppImage
  AppDir layout), but `install.sh` installs the binary to
  `/usr/bin/ipview-pro` (lowercase, the long-standing install
  convention). Clicking the menu item after `sudo ./install.sh`
  therefore launched a `Failed to execute child process
  "IPView"` error. `ipview.desktop`'s `Exec=` is now
  `ipview-pro` to match the installed binary. (The AppImage ships
  its own embedded desktop with the in-AppDir path intact.)
- **`install.sh` banner was stuck at v2.9.1.** Updated to v2.15.5
  so the script's `--help` and the uninstall dialogue reflect the
  current version.
- **GitHub Actions release pipeline did not run on C++26 / Qt 6
  and emitted `1 error + 1 warning` per push.** The previous
  workflows ran on `ubuntu-22.04` (GCC 11, Qt 6.2 from apt) and
  used the now-deprecated `actions/checkout@v4` and
  `softprops/action-gh-release@v2` (both on Node.js 20, removed
  from runners 2026-09-16). The CMake `find_package(Qt6 6.11
  REQUIRED …)` rejected the system Qt 6.2 from `apt`, the
  `cmake -std=c++26` flag was rejected by GCC 11, and the
  release-creation step was the deprecated softprops action.
  All four workflows (`release.yml`, `release-on-changelog.yml`,
  `build.yml`, `lint.yml`) are now on:
  - `ubuntu-24.04`
  - `actions/checkout@v5`
  - `FORCE_JAVASCRIPT_ACTIONS_TO_NODE24: 'true'` env var
  - GCC 16 via the `ubuntu-toolchain-r/test` PPA (first GCC
    with full C++26 support)
  - Qt 6.8.* via `jurplel/install-qt-action@v4` (LTS line;
    CMakeLists now requires only Qt 6.5+)
  - `appimagetool` from `AppImage/AppImageKit` tag 12
  - `gh release create` for the GitHub Release upload
    (replaces the deprecated softprops action)
  - Three-statement QA gate: build → ctest → offscreen
    smoke test, with a 124-exit-code tolerance on the
    smoke test (the app is designed to keep running)

## [2.15.5] — 2026-06-04

### Added

- **Light theme.** A full light-mode stylesheet (`appStyleSheetLight()`)
  using the same Teal Amber brand hues on a near-white surface. Defined
  via a parallel set of `C_L_BG / C_L_BG_ELEVATED / C_L_SURFACE /
  C_L_BORDER / C_L_TEXT / C_L_TEXT_SEC / C_L_TEXT_DIM / C_L_TEXT_MUTED /
  C_L_TEXT_INV / C_L_MUTED` tokens, plus `C_L_CARET`,
  `C_L_SELECTION_BG / C_L_SELECTION_FG`, `C_L_FOCUS_OUTLINE`, and
  accent / primary / status fallbacks. Designed for daylight and
  projector use.
- **High-Contrast theme.** A WCAG-AAA accessibility stylesheet
  (`appStyleSheetHighContrast()`) with pure black + pure white +
  saturated cyan / yellow / red / green accents, `2px` borders
  everywhere, no `border-radius`, slightly larger fonts (14 px base,
  13 px tab), larger `QCheckBox` indicator (18 px). Tokens:
  `C_HC_BG / C_HC_SURFACE / C_HC_BORDER / C_HC_TEXT / C_HC_PRIMARY /
  C_HC_ACCENT / C_HC_SUCCESS / C_HC_WARNING / C_HC_ERROR /
  C_HC_CRITICAL / C_HC_MUTED` plus caret / selection / focus variants.
- **Theme-mode dispatch.** `ThemeMode` enum (`Dark=0 / Light=1 /
  HighContrast=2`) with `themeModeName(mode)` returning
  `"Dark (OLED)"` / `"Light"` / `"High Contrast"`. New entry point
  `IPView::Theme::applyTheme(QApplication&, ThemeMode)` plus a
  `QCoreApplication` no-op overload and `refreshTheme()` for runtime
  toggles. `appStyleSheet()` is kept as a back-compat alias returning
  the dark theme.
- **Data-visualization palettes.**
  - `C_CHART_0..9` — 10-colour categorical palette (teal / amber /
    teal-green / red / purple / deep-orange / cyan / lime / pink /
    blue-grey) for NetworkDiscovery, PortScanner, Telemetry graphs.
  - `C_STATE_ESTABLISHED / LISTEN / TIME_WAIT / CLOSE_WAIT /
    FIN_WAIT / CLOSED / SYN_SENT / UDP` — connection-state palette
    for `PacketModule`.
  - `C_LATENCY_EXCELLENT / GOOD / FAIR / POOR / TIMEOUT` — latency
    gradient for Topology + Iperf3.
- **Pure-function colour helpers** (no QObject dep, safe from any
  thread): `chartColor(int index)`, `connectionStateColor(state)`,
  `latencyColorMs(ms)`, `speedColorMbps(mbps)`.
- **Form-validation variants of `inputStyle()`**:
  `inputStyleWarning()` (amber-700 border),
  `inputStyleSuccess()` (teal-green border),
  `inputStyleError()` (red border).
- **New widget styles in `Theme.h`:** `btnSmallStyle`,
  `treeWidgetStyle`, `listWidgetStyle`, `toolbarStyle`. New QSS
  selectors in `appStyleSheetDark()`: `QSpinBox / QDoubleSpinBox`,
  `QTreeWidget / QListWidget`, `QToolButton / QToolBar`, plus
  vertical+horizontal scrollbar styling.
- **`constexpr std::array<std::string_view>` token tables** in
  `namespace IPView::Theme` for tooling / CI lint:
  `COLOR_TOKENS_DARK` (70+), `COLOR_TOKENS_LIGHT` (20),
  `COLOR_TOKENS_HC` (26), `SPACING_TOKENS` (16), `RADIUS_TOKENS` (8),
  `SIZING_TOKENS` (16), `TYPOGRAPHY_TOKENS` (19), `ANIMATION_TOKENS`
  (11), `SHADOW_TOKENS` (9), `Z_INDEX_TOKENS` (9), `OPACITY_TOKENS`
  (5), `BORDER_TOKENS` (8), `BREAKPOINT_TOKENS` (4).

### Compatibility

- `appStyleSheet()` is unchanged at the API level — it now delegates
  to `appStyleSheetDark()`. Existing call sites in `main.cpp` and
  the rest of the project keep working. New code should call
  `appStyleSheetFor(ThemeMode)` or `applyTheme(app, ThemeMode)`.

## [2.15.4] — 2026-06-04

### Fixed

- **All IP lookups were silently failing.** `NetworkManager`
  enforced an HTTPS-only check in `doRequest()` but three IPv4
  endpoints (`ipwhois.app`, `myip.com`, `checkip.amazonaws.com`)
  and two IPv6 endpoints (`api6.ipify.org`, `api64.ipify.org`)
  were still listed with `http://` URLs. The check rejected
  every response, so no provider ever returned data. All URLs
  switched to `https://`. `doRequest()` retains the check as
  defence-in-depth.
- **Country flags never loaded.** `FlagLoader` was using
  `https://flagpedia.net` (rate-limited, frequently 429'd), set
  the `User-Agent` header on a *copy* of the request *after*
  `get()` had already fired (no-op), and stored the target
  `QLabel*` as a `QVariant<quintptr>` + `reinterpret_cast` —
  which broke the moment a label was destroyed and recreated.
  Rewrite uses a `QHash<QNetworkReply*, FlagRequest>` with
  `QPointer<QLabel>` for safe lifetime tracking, the
  `User-Agent` set on the live request before `get()`, and
  `https://flagcdn.com/w160/<cc>.png` (Cloudflare CDN).

### Added

- **Full tray view.** The tray menu now contains a
  live status header (IP, country flag, country name, ISP),
  *Refresh now*, *Show / hide window*, *Go to tab* (12-entry
  sub-menu built from the central `TabRegistry`),
  *Auto-refresh* (checkable), and *Quit*. Middle-click on the
  tray icon refreshes, double-click restores the window. The
  tray tooltip shows a richer multi-line "IP View Pro / IP /
  Country / ISP / Org" with a "last refreshed" timestamp. On
  close-to-tray, `QSystemTrayIcon::showMessage()` is fired
  once per actual IP change (not on every refresh) and is
  guarded by `trayIcon->supportsMessages()` so it no-ops on
  desktops without a notification daemon.
- **Theme design tokens expanded.** Added severity scale
  (info / warning / critical), status dots (online / offline /
  pending / idle), spacing scale (8-px base), sizing tokens
  (icon, button, flag, table row), typography scale, animation
  durations (hover, press, fade, toast, traytip), shadows
  (sm / md / lg / focus / glow), z-index, opacity. New
  helper styles: `btnPrimaryStyle`, `btnDangerStyle`,
  `btnGhostStyle`, `statusDotStyle(color)`,
  `severityBadgeStyle(bg, fg)`, `toastStyle()`,
  `splitterStyle()`. New global QSS selectors:
  `QFrame[card]`, `QStatusBar`, `QLabel[statusdot=...]`,
  `QLabel[badge=...]`, plus richer `QPushButton` variants.
  `IPView::Theme::COLOR_TOKENS` is a `constexpr std::array`
  of token names for tooling / CI lint.
- **`applyTableStyle(QTableWidget*, sortable, showVHeader)`.**
  Centralises the 3-call table setup
  (SelectRows + Alternating + NoEditTriggers + optional
  sorting / vHeader visibility) used in 5 different tabs.
  Future behaviour change is a one-file edit.
- **`colorStyle(const char* color)` helper.** Replaces
  `QStringLiteral("color: %1;").arg(C_*)` in 8 call sites.
- **`Format.hpp` constants.** `TIME_HMS` ("hh:mm:ss") and
  `DATETIME_SEC` ("yyyy-MM-dd hh:mm:ss") extracted from
  7 raw `QStringLiteral()` calls in 4 files.
- **`TabRegistry::entries()` accessor** returns
  `std::span<const TabEntry>` for iteration by the tray
  sub-menu and any future palette consumer.

### Changed

- **Refactor: dedupe QMessageBox calls.** 9 direct
  `QMessageBox::warning` / `information` calls in
  `AlertTab`, `PacketTab`, `TelemetryTab`, `TopologyTab` were
  migrated to `IPView::UI::ErrorDialog::showError` /
  `showInfo`. Drops the `<QMessageBox>` include from those
  files and routes everything through the project's central
  dialog wrapper. Net: -27 lines, 0 functional changes, 0
  new dependencies.
- **Refactor: remove dead `QPushButton` colour override.**
  Three `QDialogButtonBox` instances had a local
  `setStyleSheet("QPushButton { color: ... }")` that was
  already covered by the global `appStyleSheet()`. Removed.

## [2.15.3] — 2026-06-04

### Added

- **Network discovery sub-tab in Network Tools:** the old
  per-target port-scan sub-tab is replaced by an interactive
  device discovery flow. Enter a `/24` subnet prefix
  (e.g. `192.168.1`, `10.0.0`) or leave it empty to auto-detect
  the local subnet, click *Discover* and the tab populates with
  every reachable host in under 30 s.
  - Parallel ping sweep via bounded `QProcess` pool (max 20
    workers, 50 ms dispatch tick). Total wall time for a /24
    is roughly 1.3 s on a 1 Gbps LAN; longer on Wi-Fi or with
    ICMP rate-limiting in the way.
  - MAC address lookup via `/proc/net/arp` (no extra deps,
    works on any Linux box, returns "—" for hosts that have
    not yet sent a frame through the local switch).
  - Reverse-DNS hostname resolution via `QHostInfo::lookupHost`
    with a serialized pending queue (QHostInfo callbacks fire
    out of order, we tag every request with its IP and
    dequeue on each result).
  - OUI-based vendor identification against a hand-curated
    table of ~400 MAC prefixes (Apple, Samsung, HP, Dell,
    Intel, RPi, Google, Microsoft, VMware, …). OUI lookups
    are linear — 400 entries fits comfortably in L1 cache.
  - Per-row **Port scan** button. Clicking it emits
    `ToolsTab::portScanRequested(ip)`. `MainWindow` catches
    the signal, switches to the Port Scanner tab, and calls
    `ScannerTab::setTargetAndStart(ip)` so a quick 28-port
    scan starts immediately.

### Changed

- `ToolsTab` header rewritten: the old `parseNetScanTargets`
  helper and the three-line port-scan queue are gone. The new
  sub-tab is driven by `IPView::Scanner::NetworkDiscovery` and
  a `QTableWidget` with 5 columns (IP, Hostname, MAC, Vendor,
  Action).
- `ScannerTab` gains a public `setTargetAndStart(const QString &)`
  method as the cross-tab entry point.
- `MainWindow` wires `toolsTab::portScanRequested` to a lambda
  that selects the Port Scanner tab and invokes
  `setTargetAndStart()`.

### Tests

- `tests/test_networkdiscovery.cpp` covers `parseSubnet()`
  happy path (`a.b.c`, `a.b.c.d/n`), rejection of empty
  strings, garbage input, and out-of-range octets, plus a
  smoke check on `detectLocalSubnets()` (loopback is
  expected to be present on every CI runner).

## [2.15.2] — 2026-06-04

### Added

- **Binary size optimization pipeline (CMake + build.sh):**
  - New CMake options: `IPVIEW_MINSIZE`, `IPVIEW_STRIP_BINARY`,
    `IPVIEW_COMPRESS_BINARY`, `IPVIEW_SIZE_REPORT`. All default to OFF
    except `IPVIEW_SIZE_REPORT=ON` so the daily dev loop is unchanged.
  - `IPVIEW_MINSIZE=ON` adds `-Os`, `-fdata-sections`,
    `-ffunction-sections`, `--gc-sections`, `--as-needed`,
    `--hash-style=gnu`, `--build-id=none`, `-z,nodump` and forces
    LTO on. Dead-code elimination at link time typically drops the
    .text section by ~25% on a Qt 6.11 app.
  - `IPVIEW_STRIP_BINARY=ON` runs `strip --strip-all` plus aggressive
    section removal (`.comment`, `.note`, `.note.gnu.build-id`,
    `.note.gnu.property`, `.note.ABI-tag`). Removes ~16% of the
    .symtab/.strtab footprint.
  - `IPVIEW_COMPRESS_BINARY=ON` wraps the binary with `upx --best`.
    Typical ratio is 3-4x on a Qt 6 ELF, transparent to the user
    (decompresses on first exec into memory, leaves the compressed
    file on disk).
  - `IPVIEW_SIZE_REPORT=ON` prints a `du -b` snapshot + `file -b`
    line in the CMake post-build hook, and a formatted table in
    `build.sh` after the build finishes.
  - All three stages degrade gracefully when `strip` or `upx` is not
    installed: a CMake `WARNING` is emitted, the stage is skipped,
    the rest of the pipeline still runs.
  - `build.sh` grew three positional flags: `STRIP`, `COMPRESS`,
    `MINSIZE` (default off) and one `REPORT` flag (default on).
    `./build.sh Release none on off off off on on on on` runs the
    full pipeline.
  - Conflict detection: `MINSIZE=on` + any sanitizer is rejected
    early because sanitizers need `-O1 -g` while MINSIZE strips both.
  - `STRIP=on` with `Debug` build emits a `WARNING` (gdb would be
    useless without symbols).

### Measured impact

| Build configuration                    | Binary size | Δ vs baseline |
|----------------------------------------|------------:|--------------:|
| Release (baseline, unstripped)         | 1,207,176 B |          0.0% |
| + `STRIP=ON`                           | 1,015,288 B |        −15.9% |
| + `MINSIZE=ON`                         |   964,512 B |        −20.1% |
| + `COMPRESS=ON` (UPX --best)           |   403,000 B |        −66.6% |
| `Release` + `MINSIZE` + `STRIP`        |   762,104 B |        −36.9% |
| **`Release` + `MINSIZE` + `STRIP` + `COMPRESS`** | **261,328 B** |  **−78.4%** |
| `MinSizeRel` + `MINSIZE` + `STRIP` + `COMPRESS` |   261,216 B |        −78.4% |

The combined `MINSIZE + STRIP + COMPRESS` configuration shrinks a
fresh `Release` build from **1.18 MiB to 255 KiB** — a **4.7x
reduction in disk footprint** without changing runtime behaviour.
All 50 unit tests, 103 checks, and the 3 s smoke test still pass.

## [2.15.1] — 2026-06-04

### Added

- **Network Tools — "Network scan" sub-tab (sequential multi-target port scan):**
  - New third sub-tab in `ToolsTab` alongside **Ping / iPerf3** and **Traceroute**.
  - Single input field accepts up to **3 comma-separated target IPs** (e.g.
    `192.168.1.1, 192.168.1.2, 192.168.1.3`); optional whitespace around
    commas is trimmed.
  - Each entry is validated through `SecurityUtil::isValidNetworkTarget`
    (the same regex that blocks command injection in the rest of the app).
    Invalid or empty entries are reported in the status label with a list
    of the dropped targets; the run is not aborted.
  - Scan walks the queue sequentially: `scanCompleted` handler triggers the
    next `runScan()` call, so the three ports sweeps never overlap.
  - Results land in a 4-column table (`Target IP`, `Open Port`,
    `Service`, `Latency (ms)`) sorted in the order they were found.
  - Stop button cancels the in-flight scan via `ScannerModule::cancelScan()`,
    clears the queue, and leaves the table in its current state for
    inspection.
  - The hard 3-target cap is enforced with a notice — anything beyond the
    third IP is dropped and named in the status line rather than silently
    truncated.

## [2.15.0] — 2026-06-04

### Added

- **Single source of truth for version metadata:**
  - New `version.hpp.in` template, generated to `build/generated/version.hpp`
    via `configure_file()` in CMakeLists.txt.
  - `IPView::Version::Major / Minor / Patch / Commits / String` constants.
  - `consteval appName / appVersion / appOrgName / appOrgDomain / appDisplayName`
    accessors replace the literal strings previously inlined in `main.cpp`.
  - Git commit count is auto-detected (best-effort, falls back to 0).

### Changed

- **Theme refresh — Teal Amber OLED Dark:**
  - Palette renamed and tightened (primary `#00BCD4`, accent `#FFB300`).
  - Surfaces use a 4-tier grey hierarchy on true OLED black (`#000000`).
  - Removed QSS `transition` rules (Qt does not animate QSS changes for
    the property classes we use; removing them shrinks the stylesheet).
  - Tab-selected color switched to OLED-black text on amber for contrast.
- **Version bumped:** `2.14.0` → `2.15.0` across `CMakeLists.txt`, `main.cpp`,
  `PacketTab.cpp/.h`, `README.md`, build script, and all header banners.
- **Header comments:** remaining German inline comments translated to
  US English to match the rest of the source tree.

### Fixed

- **Version inconsistency:** `main.cpp` was hard-coded to `"2.14.0"` while
  Theme.h, MainWindow.cpp/.h, and build.sh were already on v2.15.0 banners.
  The new `version.hpp` makes this class of drift impossible.

---

## [2.14.0] — 2026-05-21

### Fixed

- **PacketTab — Complete rewrite for reliable connection display:**
  - **Signal-driven updates:** Now connects to `PacketModule::connectionsUpdated` signal
    instead of relying solely on a separate timer. Connections appear immediately when
    the module polls `/proc/net/*`.
  - **Snapshot caching:** Stores last snapshot locally to enable filter/search without
    re-polling the kernel.
  - **Empty state handling:** Shows a clear "No active connections found" message when
    no connections are detected, with instructions to click Refresh.
  - **Filter mismatch message:** Shows warning when connections exist but none match
    the current filter/search criteria.
  - **Detail dialog:** Double-click any row to see full connection details in a styled
    dialog (Protocol, Local/Remote Address/Port, State, UID, TX/RX Queue).
  - **Status indicator:** Live status bar showing connection count or warning messages.
  - **Enhanced theme:** Filter bar wrapped in card-style container, buttons use accent
    and secondary styles consistently, cursor pointers on all interactive elements.
  - **State icons:** Unicode dot indicators (● ○ ◌) for visual state differentiation.
  - **Bold protocol/state columns:** Improved readability with font-weight: bold.
  - **Extended CSV export:** Now includes TX Queue and RX Queue columns.

- **iPerf3 color-coded progress bar — Now functional:**
  - `handleSpeedUpdated()` now calls `updateVisualization()` to apply color-coding:
    green (≥100 Mbps), cyan (≥50 Mbps), orange (≥10 Mbps), red (<10 Mbps).
  - Previously the method existed but was never invoked.

- **Version consistency:** `main.cpp` appVersion updated from "2.9.1" to "2.14.0"
  to match `CMakeLists.txt` and `README.md`.

### Changed

- **Tab bar styling:** Added explicit padding (`6px 12px`) and font-size (`11px`)
  to QTabBar stylesheet for consistent compact tab display across all 12 tabs.
- **Version bumped:** `2.13.0` → `2.14.0` (`CMakeLists.txt`, `main.cpp`).

---

## [2.13.0] — 2026-05-21

### Added

- **PacketTab — Professional connection monitor with filtering and statistics:**
  - **Protocol filter:** TCP, UDP, or All — dropdown selector.
  - **State filter:** Filter by ESTABLISHED, LISTEN, TIME_WAIT, CLOSE_WAIT, SYN_SENT, etc.
  - **Search field:** Real-time search across IP addresses, ports, states, and UIDs.
  - **Statistics bar:** Live counts for TCP, UDP, Total connections, and Established sessions.
  - **State color-coding:** ESTABLISHED (green), LISTEN (blue), TIME_WAIT/CLOSE_WAIT (orange),
    SYN_SENT/SYN_RECV (cyan), closing states (gray).
  - **Detailed tooltips:** Hover any row to see full connection details.
  - **CSV Export:** Export all connections to CSV with one click.
  - **Auto-refresh toggle:** Enable/disable 5-second auto-refresh.
  - **Theme integration:** Full Theme.h stylesheet support for all controls.

- **TopologyTab — Enhanced network path visualization:**
  - **Statistics bar:** Total hops, average latency, maximum latency, timeout count.
  - **Node click details:** Double-click any node to see full hop details in a dialog
    (IP, hostname, latency, quality assessment).
  - **Glow effects:** Nodes now have a subtle glow effect for better visibility.
  - **Color-coded connections:** Lines between nodes colored by latency (green/blue/orange/red).
  - **TXT Export:** Export topology trace to formatted text file with hop details.
  - **Improved layout:** Better spacing and node sizing based on latency.

- **TelemetryTab — Enhanced network monitoring with peak detection:**
  - **Interface filter:** Dropdown to filter display to a specific interface or show all.
  - **Peak detection:** Tracks and displays peak download/upload speeds during monitoring session.
  - **8-column table:** Added separate RX Errors and TX Errors columns, plus Status column.
  - **Status indicators:** Active (green dot), High traffic (green dot), Idle (white circle).
  - **Interface detail view:** Double-click any interface row to see full details dialog
    (speeds, bytes, packets, errors, status).
  - **CSV Export:** Export telemetry data to CSV with one click.
  - **Dynamic filter population:** Interface filter dropdown auto-populates with detected interfaces.

### Changed

- **Version bumped:** `2.12.0` → `2.13.0` (`CMakeLists.txt`).
- **PacketTab:** Complete rewrite from basic table to professional monitoring dashboard.
- **TopologyTab:** Added statistics, detail dialogs, glow effects, export functionality.
- **TelemetryTab:** Added interface filter, peak tracking, detail view, export, status column.

---

## [2.12.0] — 2026-05-21

### Added

- **PacketTab — Active Connections GUI (Item 47):** New tab displaying live TCP/UDP
  connections parsed from `/proc/net/tcp`, `/proc/net/tcp6`, `/proc/net/udp`, `/proc/net/udp6`.
  - 7-column table: Protocol, Local Addr, Local Port, Remote Addr, Remote Port, State, UID.
  - Auto-refresh every 5 seconds via QTimer.
  - TCP state decoding: ESTABLISHED, SYN_SENT, SYN_RECV, FIN_WAIT1, FIN_WAIT2, TIME_WAIT,
    CLOSE, CLOSE_WAIT, LAST_ACK, LISTEN, CLOSING, UNKNOWN.
  - Connected to `PacketModule` — uses `pollNow()` for on-demand snapshots.

- **AlertTab — Security & Telemetry Alerts GUI (Item 49):** New tab displaying active
  alerts from `AlertEngine` with severity-based color coding.
  - 6-column table: Time, Severity, Category, Title, Message, Source.
  - Severity icons and colors: ⚠ Critical (red), ⚡ Warning (orange), ℹ Info (cyan).
  - Category labels: Telemetry, Security, System, Custom.
  - **Acknowledge Selected** button to mark individual alerts as handled.
  - **Clear Acknowledged** button to remove all acknowledged alerts.
  - Auto-refresh every 3 seconds + real-time updates via `alertTriggered`/`alertsChanged` signals.
  - Status bar showing count of active alerts.

### Changed

- **Tab count expanded to 12:** Overview, Whois, Port Scanner, Network Tools, Speedtest,
  TLS Auditor, Topology, Connections, Telemetry, Alerts, History, About.
- **MainWindow constructor order:** `setupPacketModule()` moved before `setupUI()` so
  `PacketTab` can receive the `PacketModule` pointer during registration.
- **Version bumped:** `2.11.0` → `2.12.0` (`CMakeLists.txt`).

### Build

- `CMakeLists.txt` supplemented with `PacketTab.cpp` and `AlertTab.cpp`.
- `MainWindow.h` extended with `PacketTab.h` and `AlertTab.h` includes.

---

## [2.11.0] — 2026-05-21

### Fixed

- **TopologyTab — Dead eventFilter code (CRITICAL):** Removed unnecessary
  `view->viewport()->installEventFilter(this)` call. QGraphicsView already handles
  scroll-wheel zoom natively via `setTransformationAnchor(QGraphicsView::AnchorUnderMouse)`.
  No `eventFilter` override was declared, making this dead code.

- **AlertEngine — Broken cooldown logic (CRITICAL):** `isOnCooldown()` returned
  `elapsed < 0` which is always false since `secsTo()` returns positive values
  for past timestamps. Fixed to properly compare elapsed seconds against the
  rule's `cooldownSec` value.

- **AuditorModule — Incorrect `noexcept` on `performAudit()` (HIGH):** The method
  was marked `noexcept` but performs `new QSslSocket()` which can throw
  `std::bad_alloc`. Removed `noexcept` to allow proper exception propagation.

- **TracerouteTab — Slot signature mismatch (MEDIUM):** `onTraceFinished(int)`
  did not match Qt6's `QProcess::finished(int, QProcess::ExitStatus)` signal.
  Added the `QProcess::ExitStatus` parameter with `[[maybe_unused]]` attribute.

### Added

- **AlertEngine → AuditorModule integration (CRITICAL):** Connected
  `AuditorModule::auditFinished` signal to `AlertEngine::feedAuditResult()` in
  `MainWindow::setupAlertEngine()`. TLS audit results now automatically trigger
  security alerts (certificate expiry, insecure certificates).

### Changed

- **TelemetryModule — C++26 ranges:** Replaced `std::find_if` with
  `std::ranges::find_if` for idiomatic C++26 container searching.

- **DatabaseModule — Macro elimination:** Replaced `DB_STATUS` macro with
  `static inline void dbStatus()` function. Modern C++26 best practice.

- **ScannerModule — Unused parameter:** Replaced C-style `/*error*/` comment
  with `[[maybe_unused]]` attribute on `onSocketError()` parameter.

- **PacketModule — Full `[[nodiscard]]`/`noexcept` coverage:** Added missing
  `[[nodiscard]]` and `noexcept` to all static parser methods (`parseProcNet`,
  `parseLine`, `hexToIp`, `hexToPort`, `tcpStateFromCode`) and public API
  (`pollNow`, `startPolling`, `stopPolling`).

- **SpeedtestTab — `noexcept` coverage:** Added `noexcept` to `setupUI()`,
  `startProcess()`, `aggregateMultiResults()`, `setSelectedServer()`.

- **Iperf3Window — `noexcept` coverage:** Added `noexcept` to `setupUI()`,
  `updateStats()`, `handleSpeedUpdated()`.

- **AuditorTab — `noexcept` coverage:** Added `noexcept` to `setupUI()`,
  `addResultToTable()`, `showCertificateDetails()`.

- **TopologyTab — `noexcept` coverage:** Added `noexcept` to `setupUI()`,
  `buildTopology()`, `addNode()`, `clearScene()`.

- **HistoryTab — `noexcept` coverage:** Added `noexcept` to `updateHistory()`.

- **DatabaseModule — `[[nodiscard]]` coverage:** Added `[[nodiscard]]` to all
  bool-returning methods: `storeResult`, `storeTelemetry`, `storeTelemetryAggregated`,
  `clearTelemetryAggregated`, `clearHistory`, `vacuum`, `createSchema`.

- **DatabaseModule — `emitStatusMsg` visibility:** Moved from private to public
  section to enable the `dbStatus()` helper function.

### Build

- **Debug + Release:** Both build configurations compile cleanly with `-Werror`
  and zero warnings.

---

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
