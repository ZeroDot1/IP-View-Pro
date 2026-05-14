// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — ToolsTab.cpp
//  C++26: auto, QStringLiteral, [[maybe_unused]], const-correctness
//  Ping and iPerf3 tools with QProcess management.
// ═══════════════════════════════════════════════════════════════════════════════

#include "ToolsTab.h"
#include "Theme.h"
#include "SecurityUtil.h"
#include "Iperf3Window.h"
#include "TracerouteTab.h"

#include <QDateTime>
#include <QTimer>

// ═══════════════════════════════════════════════════════════════════════════════
ToolsTab::ToolsTab(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    auto *toolsTabWidget = new QTabWidget();

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

    toolsTabWidget->addTab(pingIperfTab, QStringLiteral("Ping / iPerf3"));
    toolsTabWidget->addTab(trace,        QStringLiteral("Traceroute"));

    mainLayout->addWidget(toolsTabWidget);

    // ── Connections ───────────────────────────────────────────────────────
    connect(pingButton,    &QPushButton::clicked, this, &ToolsTab::onPingClicked);
    connect(stopPingButton, &QPushButton::clicked, this, &ToolsTab::onStopPingClicked);
    connect(iperfButton,   &QPushButton::clicked, this, &ToolsTab::onIperfClicked);
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
        pingProcess->waitForFinished(1000);
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
    auto *iperf = new Iperf3Window(this);
    iperf->show();
}
