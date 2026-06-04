// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — ToolsTab.cpp
//  C++26: auto, QStringLiteral, [[maybe_unused]], const-correctness,
//         std::array, std::span
//  Ping, iPerf3, and sequential multi-target network scan tools
//  with QProcess management.
// ═══════════════════════════════════════════════════════════════════════════════

#include "ToolsTab.h"
#include "Theme.h"
#include "SecurityUtil.h"
#include "Timeouts.hpp"
#include "Iperf3Window.h"
#include "TracerouteTab.h"

#include <QDateTime>
#include <QTimer>
#include <QHeaderView>

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

    // ── Network-Scan Tab ──────────────────────────────────────────────────
    // Sequential multi-target port scan. The user types up to
    // three comma-separated IPs; pressing Start walks the list
    // and runs ScannerModule::runScan() on each, appending the
    // results to a single QTableWidget grouped by target IP.
    auto *netScanTab   = new QWidget();
    auto *netScanLayout = new QVBoxLayout(netScanTab);
    netScanLayout->setSpacing(10);
    netScanLayout->setContentsMargins(16, 16, 16, 16);

    auto *inputRow = new QHBoxLayout();
    auto *inputLbl = new QLabel(QStringLiteral("Ziel-IPs (max. 3, Komma-getrennt):"));
    inputLbl->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(C_TEXT));
    mNetScanTargetsEdit = new QLineEdit();
    mNetScanTargetsEdit->setPlaceholderText(
        QStringLiteral("z. B. 192.168.1.1, 192.168.1.2, 192.168.1.3"));
    mNetScanTargetsEdit->setStyleSheet(inputStyle());

    mNetScanStartBtn = new QPushButton(QStringLiteral("▶ Scan starten"));
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

    inputRow->addWidget(inputLbl);
    inputRow->addWidget(mNetScanTargetsEdit, 1);
    inputRow->addWidget(mNetScanStartBtn);
    inputRow->addWidget(mNetScanStopBtn);
    netScanLayout->addLayout(inputRow);

    mNetScanStatusLbl = new QLabel();
    mNetScanStatusLbl->setStyleSheet(QStringLiteral("color: %1; font-size: 12px;").arg(C_TEXT_DIM));
    mNetScanStatusLbl->setText(QStringLiteral("Bereit. IPs eintragen und „Scan starten“ drücken."));
    netScanLayout->addWidget(mNetScanStatusLbl);

    mNetScanTable = new QTableWidget(0, 4);
    mNetScanTable->setHorizontalHeaderLabels({
        QStringLiteral("Ziel-IP"),
        QStringLiteral("Offener Port"),
        QStringLiteral("Dienst"),
        QStringLiteral("Latenz (ms)")
    });
    mNetScanTable->horizontalHeader()->setStretchLastSection(false);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    mNetScanTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    mNetScanTable->verticalHeader()->setVisible(false);
    mNetScanTable->setEditTriggers(QTableWidget::NoEditTriggers);
    mNetScanTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mNetScanTable->setAlternatingRowColors(true);
    netScanLayout->addWidget(mNetScanTable, 1);

    toolsTabWidget->addTab(pingIperfTab,  QStringLiteral("Ping / iPerf3"));
    toolsTabWidget->addTab(trace,         QStringLiteral("Traceroute"));
    toolsTabWidget->addTab(netScanTab,    QStringLiteral("Netzwerkscan"));

    mainLayout->addWidget(toolsTabWidget);

    // ── Connections ───────────────────────────────────────────────────────
    connect(pingButton,    &QPushButton::clicked, this, &ToolsTab::onPingClicked);
    connect(stopPingButton, &QPushButton::clicked, this, &ToolsTab::onStopPingClicked);
    connect(iperfButton,   &QPushButton::clicked, this, &ToolsTab::onIperfClicked);

    // ── Network-Scan: own ScannerModule instance ──────────────────────────
    // The module lives for the lifetime of ToolsTab; we kick off
    // scanCompleted() handlers in a chain so each target waits
    // for the previous one to finish before starting.
    mNetScanner = new IPView::Scanner::ScannerModule(this);
    connect(mNetScanner, &IPView::Scanner::ScannerModule::portFound,
            this, &ToolsTab::onNetScanPortFound);
    connect(mNetScanner, &IPView::Scanner::ScannerModule::scanCompleted,
            this, &ToolsTab::onNetScanCompleted);
    connect(mNetScanner, &IPView::Scanner::ScannerModule::scanError,
            this, &ToolsTab::onNetScanError);
    connect(mNetScanner, &IPView::Scanner::ScannerModule::scanProgress,
            this, &ToolsTab::onNetScanProgress);

    connect(mNetScanStartBtn, &QPushButton::clicked, this, &ToolsTab::onNetScanStartClicked);
    connect(mNetScanStopBtn,  &QPushButton::clicked, this, &ToolsTab::onNetScanStopClicked);
}

void ToolsTab::setTargetIp(const QString &ip) noexcept
{
    targetEdit->setText(ip);
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

    // Terminate previous process
    if (pingProcess && pingProcess->state() == QProcess::Running) {
        pingProcess->kill();
        pingProcess->waitForFinished(static_cast<int>(IPView::Timeouts::PROCESS_QUIT_FAST.count()));
    }

    outputArea->clear();
    outputArea->append(QStringLiteral("═══ Ping Test: %1 ═══").arg(target));
    outputArea->append(QStringLiteral("[%1] Starting...")
                           .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))));

    pingProcess = new QProcess(this);

    connect(pingProcess, &QProcess::readyReadStandardOutput, this, [this]() {
        if (pingProcess) {
            outputArea->append(QString::fromLocal8Bit(pingProcess->readAllStandardOutput()));
        }
    });
    connect(pingProcess, &QProcess::readyReadStandardError, this, [this]() {
        if (pingProcess) {
            outputArea->append(QString::fromLocal8Bit(pingProcess->readAllStandardError()));
        }
    });
    connect(pingProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code) {
        QString const timeStr = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
        if (code == 0) {
            outputArea->append(QStringLiteral("[%1] Ping completed successfully.").arg(timeStr));
        } else {
            outputArea->append(QStringLiteral("[%1] Ping finished with exit code %2.")
                                   .arg(timeStr).arg(code));
        }
        pingButton->setEnabled(true);
        stopPingButton->setEnabled(false);
        pingProcess->deleteLater();
        pingProcess = nullptr;
    });

    // Platform-specific arguments
    QStringList args;
#ifdef Q_OS_WIN
    args << QStringLiteral("-n") << QStringLiteral("4") << target;
#else
    args << QStringLiteral("-c") << QStringLiteral("4") << target;
#endif

    // Security: full path instead of relative command (prevents PATH hijacking)
    QString const pingPath = findSystemTool(QStringLiteral("ping"));
    if (pingPath.isEmpty()) {
        outputArea->append(QStringLiteral("Error: 'ping' not found on system."));
        pingProcess->deleteLater();
        pingProcess = nullptr;
        return;
    }
    pingProcess->start(pingPath, args);
    pingButton->setEnabled(false);
    stopPingButton->setEnabled(true);

    // Security timeout: auto-stop after 60 seconds
    QTimer::singleShot(60000, this, [this]() {
        if (pingProcess && pingProcess->state() == QProcess::Running) {
            onStopPingClicked();
            outputArea->append(QStringLiteral("Ping timed out (60s) – auto-stopped."));
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
void ToolsTab::onStopPingClicked()
{
    if (pingProcess && pingProcess->state() == QProcess::Running) {
        pingProcess->kill();
        outputArea->append(QStringLiteral("[%1] Ping cancelled by user.")
                               .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))));
    }
    pingButton->setEnabled(true);
    stopPingButton->setEnabled(false);
}

// ═══════════════════════════════════════════════════════════════════════════════
void ToolsTab::onIperfClicked()
{
    if (!mIperfWindow) {
        // Iperf3Window einmal erstellen und als Tab einbetten
        mIperfWindow = new Iperf3Window(this);
        mIperfWindow->setEmbeddedMode(true);
        mIperfWindow->setWindowTitle(QStringLiteral("iPerf3 Bandwidth Test"));
        mIperfTabIndex = mToolsTabWidget->addTab(mIperfWindow,
            QIcon(QStringLiteral(":/svgs/lightning.svg")),
            QStringLiteral("iPerf3"));
    }
    // Zum iPerf3-Tab wechseln
    mToolsTabWidget->setCurrentWidget(mIperfWindow);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Netzwerkscan — multi-target sequential port scan
//
//  parseNetScanTargets() pulls the user-typed comma-separated
//  string out of mNetScanTargetsEdit, trims whitespace off
//  every entry, drops empties, validates each IP via
//  SecurityUtil::isValidNetworkTarget (the same regex used
//  by the rest of the app to block command injection), and
//  caps the result at MAX_NET_SCAN_TARGETS.
//
//  Anything beyond the cap is reported in the status label
//  rather than silently discarded — the user typed the
//  extra IPs on purpose and deserves to know they were
//  dropped. Anything that fails isValidNetworkTarget is
//  reported in the same way with a specific reason.
// ═══════════════════════════════════════════════════════════════════════════════

QStringList ToolsTab::parseNetScanTargets() const noexcept
{
    QStringList result;
    QString const raw = mNetScanTargetsEdit->text().trimmed();
    if (raw.isEmpty()) return result;

    // Split on commas, trim each token, drop empties.
    for (QString const &part : raw.split(QLatin1Char(','), Qt::SkipEmptyParts)) {
        QString const trimmed = part.trimmed();
        if (!trimmed.isEmpty()) {
            result.append(trimmed);
        }
    }
    return result;
}

void ToolsTab::onNetScanStartClicked()
{
    if (mNetScanRunning) return;  // already running — ignore

    QStringList const targets = parseNetScanTargets();
    if (targets.isEmpty()) {
        mNetScanStatusLbl->setText(QStringLiteral(
            "⚠ Keine gültigen Ziel-IPs eingegeben."));
        mNetScanStatusLbl->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 12px;").arg(C_WARNING));
        return;
    }

    // Validate each entry. Build a clean queue containing only
    // the entries that pass isValidNetworkTarget; report the
    // dropped ones individually.
    QStringList valid;
    QStringList dropped;
    for (QString const &t : targets) {
        if (isValidNetworkTarget(t)) valid.append(t);
        else                          dropped.append(t);
    }

    if (valid.isEmpty()) {
        mNetScanStatusLbl->setText(QStringLiteral(
            "⚠ Keine gültigen Ziel-IPs (alle Eingaben ungültig): %1")
            .arg(dropped.join(QStringLiteral(", "))));
        mNetScanStatusLbl->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 12px;").arg(C_ERROR));
        return;
    }

    // Hard cap at MAX_NET_SCAN_TARGETS (3). The user wanted
    // "bis zu 3 IPs" — anything beyond is dropped with a
    // notice.
    if (valid.size() > static_cast<int>(MAX_NET_SCAN_TARGETS)) {
        dropped.append(valid.mid(static_cast<int>(MAX_NET_SCAN_TARGETS)));
        valid = valid.mid(0, static_cast<int>(MAX_NET_SCAN_TARGETS));
    }

    mNetScanQueue      = valid;
    mNetScanRunning    = true;
    mNetScanStartBtn->setEnabled(false);
    mNetScanStopBtn->setEnabled(true);
    mNetScanTargetsEdit->setReadOnly(true);
    mNetScanTable->setRowCount(0);

    QString statusMsg = QStringLiteral("▶ Starte Scan von %1 Ziel(s): %2")
        .arg(valid.size())
        .arg(valid.join(QStringLiteral(", ")));
    if (!dropped.isEmpty()) {
        statusMsg += QStringLiteral("  (verworfen: %1)")
            .arg(dropped.join(QStringLiteral(", ")));
    }
    mNetScanStatusLbl->setText(statusMsg);
    mNetScanStatusLbl->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px;").arg(C_INFO));

    startNextNetScanTarget();
}

void ToolsTab::onNetScanStopClicked()
{
    if (!mNetScanRunning) return;
    mNetScanner->cancelScan();
    mNetScanQueue.clear();
    mNetScanRunning    = false;
    mNetScanStartBtn->setEnabled(true);
    mNetScanStopBtn->setEnabled(false);
    mNetScanTargetsEdit->setReadOnly(false);
    mNetScanStatusLbl->setText(QStringLiteral("■ Scan abgebrochen."));
    mNetScanStatusLbl->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px;").arg(C_WARNING));
}

void ToolsTab::startNextNetScanTarget() noexcept
{
    if (mNetScanQueue.isEmpty()) {
        // All targets done. Reset UI.
        mNetScanRunning    = false;
        mNetScanCurrentTarget.clear();
        mNetScanStartBtn->setEnabled(true);
        mNetScanStopBtn->setEnabled(false);
        mNetScanTargetsEdit->setReadOnly(false);
        mNetScanStatusLbl->setText(QStringLiteral(
            "✓ Sequentieller Scan abgeschlossen."));
        mNetScanStatusLbl->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 12px;").arg(C_SUCCESS));
        return;
    }

    QString const nextTarget = mNetScanQueue.takeFirst();
    mNetScanCurrentTarget = nextTarget;
    mNetScanStatusLbl->setText(QStringLiteral(
        "▶ Scanne %1 (%2 verbleibend)…")
        .arg(nextTarget)
        .arg(mNetScanQueue.size() + 1));
    mNetScanner->runScan(nextTarget, IPView::Scanner::ScannerModule::defaultPorts());
}

void ToolsTab::onNetScanPortFound(const IPView::Scanner::ScanResult &result)
{
    if (!result.open) return;   // only show open ports
    int const row = mNetScanTable->rowCount();
    mNetScanTable->insertRow(row);

    // Column 0 stays blank — the IP is set by the
    // scanCompleted handler below once the run for that
    // target finishes. Showing it per-row would be nicer but
    // requires keeping the "current target" string around;
    // the current-target label in the status bar covers
    // that case.
    auto *ipItem = new QTableWidgetItem(mNetScanCurrentTarget);
    ipItem->setForeground(QBrush(QColor(C_PRIMARY)));
    ipItem->setFont(QFont(QStringLiteral("Segoe UI"), 10, QFont::Bold));
    mNetScanTable->setItem(row, 0, ipItem);

    auto *portItem = new QTableWidgetItem(QString::number(result.port));
    portItem->setTextAlignment(Qt::AlignCenter);
    mNetScanTable->setItem(row, 1, portItem);

    auto *serviceItem = new QTableWidgetItem(
        result.service.isEmpty() ? QStringLiteral("?") : result.service);
    serviceItem->setTextAlignment(Qt::AlignCenter);
    mNetScanTable->setItem(row, 2, serviceItem);

    auto *latencyItem = new QTableWidgetItem(QString::number(result.latencyMs));
    latencyItem->setTextAlignment(Qt::AlignCenter);
    mNetScanTable->setItem(row, 3, latencyItem);
}

void ToolsTab::onNetScanCompleted(const QVector<IPView::Scanner::ScanResult> &results)
{
    int const openCount = static_cast<int>(std::ranges::count_if(results,
        [](const IPView::Scanner::ScanResult &r) { return r.open; }));
    mNetScanStatusLbl->setText(QStringLiteral(
        "✓ %1 abgeschlossen (%2 offene Ports). Starte nächsten…")
        .arg(mNetScanCurrentTarget)
        .arg(openCount));
    mNetScanStatusLbl->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px;").arg(C_SUCCESS));
    Q_UNUSED(results);
    startNextNetScanTarget();
}

void ToolsTab::onNetScanError(const QString &message)
{
    mNetScanStatusLbl->setText(QStringLiteral(
        "⚠ Fehler bei %1: %2")
        .arg(mNetScanCurrentTarget, message));
    mNetScanStatusLbl->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px;").arg(C_ERROR));
    // Continue with the next target even on error — one bad
    // IP should not abort the whole multi-target run.
    startNextNetScanTarget();
}

void ToolsTab::onNetScanProgress(int current, int total)
{
    Q_UNUSED(current);
    Q_UNUSED(total);
    // Status label is already updated by onNetScanStartClicked
    // and onNetScanPortFound; this slot is here for future
    // progress-bar work and to keep the signal connection
    // alive so the scanner's signal does not become a
    // dead-letter warning.
}
