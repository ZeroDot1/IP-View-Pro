// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — MainWindow.cpp
//  C++26: std::array for compile-time constants, auto, [[maybe_unused]]
//  QStringLiteral, structured bindings
//  Dashboard functionality extracted into DashboardView (IPView::UI).
//  v2.15.4: Full tray view — status header, refresh, show/hide, tab
//  submenu, auto-refresh toggle, quit. Tray icon tooltip is rebuilt
//  on every data refresh; a notification fires on every IP change.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "MainWindow.h"
#include "ScannerTab.h"
#include "TelemetryTab.h"
#include "AuditorTab.h"
#include "Theme.h"
#include "Timeouts.hpp"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QMessageBox>
#include "ErrorDialog.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextStream>
#include <QIcon>
#include "Format.hpp"

// Forward declaration for the file-static helper that turns an
// ISO-3166 alpha-2 country code into the corresponding Unicode
// regional indicator flag. Used by the tray menu status header
// and the tray tooltip builder further down.
[[nodiscard]] static QString countryCodeToFlag(const QString &cc) noexcept;

// ═══════════════════════════════════════════════════════════════════════════════
MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setupPacketModule();  // Must be before setupUI() — PacketTab needs mPacketModule
    setupUI();
    setupAlertEngine();
    setupTray();

    networkManager   = new NetworkManager(this);
    flagLoader       = new FlagLoader(this);
    autoRefreshTimer = new QTimer(this);

    connect(autoRefreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshClicked);

    // ── Pass API names to DashboardView ─────────────────────────────────
    dashboardView->setApiNames(networkManager->getApiNames());

    // ── Connect Dashboard signals to MainWindow ─────────────────────────
    connect(dashboardView, &IPView::UI::DashboardView::refreshRequested,
            this, &MainWindow::onRefreshClicked);
    connect(dashboardView, &IPView::UI::DashboardView::apiChanged,
            this, &MainWindow::onApiChanged);
    connect(dashboardView, &IPView::UI::DashboardView::ipv6Toggled,
            this, &MainWindow::onIPv6Toggled);
    connect(dashboardView, &IPView::UI::DashboardView::autoRefreshToggled,
            this, &MainWindow::onAutoRefreshToggled);
    connect(dashboardView, &IPView::UI::DashboardView::copyAllRequested,
            this, &MainWindow::onCopyAllRequested);
    connect(dashboardView, &IPView::UI::DashboardView::exportJsonRequested,
            this, &MainWindow::onExportJsonRequested);

    // ── Event-based tab distribution ────────────────────────────────────
    //  Instead of direct method calls, signals are emitted so that
    //  tabs can subscribe independently without tight coupling.
    connect(this, &MainWindow::dataRefreshed, this, [this](const QJsonObject &d) {
        // IP-change notification fires before the data is committed
        // to currentData so notifyIpChange can compare the old
        // and new IPs against `history.first()`.
        notifyIpChange(currentData, d);

        dashboardView->updateDisplay(d);
        QString const cc = d[QStringLiteral("country_code")].toString();
        if (!cc.isEmpty()) {
            flagLoader->loadFlag(cc, dashboardView->flagLabelWidget());
        }
        // Sub-tabs via signals instead of direct calls
        whoisTab->setIp(d[QStringLiteral("ip")].toString());
        toolsTab->setTargetIp(d[QStringLiteral("ip")].toString());
        updateTrayTooltip(d);
        updateTrayMenuStatus(d);
        statusLabel->setText(QStringLiteral("Last update successful"));
    });

    connect(this, &MainWindow::historyUpdated, this, [this](const QList<QJsonObject> &h) {
        historyTab->updateHistory(h);
    });

    // ── Network signals ─────────────────────────────────────────────────────
    connect(networkManager, &NetworkManager::dataReceived,
            this, &MainWindow::onDataReceived);
    connect(networkManager, &NetworkManager::errorOccurred, this, [this](QString const& err) {
        statusLabel->setText(QStringLiteral("Error: ") + err);
        dashboardView->setStatusMessage(err);
    });

    // ── Restore per-user configuration (XDG-compliant: ~/.config/IPView/) ──
    loadSettings();

    onRefreshClicked();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  System Tray — full tray view (v2.15.4)
//
//  The tray icon is the application's "always-visible" entry point
//  when the user has minimised the main window. The menu and
//  tooltip give the user a complete overview without re-opening
//  the main window:
//    • Status header (disabled QAction) shows the current IP and
//      country flag without hovering.
//    • Refresh now — kicks the same code path as the dashboard's
//      refresh button.
//    • Show / hide window — toggle, not a one-shot "show".
//    • Go to tab → 12-entry submenu built from the TabRegistry.
//    • Auto-refresh — checkable action that mirrors the dashboard
//      checkbox state.
//    • Quit — terminates the application with `reallyQuit = true`
//      so the close event doesn't intercept and re-hide.
//
//  The icon is updated by QSystemTrayIcon::setIcon() at refresh
//  time to indicate status (offline → red dot icon would be nice
//  but we use the same icon for now and let the tooltip / menu
//  carry the state — keeps the resource tree slim).
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::setupTray() noexcept
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(QStringLiteral(":/icon.svg")));
    trayIcon->setToolTip(QStringLiteral("IP View Pro"));

    buildTrayMenu();

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
}

// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::buildTrayMenu() noexcept
{
    // The QMenu is parented to the tray icon so the standard
    // "delete on parent destruction" rule applies. The actions
    // are added to the menu and live as long as the menu does.
    trayMenu = new QMenu(this);

    // ── Status header (read-only) ────────────────────────────────────────
    trayStatusAction = trayMenu->addAction(
        QStringLiteral("IP View Pro — initialising…"));
    trayStatusAction->setEnabled(false);

    trayMenu->addSeparator();

    // ── Refresh now ──────────────────────────────────────────────────────
    trayRefreshAction = trayMenu->addAction(
        QIcon(QStringLiteral(":/svgs/chart-bar.svg")),
        QStringLiteral("Refresh now"),
        this, &MainWindow::onRefreshClicked);

    // ── Show / hide window ───────────────────────────────────────────────
    trayShowHideAction = trayMenu->addAction(
        QIcon(QStringLiteral(":/svgs/info.svg")),
        QStringLiteral("Show / hide window"),
        this, &MainWindow::onTrayShowHideTriggered);

    trayMenu->addSeparator();

    // ── "Go to tab" submenu ──────────────────────────────────────────────
    trayTabsMenu = trayMenu->addMenu(
        QIcon(QStringLiteral(":/svgs/topology.svg")),
        QStringLiteral("Go to tab"));
    populateTrayTabsSubmenu();

    trayMenu->addSeparator();

    // ── Auto-refresh toggle ───────────────────────────────────────────────
    trayAutoRefreshAction = trayMenu->addAction(
        QStringLiteral("Auto-refresh every 5 min"));
    trayAutoRefreshAction->setCheckable(true);
    trayAutoRefreshAction->setChecked(autoRefreshTimer
                                      && autoRefreshTimer->isActive());
    connect(trayAutoRefreshAction, &QAction::toggled,
            this, [this](bool checked) { onAutoRefreshToggled(checked); });

    // ── Quit ──────────────────────────────────────────────────────────────
    trayExitAction = trayMenu->addAction(
        QIcon(QStringLiteral(":/svgs/wastebasket.svg")),
        QStringLiteral("Quit"),
        this, &MainWindow::onExitRequested);
}

// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::populateTrayTabsSubmenu() noexcept
{
    if (!trayTabsMenu) return;
    trayTabsMenu->clear();

    // Build the entries directly from the registry so adding a new
    // tab in setupUI() shows up in the tray submenu without any
    // extra wiring. The QAction carries the target widget as
    // `setData(QVariant::fromValue<QWidget*>(...))` so
    // onTraySwitchTabTriggered can cast it back without a name
    // lookup.
    for (auto const &entry : mTabRegistry.entries()) {
        auto *act = trayTabsMenu->addAction(entry.icon, entry.title);
        act->setData(QVariant::fromValue<QWidget*>(entry.widget));
        connect(act, &QAction::triggered,
                this, &MainWindow::onTraySwitchTabTriggered);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::updateTrayMenuStatus(const QJsonObject &jsonData) noexcept
{
    if (!trayStatusAction) return;

    QString const ip  = jsonData[QStringLiteral("ip")].toString();
    QString const cc  = jsonData[QStringLiteral("country_code")].toString();
    QString const cn  = jsonData[QStringLiteral("country_name")].toString();
    QString const org = jsonData[QStringLiteral("org")].toString();

    QString line;
    if (ip.isEmpty()) {
        line = QStringLiteral("IP View Pro — refreshing…");
    } else {
        // "🇩🇪 1.2.3.4 — Germany · Acme ISP"
        QStringList parts;
        if (!cc.isEmpty()) parts.append(countryCodeToFlag(cc));
        parts.append(ip);
        if (!cn.isEmpty()) parts.append(cn);
        if (!org.isEmpty()) parts.append(org);
        line = QStringLiteral("IP View Pro — ") + parts.join(QStringLiteral(" · "));
    }
    trayStatusAction->setText(line);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Tray & Event Handlers
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            activateWindow();
        }
    } else if (reason == QSystemTrayIcon::MiddleClick) {
        // Middle-click is a fast way to kick a refresh, matching
        // common UX (browsers, chat clients).
        onRefreshClicked();
    } else if (reason == QSystemTrayIcon::DoubleClick) {
        // Double-click always brings up the main window — the
        // "already visible" case is a no-op via showNormal().
        showNormal();
        activateWindow();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onTrayShowHideTriggered()
{
    if (isVisible()) {
        hide();
    } else {
        showNormal();
        activateWindow();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onTraySwitchTabTriggered()
{
    auto *act = qobject_cast<QAction*>(sender());
    if (!act) return;
    auto *target = act->data().value<QWidget*>();
    if (!target || !tabWidget) return;

    int const idx = tabWidget->indexOf(target);
    if (idx >= 0) {
        tabWidget->setCurrentIndex(idx);
        // Bring the window forward so the user can see the switch
        // — switching the active tab is meaningless if the window
        // is hidden in the tray.
        if (!isVisible()) {
            showNormal();
            activateWindow();
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onExitRequested()
{
    reallyQuit = true;
    QApplication::quit();
}

// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!reallyQuit && trayIcon && trayIcon->isVisible()) {
        // Minimize to tray — save settings regardless
        saveSettings();
        hide();
        event->ignore();
        if (trayIcon->supportsMessages()) {
            trayIcon->showMessage(QStringLiteral("IP View Pro"),
                                  QStringLiteral("Running in the system tray. "
                                                 "Click the icon to restore."),
                                  QIcon(QStringLiteral(":/icon.svg")),
                                  static_cast<int>(IPView::Timeouts::TRAY_TOOLTIP.count()));
        }
    } else {
        // Really quit — save everything first
        saveSettings();
        event->accept();
    }
}

// ── Country code (e.g. "DE") → Unicode flag (🇩🇪) ─────────────────────────
//  Uses Unicode Regional Indicator Symbols (U+1F1E6..U+1F1FF).
//  Fallback: if the code is invalid, returns a white flag "🏳".
[[nodiscard]]
static QString countryCodeToFlag(const QString &cc) noexcept
{
    if (cc.length() != 2) return QStringLiteral("\U0001F3F3"); // white flag

    QString const upper = cc.toUpper();
    char32_t const a = static_cast<char32_t>(upper[0].unicode());
    char32_t const b = static_cast<char32_t>(upper[1].unicode());

    if (a < 'A' || a > 'Z' || b < 'A' || b > 'Z')
        return QStringLiteral("\U0001F3F3");

    char32_t const chars[2] = {
        0x1F1E6U + (static_cast<char32_t>(a) - 'A'),
        0x1F1E6U + (static_cast<char32_t>(b) - 'A')
    };
    return QString::fromUcs4(chars, 2);
}

// ── Tray hover tooltip (dark-theme design with Unicode) ────────────────────
void MainWindow::updateTrayTooltip(const QJsonObject &jsonData) noexcept
{
    QString tip;
    QString const ip     = jsonData[QStringLiteral("ip")].toString();
    QString const city   = jsonData[QStringLiteral("city")].toString();
    QString const cc     = jsonData[QStringLiteral("country_code")].toString();
    QString const cn     = jsonData[QStringLiteral("country_name")].toString();
    QString const org    = jsonData[QStringLiteral("org")].toString();
    QString const asn    = jsonData[QStringLiteral("asn")].toString();

    tip += QStringLiteral("\u250F\u2501\u2501\u2501 IP View Pro \u2501\u2501\u2501\u2513\n");
    tip += QStringLiteral("\u2503  \u25CF Online                     \u2503\n");

    tip += QStringLiteral("\u2523\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u252B\n");
    tip += QStringLiteral("\u2503  %1 %2\n")
              .arg(countryCodeToFlag(cc),
                   ip.isEmpty()  ? QStringLiteral("\u2014") : ip);
    if (!cn.isEmpty()) {
        tip += QStringLiteral("\u2503     %1").arg(cn);
        if (!city.isEmpty()) tip += QStringLiteral("  \u00B7  %1").arg(city);
        tip += QLatin1Char('\n');
    }

    if (!org.isEmpty() || !asn.isEmpty()) {
        tip += QStringLiteral("\u2523\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u252B\n");
        if (!org.isEmpty())
            tip += QStringLiteral("\u2503  %1\n").arg(org);
        if (!asn.isEmpty())
            tip += QStringLiteral("\u2503  %1\n").arg(asn);
    }

    tip += QStringLiteral("\u2523\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u252B\n");
    tip += QStringLiteral("\u2503  %1\n")
              .arg(QDateTime::currentDateTime().toString(QString::fromLatin1(IPView::Format::TIME_HMS.data(),
                                                                             static_cast<int>(IPView::Format::TIME_HMS.size()))));

    tip += QStringLiteral("\u2517\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u251B");

    trayIcon->setToolTip(tip);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  notifyIpChange — fire a single QSystemTrayIcon::showMessage()
//  notification when the IP changes between two refreshes. Silently
//  no-ops if the new data is empty (still loading) or if the new
//  IP is the same as the previous one (refresh, no change). Also
//  no-ops if the OS shell doesn't support notifications (common on
//  Linux without a notification daemon).
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::notifyIpChange(const QJsonObject &oldData,
                                const QJsonObject &newData) noexcept
{
    if (!trayIcon || !trayIcon->supportsMessages()) return;

    QString const newIp = newData[QStringLiteral("ip")].toString();
    bool const oldHasCc = !oldData.value(QStringLiteral("country_code"))
                                .toString().isEmpty();
    QString const oldIp = oldHasCc
                              ? oldData[QStringLiteral("ip")].toString()
                              : QString();

    if (newIp.isEmpty() || newIp == mLastNotifiedIp) return;
    if (!oldIp.isEmpty() && newIp == oldIp) return;   // First refresh after launch.

    mLastNotifiedIp = newIp;

    QString const cc  = newData[QStringLiteral("country_code")].toString();
    QString const cn  = newData[QStringLiteral("country_name")].toString();
    QString const org = newData[QStringLiteral("org")].toString();

    QString const title = QStringLiteral("IP address changed");
    QString const body  = QStringLiteral("%1 — %2%3%4")
        .arg(newIp,
             countryCodeToFlag(cc),
             cn.isEmpty() ? QString() : QStringLiteral(" ") + cn,
             org.isEmpty() ? QString() : QStringLiteral(" · ") + org);

    trayIcon->showMessage(title, body,
                          QIcon(QStringLiteral(":/icon.svg")),
                          static_cast<int>(IPView::Timeouts::TRAY_TOOLTIP.count()));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  UI Setup
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::setupUI() noexcept
{
    setWindowTitle(QStringLiteral("IP View Pro v") + QApplication::applicationVersion());
    setMinimumSize(1280, 720);
    resize(1920, 1080);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    tabWidget = new QTabWidget();

    // ── Tab bar: no scrolling → all 12 tabs always visible ────────────────
    tabWidget->tabBar()->setUsesScrollButtons(false);
    tabWidget->tabBar()->setExpanding(true);
    tabWidget->tabBar()->setStyleSheet(QStringLiteral(
        "QTabBar::tab { padding: 6px 12px; font-size: 11px; }"
    ));
    tabWidget->setIconSize(QSize(14, 14));

    // ── Tabs in logical order ──────────────────────────────────────
    //  1. Overview    – Entry point & status
    //  2. Whois       – IP/host information
    //  3. Scanner     – Port scan
    //  4. Tools       – Ping, Traceroute, iPerf3
    //  5. Speedtest   – Bandwidth
    //  6. TLS Auditor – Security check
    //  7. Topology    – Network path visualization
    //  8. Telemetry   – Live monitoring
    //  9. History     – History
    // 10. About       – App info

    // Dashboard (Overview) — special case (own namespace)
    dashboardView = mTabRegistry.registerTab<IPView::UI::DashboardView>(
        QStringLiteral("dashboard"), QStringLiteral(" Overview"),
        QIcon(QStringLiteral(":/svgs/chart-bar.svg")));

    whoisTab     = mTabRegistry.registerTab<WhoisTab>(
                       QStringLiteral("whois"),     QStringLiteral(" Whois Lookup"),
                       QIcon(QStringLiteral(":/svgs/search.svg")));
    scannerTab   = mTabRegistry.registerTab<ScannerTab>(
                       QStringLiteral("scanner"),   QStringLiteral(" Port Scanner"),
                       QIcon(QStringLiteral(":/svgs/compass.svg")));
    toolsTab     = mTabRegistry.registerTab<ToolsTab>(
                       QStringLiteral("tools"),     QStringLiteral(" Network Tools"),
                       QIcon(QStringLiteral(":/svgs/wrench.svg")));
    // The ToolsTab emits portScanRequested(ip) when the user
    // clicks the per-row "Port scan" button in the network
    // discovery sub-tab. We catch it here, switch focus to the
    // Port Scanner tab, and start the scan via the new
    // setTargetAndStart() entry point.
    connect(toolsTab, &ToolsTab::portScanRequested,
            this, [this](const QString &ip) {
                if (!scannerTab) return;
                scannerTab->setTargetAndStart(ip);
                if (tabWidget) {
                    int const idx = tabWidget->indexOf(scannerTab);
                    if (idx >= 0) tabWidget->setCurrentIndex(idx);
                }
            });
    speedtestTab = mTabRegistry.registerTab<SpeedtestTab>(
                       QStringLiteral("speedtest"), QStringLiteral(" Speedtest"),
                       QIcon(QStringLiteral(":/svgs/lightning.svg")));
    mTabRegistry.registerTab<AuditorTab>(
        QStringLiteral("auditor"), QStringLiteral(" TLS Auditor"),
        QIcon(QStringLiteral(":/svgs/shield.svg")));
    topologyTab = mTabRegistry.registerTab<TopologyTab>(
        QStringLiteral("topology"), QStringLiteral(" Topology"),
        QIcon(QStringLiteral(":/svgs/topology.svg")));
    packetTab = mTabRegistry.registerTab<IPView::UI::PacketTab>(
        QStringLiteral("packets"), QStringLiteral(" Connections"),
        QIcon(QStringLiteral(":/svgs/graph.svg")), mPacketModule);
    telemetryTab = mTabRegistry.registerTab<TelemetryTab>(
                       QStringLiteral("telemetry"), QStringLiteral(" Telemetry"),
                       QIcon(QStringLiteral(":/svgs/graph.svg")));
    historyTab   = mTabRegistry.registerTab<HistoryTab>(
                       QStringLiteral("history"),   QStringLiteral(" History"),
                       QIcon(QStringLiteral(":/svgs/scroll.svg")));
    aboutTab     = mTabRegistry.registerTab<AboutTab>(
                       QStringLiteral("about"),     QStringLiteral(" About"),
                       QIcon(QStringLiteral(":/svgs/info.svg")));

    // Insert all registered tabs into the tab widget
    mTabRegistry.populateTabWidget(tabWidget);

    mainLayout->addWidget(tabWidget);

    // Status bar
    auto *statusLayout = new QHBoxLayout();
    statusLabel = new QLabel(QStringLiteral("Ready"));
    statusLabel->setStyleSheet(statusLabelStyle());
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Data Processing
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onDataReceived(const QJsonObject &jsonData)
{
    currentData = jsonData;

    // Update history only on IP change
    updateHistory(jsonData);

    // Event-based distribution to all tabs
    emit dataRefreshed(jsonData);
}

void MainWindow::updateHistory(const QJsonObject &jsonEntry) noexcept
{
    if (history.isEmpty() || history.first()[QStringLiteral("ip")] != jsonEntry[QStringLiteral("ip")]) {
        history.prepend(jsonEntry);
        static constexpr int MAX_HISTORY = 50;
        while (history.size() > MAX_HISTORY) {
            history.removeLast();
        }
        emit historyUpdated(history);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Buttons & Controls
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onRefreshClicked()
{
    statusLabel->setText(QStringLiteral("Refreshing data..."));
    if (dashboardView->isIPv6Mode()) {
        networkManager->fetchIPv6Data();
    } else {
        networkManager->fetchIPData();
    }
}

void MainWindow::onApiChanged(int index)
{
    networkManager->setSelectedAPI(index);
}

void MainWindow::onIPv6Toggled(bool /*checked*/)
{
    // API combo-box is managed by DashboardView — no further action needed
}

void MainWindow::onAutoRefreshToggled(bool checked)
{
    if (checked) {
        autoRefreshTimer->start(300000);  // 5 minutes
    } else {
        autoRefreshTimer->stop();
    }
}

void MainWindow::onCopyAllRequested()
{
    if (currentData.isEmpty()) return;

    QString text;
    text += QStringLiteral("IP View Pro - Data Export\n");
    text += QStringLiteral("========================\n");
    text += QStringLiteral("Timestamp: %1\n\n")
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    // C++26: structured bindings for QJsonObject iteration
    for (auto it = currentData.begin(); it != currentData.end(); ++it) {
        text += QStringLiteral("%1: %2\n")
                    .arg(it.key(), -22)
                    .arg(it.value().toString().isEmpty()
                             ? QStringLiteral("N/A")
                             : it.value().toString());
    }

    QApplication::clipboard()->setText(text);
    statusLabel->setText(QStringLiteral("Copied to clipboard!"));
}

void MainWindow::onExportJsonRequested()
{
    if (currentData.isEmpty()) {
        statusLabel->setText(QStringLiteral("No data to export"));
        return;
    }

    QString const fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export IP Data as JSON"),
        QStringLiteral("ipview_%1.json")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("JSON Files (*.json)")
    );

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        IPView::UI::ErrorDialog::showError(this,
                                           QStringLiteral("Export Error"),
                                           QStringLiteral("Could not open file for writing:\n%1")
                                               .arg(file.errorString()));
        return;
    }

    QJsonDocument const doc(currentData);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    statusLabel->setText(QStringLiteral("Data exported to: %1").arg(fileName));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Per-user configuration persistence
//  XDG Base Directory compliant: settings stored at ~/.config/IPView/IPView.conf
//  Each user maintains an independent configuration and SQLite database.
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::saveSettings() noexcept
{
    using namespace IPView::Config;

    // ── Window ──────────────────────────────────────────────────────────────
    Manager::saveWindowGeometry(saveGeometry());
    Manager::saveLastTab(tabWidget->currentIndex());

    // ── Network signals ─────────────────────────────────────────────────────────
    Manager::saveNetworkSettings(
        networkManager->getSelectedApiIndex(),
        dashboardView->isIPv6Mode(),
        autoRefreshTimer->isActive()
    );

    Manager::sync();
}

void MainWindow::loadSettings() noexcept
{
    using namespace IPView::Config;

    // ── Restore window geometry ────────────────────────────────────────────
    QByteArray const geom = Manager::loadWindowGeometry();
    if (!geom.isEmpty()) {
        restoreGeometry(geom);
    }

    // ── Restore last tab ───────────────────────────────────────────────────
    int const lastTab = Manager::loadLastTab();
    if (lastTab >= 0 && lastTab < tabWidget->count()) {
        tabWidget->setCurrentIndex(lastTab);
    }

    // ── Restore network settings ───────────────────────────────────────────
    int const apiIndex = Manager::loadApiIndex();
    if (apiIndex >= 0) {
        networkManager->setSelectedAPI(apiIndex);
        dashboardView->setApiIndex(apiIndex);  // Synchronize the combo box
    }

    bool const ipv6 = Manager::loadIPv6Mode();
    dashboardView->setIPv6Mode(ipv6);

    bool const autoRefresh = Manager::loadAutoRefresh();
    dashboardView->setAutoRefresh(autoRefresh);
    if (autoRefresh) {
        autoRefreshTimer->start(300000);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Item 47: PacketModule — active connection monitoring
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::setupPacketModule() noexcept
{
    mPacketModule = new IPView::Packet::PacketModule(this);
    mPacketModule->startPolling(static_cast<int>(IPView::Timeouts::POLL_PACKET_DEFAULT.count()));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Item 49: Alert Engine — connects Telemetry + Audit with alerts
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::setupAlertEngine() noexcept
{
    mAlertEngine = new IPView::Alert::AlertEngine(this);

    // ── Telemetry → AlertEngine ───────────────────────────────────────────
    if (telemetryTab && telemetryTab->telemetryModule()) {
        connect(telemetryTab->telemetryModule(),
                &IPView::Telemetry::TelemetryModule::telemetryUpdated,
                mAlertEngine,
                &IPView::Alert::AlertEngine::feedTelemetry);
    }

    // ── TLS Auditor → AlertEngine ─────────────────────────────────────────
    // AuditorTab is registered in setupUI(); retrieve it via TabRegistry.
    if (auto *auditorTabPtr = mTabRegistry.findAs<AuditorTab>(QStringLiteral("auditor"))) {
        if (auditorTabPtr->auditorModule()) {
            connect(auditorTabPtr->auditorModule(),
                    &IPView::Auditor::AuditorModule::auditFinished,
                    this, [this](const QString & /*host*/,
                                 const IPView::Auditor::AuditResult &result) {
                        mAlertEngine->feedAuditResult(result);
                    });
        }
    }

    // ── AlertTab — GUI for AlertEngine ────────────────────────────────────
    alertTab = new IPView::UI::AlertTab(mAlertEngine);
    tabWidget->addTab(alertTab, QIcon(QStringLiteral(":/svgs/warning.svg")),
                      QStringLiteral(" Alerts"));
}
