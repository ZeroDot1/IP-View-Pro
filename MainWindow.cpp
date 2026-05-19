// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — MainWindow.cpp
//  C++26: std::array for compile-time constants, auto, [[maybe_unused]]
//  QStringLiteral, structured bindings
//  Dashboard functionality extracted into DashboardView (IPView::UI).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "MainWindow.h"
#include "ScannerTab.h"
#include "TelemetryTab.h"
#include "AuditorTab.h"
#include "Theme.h"
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

// ═══════════════════════════════════════════════════════════════════════════════
MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupAlertEngine();
    setupPacketModule();
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

    // ── Event-based tab distribution (Item 2) ──────────────────────────
    //  Statt direkter Methodenaufrufe werden Signale emittiert,
    //  to which tabs can attach themselves independently.
    connect(this, &MainWindow::dataRefreshed, this, [this](const QJsonObject &d) {
        dashboardView->updateDisplay(d);
        QString const cc = d[QStringLiteral("country_code")].toString();
        if (!cc.isEmpty()) {
            flagLoader->loadFlag(cc, dashboardView->flagLabelWidget());
        }
        // Sub-tabs via signals instead of direct calls
        whoisTab->setIp(d[QStringLiteral("ip")].toString());
        toolsTab->setTargetIp(d[QStringLiteral("ip")].toString());
        updateTrayTooltip(d);
        statusLabel->setText(QStringLiteral("Last update successful"));
    });

    connect(this, &MainWindow::historyUpdated, this, [this](const QList<QJsonObject> &h) {
        historyTab->updateHistory(h);
    });

    // ── Network-Signale ─────────────────────────────────────────────────
    connect(networkManager, &NetworkManager::dataReceived,
            this, &MainWindow::onDataReceived);
    connect(networkManager, &NetworkManager::errorOccurred, this, [this](QString const& err) {
        statusLabel->setText(QStringLiteral("Error: ") + err);
        dashboardView->setStatusMessage(err);
    });

    // ── Restore per-user configuration (XDG: ~/.config/IPView/) ──────────
    loadSettings();

    onRefreshClicked();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  System Tray
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::setupTray() noexcept
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(QStringLiteral(":/icon.svg")));
    trayIcon->setToolTip(QStringLiteral("IP View Pro"));

    trayMenu = new QMenu(this);
    restoreAction = trayMenu->addAction(QStringLiteral("Restore Window"),
                                        this, &QWidget::showNormal);
    trayMenu->addSeparator();
    exitAction = trayMenu->addAction(QStringLiteral("Exit"),
                                     this, &MainWindow::onExitRequested);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
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
    }
}

void MainWindow::onExitRequested()
{
    reallyQuit = true;
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!reallyQuit && trayIcon->isVisible()) {
        // Minimize to tray – save settings regardless
        saveSettings();
        hide();
        event->ignore();
        trayIcon->showMessage(QStringLiteral("IP View Pro"),
                              QStringLiteral("Application running in tray."),
                              QSystemTrayIcon::Information, 2000);
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
              .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")));

    tip += QStringLiteral("\u2517\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u251B");

    trayIcon->setToolTip(tip);
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

    // ── Tab bar: no scrolling → all 8 tabs always visible ────────────────
    tabWidget->tabBar()->setUsesScrollButtons(false);
    tabWidget->tabBar()->setExpanding(true);
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
    speedtestTab = mTabRegistry.registerTab<SpeedtestTab>(
                       QStringLiteral("speedtest"), QStringLiteral(" Speedtest"),
                       QIcon(QStringLiteral(":/svgs/lightning.svg")));
    mTabRegistry.registerTab<AuditorTab>(
        QStringLiteral("auditor"), QStringLiteral(" TLS Auditor"),
        QIcon(QStringLiteral(":/svgs/shield.svg")));
    topologyTab = mTabRegistry.registerTab<TopologyTab>(
        QStringLiteral("topology"), QStringLiteral(" Topology"),
        QIcon(QStringLiteral(":/svgs/topology.svg")));
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
//  Per-user configuration (XDG Base Directory: ~/.config/IPView/IPView.conf)
//  Each user has their own config + SQLite database.
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::saveSettings() noexcept
{
    using namespace IPView::Config;

    // ── Window ──────────────────────────────────────────────────────────────
    Manager::saveWindowGeometry(saveGeometry());
    Manager::saveLastTab(tabWidget->currentIndex());

    // ── Network ─────────────────────────────────────────────────────────────
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
            dashboardView->setApiIndex(apiIndex);  // Sync ComboBox
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
    mPacketModule->startPolling(5000);  // poll every 5 seconds
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
}
