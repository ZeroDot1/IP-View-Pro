// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.3 — ToolsTab.cpp
//  C++26: auto, QStringLiteral, [[maybe_unused]], const-correctness,
//         std::array, std::span, std::expected
//  Ping, iPerf3, Traceroute, and the local-network device discovery
//  sub-tab. The discovery sub-tab uses IPView::Scanner::NetworkDiscovery
//  to ping-sweep a /24 subnet, look up MACs in /proc/net/arp, resolve
//  hostnames via reverse DNS, and identify vendors from the MAC OUI.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "ToolsTab.h"
#include "Theme.h"
#include "SecurityUtil.h"
#include "Logger.h"
#include "Timeouts.hpp"
#include "Iperf3Window.h"
#include "TracerouteTab.h"

#include <QDateTime>
#include <QHeaderView>
#include <QProgressBar>

#include <algorithm>
#include <span>

// ═══════════════════════════════════════════════════════════════════════════════
ToolsTab::ToolsTab(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mToolsTabWidget = new QTabWidget();
    auto *toolsTabWidget = mToolsTabWidget;

    // ── Ping & iPerf3 Tab ───────────────────────────────────────────────────
    auto *pingIperfTab = new QWidget();
    auto *piLayout = new QVBoxLayout(pingIperfTab);

    auto *topRow = new QHBoxLayout();
    targetEdit = new QLineEdit();
    targetEdit->setPlaceholderText(QStringLiteral("Target IP/Host..."));
    targetEdit->setStyleSheet(inputStyle());

    pingButton = new QPushButton(QStringLiteral("▶ Ping"));
    pingButton->setProperty("accent", true);
    pingButton->setStyleSheet(btnAccentStyle());

    stopPingButton = new QPushButton(QStringLiteral("■ Stop"));
    stopPingButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %2; "
        "border-radius: %3; padding: %4; }"
    ).arg(C_BG_ELEVATED, C_ERROR, RADIUS_MD, PADDING_BTN));
    stopPingButton->setEnabled(false);

    iperfButton = new QPushButton(QStringLiteral("iPerf3 Test"));
    iperfButton->setStyleSheet(btnSecondaryStyle());

    auto *targetLabel = new QLabel(QStringLiteral("Target:"));
    topRow->addWidget(targetLabel);
    topRow->addWidget(targetEdit);
    topRow->addWidget(pingButton);
    topRow->addWidget(stopPingButton);
    topRow->addStretch();
    topRow->addWidget(iperfButton);

    outputArea = new QTextEdit();
    outputArea->setReadOnly(true);
    outputArea->setStyleSheet(monoStyle());

    piLayout->addLayout(topRow);
    piLayout->addWidget(outputArea);

    // ── Traceroute Tab ────────────────────────────────────────────────────
    auto *trace = new TracerouteTab();

    // ── Network-Scan Tab (device discovery) ──────────────────────────────
    auto *netScanTab   = new QWidget();
    auto *netScanLayout = new QVBoxLayout(netScanTab);
    netScanLayout->setSpacing(10);
    netScanLayout->setContentsMargins(16, 16, 16, 16);
    setupNetworkScanTab(netScanLayout);

    toolsTabWidget->addTab(pingIperfTab,  QStringLiteral("Ping / iPerf3"));
    toolsTabWidget->addTab(trace,         QStringLiteral("Traceroute"));
    toolsTabWidget->addTab(netScanTab,    QStringLiteral("Network scan"));

    mainLayout->addWidget(toolsTabWidget);

    // ── Connections (signal / slot wiring) ───────────────────────────────
    connect(pingButton,    &QPushButton::clicked, this, &ToolsTab::onPingClicked);
    connect(stopPingButton, &QPushButton::clicked, this, &ToolsTab::onStopPingClicked);
    connect(iperfButton,   &QPushButton::clicked, this, &ToolsTab::onIperfClicked);

    // The network-scan sub-tab uses its own NetworkDiscovery
    // worker. We own it as a child QObject so the lifetime is
    // tied to ToolsTab.
    mNetDiscovery = new IPView::Scanner::NetworkDiscovery(this);
    connect(mNetDiscovery, &IPView::Scanner::NetworkDiscovery::deviceFound,
            this, &ToolsTab::onNetScanDeviceFound);
    connect(mNetDiscovery, &IPView::Scanner::NetworkDiscovery::progress,
            this, &ToolsTab::onNetScanProgress);
    connect(mNetDiscovery, &IPView::Scanner::NetworkDiscovery::completed,
            this, &ToolsTab::onNetScanCompleted);
    connect(mNetDiscovery, &IPView::Scanner::NetworkDiscovery::error,
            this, &ToolsTab::onNetScanError);
    connect(mNetDiscovery, &IPView::Scanner::NetworkDiscovery::cancelled,
            this, &ToolsTab::onNetScanCancelled);

    connect(mNetScanStartBtn, &QPushButton::clicked, this, &ToolsTab::onNetScanStartClicked);
    connect(mNetScanStopBtn,  &QPushButton::clicked, this, &ToolsTab::onNetScanStopClicked);

    refreshSubnetPlaceholder();
}

// ═══════════════════════════════════════════════════════════════════════════════
void ToolsTab::setTargetIp(const QString &ip) noexcept
{
    targetEdit->setText(ip);
}

// ═══════════════════════════════════════════════════════════════════════════════
void ToolsTab::setupNetworkScanTab(QVBoxLayout *parentLayout) noexcept
{
    // ── Subnet input row ────────────────────────────────────────────────
    auto *subnetRow = new QHBoxLayout();
    mNetScanSubnetLbl = new QLabel(QStringLiteral("Subnet:"));
    mNetScanSubnetLbl->setStyleSheet(
        QStringLiteral("color: %1; font-weight: bold;").arg(C_TEXT));
    mNetScanSubnetEdit = new QLineEdit();
    mNetScanSubnetEdit->setPlaceholderText(
        QStringLiteral("e.g. 192.168.1 (empty = auto-detect)"));
    mNetScanSubnetEdit->setStyleSheet(inputStyle());

    mNetScanStartBtn = new QPushButton(QStringLiteral("▶ Discover"));
    mNetScanStartBtn->setProperty("accent", true);
    mNetScanStartBtn->setStyleSheet(btnAccentStyle());
    mNetScanStartBtn->setCursor(Qt::PointingHandCursor);

    mNetScanStopBtn = new QPushButton(QStringLiteral("■ Stop"));
    mNetScanStopBtn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %2; "
        "border-radius: %3; padding: %4; }"
    ).arg(C_BG_ELEVATED, C_ERROR, RADIUS_MD, PADDING_BTN));
    mNetScanStopBtn->setCursor(Qt::PointingHandCursor);
    mNetScanStopBtn->setEnabled(false);

    subnetRow->addWidget(mNetScanSubnetLbl);
    subnetRow->addWidget(mNetScanSubnetEdit, 1);
    subnetRow->addWidget(mNetScanStartBtn);
    subnetRow->addWidget(mNetScanStopBtn);
    parentLayout->addLayout(subnetRow);

    // ── Status + progress bar ───────────────────────────────────────────
    mNetScanStatusLbl = new QLabel();
    mNetScanStatusLbl->setStyleSheet(
        QStringLiteral("color: %1; font-size: 12px;").arg(C_TEXT_DIM));
    mNetScanStatusLbl->setText(QStringLiteral(
        "Ready. Enter a subnet prefix and click Discover."));
    parentLayout->addWidget(mNetScanStatusLbl);

    mNetScanProgress = new QProgressBar();
    mNetScanProgress->setRange(0, 254);
    mNetScanProgress->setValue(0);
    mNetScanProgress->setTextVisible(true);
    mNetScanProgress->setFormat(QStringLiteral("%v / %m hosts"));
    parentLayout->addWidget(mNetScanProgress);

    // ── Results table ───────────────────────────────────────────────────
    mNetScanTable = new QTableWidget(0, 5);
    mNetScanTable->setHorizontalHeaderLabels({
        QStringLiteral("IP"),
        QStringLiteral("Hostname"),
        QStringLiteral("MAC"),
        QStringLiteral("Vendor"),
        QStringLiteral("Action"),
    });
    mNetScanTable->horizontalHeader()->setStretchLastSection(false);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    mNetScanTable->verticalHeader()->setVisible(false);
    applyTableStyle(mNetScanTable);
    parentLayout->addWidget(mNetScanTable, 1);
}

// ═══════════════════════════════════════════════════════════════════════════════
void ToolsTab::refreshSubnetPlaceholder() noexcept
{
    if (!mNetScanSubnetEdit) return;
    QStringList const subs = IPView::Scanner::NetworkDiscovery::detectLocalSubnets();
    if (subs.isEmpty()) return;
    mNetScanSubnetEdit->setPlaceholderText(
        QStringLiteral("e.g. %1 (empty = auto-detect)").arg(subs.first()));
}

// ═══════════════════════════════════════════════════════════════════════════════
void ToolsTab::addPortScanButton(int row, const QString &ip) noexcept
{
    auto *btn = new QPushButton(QStringLiteral("Port scan"));
    btn->setCursor(Qt::PointingHandCursor);
    btn->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %2; "
        "border-radius: %3; padding: %4; }"
    ).arg(C_BG_ELEVATED, C_PRIMARY, RADIUS_MD, PADDING_BTN));
    // Stash the IP on the button so the slot can recover it
    // without a per-row index lookup. setProperty is the
    // idiomatic Qt way to attach a payload to a QObject.
    btn->setProperty("targetIp", ip);
    connect(btn, &QPushButton::clicked,
            this, &ToolsTab::onPortScanButtonClicked);
    mNetScanTable->setCellWidget(row, 4, btn);
}

// ═══════════════════════════════════════════════════════════════════════════════
void ToolsTab::onPingClicked()
{
    QString const target = targetEdit->text().trimmed();

    // ── Security: Input validation against command injection ───────────
    if (!isValidNetworkTarget(target)) {
        outputArea->append(QStringLiteral("Invalid target. Enter a valid IP or hostname."));
        return;
    }

    if (pingProcess && pingProcess->state() != QProcess::NotRunning) {
        return;   // already running
    }

    // Resolve absolute path via QStandardPaths (PATH-hijack safe).
    QString const pingPath = QStandardPaths::findExecutable(QStringLiteral("ping"));
    if (pingPath.isEmpty()) {
        outputArea->append(QStringLiteral("ping: command not found in PATH."));
        return;
    }

    if (!pingProcess) {
        pingProcess = new QProcess(this);
        connect(pingProcess, &QProcess::readyReadStandardOutput, this, [this]() {
            outputArea->append(
                QString::fromLocal8Bit(pingProcess->readAllStandardOutput()));
        });
        connect(pingProcess, &QProcess::readyReadStandardError, this, [this]() {
            outputArea->append(
                QString::fromLocal8Bit(pingProcess->readAllStandardError()));
        });
        connect(pingProcess,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this,
                [this](int /*code*/, QProcess::ExitStatus /*status*/) {
                    pingButton->setEnabled(true);
                    stopPingButton->setEnabled(false);
                });
    }

    pingButton->setEnabled(false);
    stopPingButton->setEnabled(true);
    outputArea->append(QStringLiteral("\n── Pinging %1 ──").arg(target));
    pingProcess->start(pingPath, {
        QStringLiteral("-c"), QStringLiteral("4"),
        QStringLiteral("-W"), QStringLiteral("2"),
        target
    });
}

void ToolsTab::onStopPingClicked()
{
    if (pingProcess && pingProcess->state() != QProcess::NotRunning) {
        pingProcess->kill();
        pingProcess->waitForFinished(
            static_cast<int>(IPView::Timeouts::PROCESS_QUIT_SLOW.count()));
        outputArea->append(QStringLiteral("Ping stopped."));
    }
    pingButton->setEnabled(true);
    stopPingButton->setEnabled(false);
}

// ═══════════════════════════════════════════════════════════════════════════════
void ToolsTab::onIperfClicked()
{
    if (!mIperfWindow) {
        // Create the Iperf3Window once and embed as a tab
        mIperfWindow = new Iperf3Window(this);
        mIperfWindow->setEmbeddedMode(true);
        mIperfWindow->setWindowTitle(QStringLiteral("iPerf3 Bandwidth Test"));
        mIperfTabIndex = mToolsTabWidget->addTab(mIperfWindow,
            QIcon(QStringLiteral(":/svgs/lightning.svg")),
            QStringLiteral("iPerf3"));
    }
    // Switch to the iPerf3 tab
    mToolsTabWidget->setCurrentWidget(mIperfWindow);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Network scan / device discovery slots
// ═══════════════════════════════════════════════════════════════════════════════

void ToolsTab::onNetScanStartClicked()
{
    if (mNetDiscovery->isRunning()) return;   // already running

    QString subnet = mNetScanSubnetEdit->text().trimmed();

    mNetScanTable->setRowCount(0);
    mNetScanFoundCount = 0;
    mNetScanProgress->setValue(0);
    mNetScanStatusLbl->setText(QStringLiteral(
        "▶ Discovering devices…"));
    mNetScanStatusLbl->setStyleSheet(
        QStringLiteral("color: %1; font-size: 12px;").arg(C_INFO));
    mNetScanStartBtn->setEnabled(false);
    mNetScanStopBtn->setEnabled(true);
    mNetScanSubnetEdit->setReadOnly(true);

    mNetDiscovery->startDiscovery(subnet);
}

void ToolsTab::onNetScanStopClicked()
{
    if (!mNetDiscovery->isRunning()) return;
    mNetDiscovery->cancel();
    mNetScanStatusLbl->setText(QStringLiteral("■ Discovery cancelled."));
    mNetScanStatusLbl->setStyleSheet(
        QStringLiteral("color: %1; font-size: 12px;").arg(C_WARNING));
    mNetScanStartBtn->setEnabled(true);
    mNetScanStopBtn->setEnabled(false);
    mNetScanSubnetEdit->setReadOnly(false);
}

void ToolsTab::onNetScanDeviceFound(
    const IPView::Scanner::DiscoveredDevice &device) noexcept
{
    int const row = mNetScanTable->rowCount();
    mNetScanTable->insertRow(row);
    mNetScanFoundCount++;

    // Column 0: IP
    auto *ipItem = new QTableWidgetItem(device.ip);
    ipItem->setForeground(QBrush(QColor(C_PRIMARY)));
    ipItem->setFont(QFont(QStringLiteral("Segoe UI"), 10, QFont::Bold));
    mNetScanTable->setItem(row, 0, ipItem);

    // Column 1: Hostname (or "—" if not resolved)
    auto *hostItem = new QTableWidgetItem(
        device.hostname.isEmpty() ? QStringLiteral("—") : device.hostname);
    mNetScanTable->setItem(row, 1, hostItem);

    // Column 2: MAC (or "—" if not in ARP cache)
    auto *macItem = new QTableWidgetItem(
        device.mac.isEmpty() ? QStringLiteral("—") : device.mac);
    macItem->setTextAlignment(Qt::AlignCenter);
    mNetScanTable->setItem(row, 2, macItem);

    // Column 3: Vendor (or "—" if OUI unknown)
    auto *vendorItem = new QTableWidgetItem(
        device.vendor.isEmpty() ? QStringLiteral("—") : device.vendor);
    mNetScanTable->setItem(row, 3, vendorItem);

    // Column 4: Port-scan button
    addPortScanButton(row, device.ip);

    mNetScanStatusLbl->setText(QStringLiteral(
        "▶ Scanning… %1 device(s) found so far").arg(mNetScanFoundCount));
}

void ToolsTab::onNetScanProgress(int scanned, int total) noexcept
{
    mNetScanProgress->setMaximum(qMax(total, 1));
    mNetScanProgress->setValue(scanned);
}

void ToolsTab::onNetScanCompleted() noexcept
{
    mNetScanStartBtn->setEnabled(true);
    mNetScanStopBtn->setEnabled(false);
    mNetScanSubnetEdit->setReadOnly(false);
    mNetScanStatusLbl->setText(QStringLiteral(
        "✓ Discovery complete — %1 device(s) found.")
        .arg(mNetScanFoundCount));
    mNetScanStatusLbl->setStyleSheet(
        QStringLiteral("color: %1; font-size: 12px;").arg(C_SUCCESS));
}

void ToolsTab::onNetScanError(const QString &message) noexcept
{
    mNetScanStartBtn->setEnabled(true);
    mNetScanStopBtn->setEnabled(false);
    mNetScanSubnetEdit->setReadOnly(false);
    mNetScanStatusLbl->setText(QStringLiteral("⚠ %1").arg(message));
    mNetScanStatusLbl->setStyleSheet(
        QStringLiteral("color: %1; font-size: 12px;").arg(C_ERROR));
}

void ToolsTab::onNetScanCancelled() noexcept
{
    mNetScanStartBtn->setEnabled(true);
    mNetScanStopBtn->setEnabled(false);
    mNetScanSubnetEdit->setReadOnly(false);
}

void ToolsTab::onPortScanButtonClicked()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString const ip = btn->property("targetIp").toString();
    if (ip.isEmpty()) return;
    IPView::Logger::info("ToolsTab: port scan requested for %s",
                         qPrintable(ip));
    emit portScanRequested(ip);
}
