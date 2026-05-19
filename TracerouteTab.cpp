// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — TracerouteTab.cpp
//  C++26: [[nodiscard]], auto, QStringLiteral, const-correctness
//  Traceroute/tracepath/tracert with cross-platform support.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TracerouteTab.h"
#include "Theme.h"
#include "SecurityUtil.h"
#include <QDateTime>
#include <QStandardPaths>
#include <QProcessEnvironment>
#include <QFileInfo>

// ═══════════════════════════════════════════════════════════════════════════════
TracerouteTab::TracerouteTab(QWidget *parent)
    : QWidget(parent)
    , process(new QProcess(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    auto *topRow = new QHBoxLayout();
    targetEdit = new QLineEdit();
    targetEdit->setPlaceholderText(QStringLiteral("Target IP/Host..."));
    targetEdit->setStyleSheet(inputStyle());

    traceButton = new QPushButton(QStringLiteral("▶ Traceroute"));
    traceButton->setProperty("accent", true);
    traceButton->setStyleSheet(btnAccentStyle());

    stopButton = new QPushButton(QStringLiteral("■ Stop"));
    stopButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %2; "
        "border-radius: %3; padding: %4; }"
    ).arg(C_BG_ELEVATED, C_ERROR, RADIUS_MD, PADDING_BTN));
    stopButton->setEnabled(false);
    topRow->addWidget(targetEdit);
    topRow->addWidget(traceButton);
    topRow->addWidget(stopButton);
    topRow->addStretch();

    outputArea = new QTextEdit();
    outputArea->setReadOnly(true);
    outputArea->setStyleSheet(monoStyle());

    layout->addLayout(topRow);
    layout->addWidget(outputArea);

    // ── Connections ───────────────────────────────────────────────────────
    connect(traceButton, &QPushButton::clicked, this, &TracerouteTab::onTraceClicked);
    connect(stopButton,  &QPushButton::clicked, this, &TracerouteTab::onStopTraceClicked);
    connect(process, &QProcess::readyReadStandardOutput, this, &TracerouteTab::onDataReceived);
    connect(process, &QProcess::readyReadStandardError,  this, &TracerouteTab::onDataReceived);
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TracerouteTab::onTraceFinished);
}

void TracerouteTab::setTargetIp(const QString &ip) noexcept
{
    targetEdit->setText(ip);
}

// ═══════════════════════════════════════════════════════════════════════════════
[[nodiscard]]
QString TracerouteTab::findTraceroute() const noexcept
{
#ifdef Q_OS_WIN
    // tracert is always in %SYSTEMROOT%\System32
    QString const systemRoot = QProcessEnvironment::systemEnvironment()
                                   .value(QStringLiteral("SYSTEMROOT"), QStringLiteral("C:\\Windows"));
    QString const tracertPath = systemRoot + QStringLiteral("\\System32\\tracert.exe");
    if (QFileInfo::exists(tracertPath)) return tracertPath;
    return QStringLiteral("tracert");
#else
        // Security: resolve full path (PATH hijacking protection)
    QString prog = findSystemTool(QStringLiteral("traceroute"));
    if (prog.isEmpty()) {
        prog = findSystemTool(QStringLiteral("tracepath"));
    }
    return prog;
#endif
}

// ═══════════════════════════════════════════════════════════════════════════════
void TracerouteTab::onTraceClicked()
{
    QString const target = targetEdit->text().trimmed();

    // ── Security: Input validation against command injection ──────────
    if (!isValidNetworkTarget(target)) {
        outputArea->append(QStringLiteral("Invalid target. Enter a valid IP or hostname."));
        return;
    }

    if (process->state() == QProcess::Running) {
        process->kill();
        process->waitForFinished(500);
    }

    outputArea->clear();
    outputArea->setPlainText(QStringLiteral("═══ Traceroute to %1 ═══").arg(target));
    outputArea->append(QStringLiteral("[%1] Starting...")
                           .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))));

    QString const program = findTraceroute();
    if (program.isEmpty()) {
        outputArea->append(QStringLiteral("Error: 'traceroute' / 'tracepath' / 'tracert' not found."));
        outputArea->append(QStringLiteral("  Linux: sudo pacman -S traceroute"));
        outputArea->append(QStringLiteral("  Windows: tracert is built-in"));
        return;
    }

    QStringList args;
#ifdef Q_OS_WIN
    args << QStringLiteral("-d") << target;   // -d: no DNS resolution
#else
    args << QStringLiteral("-n") << target;   // -n: no DNS resolution
#endif

    process->start(program, args);
    traceButton->setEnabled(false);
    stopButton->setEnabled(true);
}

// ═══════════════════════════════════════════════════════════════════════════════
void TracerouteTab::onStopTraceClicked()
{
    if (process->state() == QProcess::Running) {
        process->kill();
        outputArea->append(QStringLiteral("[%1] Traceroute cancelled by user.")
                               .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))));
    }
    traceButton->setEnabled(true);
    stopButton->setEnabled(false);
}

// ═══════════════════════════════════════════════════════════════════════════════
void TracerouteTab::onDataReceived()
{
    // Read both stdout and stderr — the signal may fire for either channel
    QString const outData = QString::fromLocal8Bit(process->readAllStandardOutput());
    QString const errData = QString::fromLocal8Bit(process->readAllStandardError());

    if (!outData.trimmed().isEmpty()) {
        outputArea->append(outData.trimmed());
    }
    if (!errData.trimmed().isEmpty()) {
        outputArea->append(QStringLiteral("[stderr] ") + errData.trimmed());
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
void TracerouteTab::onTraceFinished(int exitCode)
{
    outputArea->append(QStringLiteral("[%1] Traceroute %2.")
                           .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")))
                           .arg(exitCode == 0 ? QStringLiteral("completed") : QStringLiteral("finished with error")));
    traceButton->setEnabled(true);
    stopButton->setEnabled(false);
}