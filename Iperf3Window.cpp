// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — Iperf3Window.cpp
//  C++26: auto, QStringLiteral, const-correctness, structured bindings
//  iPerf3 Client/Server with real-time speed display.
// ═══════════════════════════════════════════════════════════════════════════════

#include "Iperf3Window.h"
#include "Theme.h"
#include "SecurityUtil.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QTimer>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDateTime>
#include <QRegularExpression>

// ═══════════════════════════════════════════════════════════════════════════════
Iperf3Window::Iperf3Window(QWidget *parent)
    : QDialog(parent)
    , iperf3Process(new QProcess(this))
    , statsTimer(new QTimer(this))
{
    setupUI();

    connect(iperf3Process, &QProcess::readyReadStandardOutput,
            this, &Iperf3Window::onIperf3ReadyRead);
    connect(iperf3Process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Iperf3Window::onIperf3Finished);
    connect(statsTimer, &QTimer::timeout,
            this, &Iperf3Window::updateStats);
    connect(this, &Iperf3Window::speedUpdated,
            this, &Iperf3Window::handleSpeedUpdated);
}

Iperf3Window::~Iperf3Window()
{
    if (iperf3Process->state() == QProcess::Running) {
        iperf3Process->terminate();
        iperf3Process->waitForFinished(2000);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Embedded mode (Item 8)
// ═══════════════════════════════════════════════════════════════════════════════

void Iperf3Window::setEmbeddedMode(bool embedded) noexcept
{
    mEmbedded = embedded;
    if (embedded) {
        // Embed as widget without window decorations
        setWindowFlags(Qt::Widget);
        setParent(parentWidget());
    } else {
        // Show as normal dialog window
        setWindowFlags(Qt::Dialog);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  UI Setup
// ═══════════════════════════════════════════════════════════════════════════════

void Iperf3Window::setupUI()
{
    setWindowTitle(QStringLiteral("IPerf3 Network Test"));
    setFixedSize(600, 500);

    // ── Header ────────────────────────────────────────────────────────────
    auto *headerFrame = new QFrame();
    headerFrame->setStyleSheet(QStringLiteral(
        "background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 %1, stop:1 #16213e); border-radius: 8px;"
    ).arg(C_BG_ELEVATED));
    headerFrame->setFixedHeight(50);

    auto *titleLabel = new QLabel(QStringLiteral("IPerf3 Network Test"));
    titleLabel->setFont(QFont(QStringLiteral("Segoe UI"), 14, QFont::Bold));
    titleLabel->setStyleSheet(QStringLiteral("color: %1; padding: 10px;").arg(C_ACCENT));

    auto *headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    // ── Content ───────────────────────────────────────────────────────────
    auto *contentFrame = new QFrame();
    contentFrame->setStyleSheet(QStringLiteral(
        "background-color: %1; border-radius: 12px;"
    ).arg(C_BG));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->addWidget(headerFrame);
    mainLayout->addWidget(contentFrame);

    auto *contentLayout = new QVBoxLayout(contentFrame);
    contentLayout->setSpacing(15);
    contentLayout->setContentsMargins(25, 25, 25, 25);

    // ── IP Display ────────────────────────────────────────────────────────
    auto *topRow = new QHBoxLayout();
    auto *ipTextLabel = new QLabel(QStringLiteral("Target IP:"));
    ipTextLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT));
    ipLabel = new QLabel();
    ipLabel->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(C_PRIMARY));
    ipLabel->setText(QStringLiteral("(Click button to get from IP View)"));
    ipTextLabel->setFixedWidth(80);
    topRow->addWidget(ipTextLabel);
    topRow->addWidget(ipLabel);

    // ── Controls ─────────────────────────────────────────────────────────
    auto *controlRow = new QHBoxLayout();
    roleCombo = new QComboBox();
    roleCombo->addItems({QStringLiteral("Client"), QStringLiteral("Server")});
    roleCombo->setFixedWidth(120);
    hostEdit = new QLineEdit();
    hostEdit->setStyleSheet(inputStyle());
    hostEdit->setPlaceholderText(QStringLiteral("Enter server IP for client mode"));
    hostEdit->setText(QStringLiteral("localhost"));

    auto *modeLabel = new QLabel(QStringLiteral("Mode:"));
    modeLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT));
    auto *hostLabel = new QLabel(QStringLiteral("Host:"));
    hostLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT));

    controlRow->addWidget(modeLabel);
    controlRow->addWidget(roleCombo);
    controlRow->addSpacing(10);
    controlRow->addWidget(hostLabel);
    controlRow->addWidget(hostEdit);

    // ── Buttons ───────────────────────────────────────────────────────────
    auto *buttonRow = new QHBoxLayout();
    startButton = new QPushButton(QStringLiteral("Start Test"));
    startButton->setProperty("accent", true);
    startButton->setStyleSheet(btnAccentStyle());
    startButton->setFixedSize(120, 35);

    stopButton = new QPushButton(QStringLiteral("Stop"));
    stopButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %2; "
        "border-radius: %3; padding: 8px 16px; }"
    ).arg(C_BG_ELEVATED, C_ACCENT, RADIUS_MD));
    stopButton->setFixedSize(120, 35);
    stopButton->setEnabled(false);

    buttonRow->addStretch();
    buttonRow->addWidget(startButton);
    buttonRow->addSpacing(10);
    buttonRow->addWidget(stopButton);

    // ── Speed Bar ─────────────────────────────────────────────────────────
    speedBar = new QProgressBar();
    speedBar->setRange(0, 1000);
    speedBar->setValue(0);
    speedBar->setStyleSheet(QStringLiteral(
        "QProgressBar { background: %1; border: 1px solid %2; "
        "border-radius: 6px; text-align: center; color: %3; font-weight: bold; }"
        "QProgressBar::chunk { background: %2; border-radius: 5px; }"
    ).arg(C_BG_ELEVATED, C_ACCENT, C_TEXT));

    speedLabel = new QLabel(QStringLiteral("Speed: 0.00 Mbps"));
    speedLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 12px; font-weight: bold;"
    ).arg(C_SUCCESS));

    transferLabel = new QLabel(QStringLiteral("Transferred: 0.00 MB"));
    transferLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT));

    timeLabel = new QLabel(QStringLiteral("Time: 00:00"));
    timeLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT_DIM));

    // ── Output ────────────────────────────────────────────────────────────
    outputEdit = new QTextEdit();
    outputEdit->setStyleSheet(QStringLiteral(
        "QTextEdit { background: %1; color: %2; border: 1px solid %3; "
        "border-radius: 6px; padding: 6px; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER));
    outputEdit->setReadOnly(true);
    outputEdit->setMinimumHeight(200);

    contentLayout->addLayout(topRow);
    contentLayout->addLayout(controlRow);
    contentLayout->addLayout(buttonRow);
    contentLayout->addWidget(speedBar);
    contentLayout->addWidget(speedLabel);
    contentLayout->addWidget(transferLabel);
    contentLayout->addWidget(timeLabel);
    contentLayout->addWidget(new QLabel(QStringLiteral("Output:")));
    contentLayout->addWidget(outputEdit);

    connect(startButton, &QPushButton::clicked, this, &Iperf3Window::onStartClicked);
    connect(stopButton,  &QPushButton::clicked, this, &Iperf3Window::onStopClicked);
}

// ═══════════════════════════════════════════════════════════════════════════════
void Iperf3Window::onStartClicked()
{
    if (isRunning) return;

    QString host = hostEdit->text().trimmed();

    // ── Security: Input validation against command injection ──────────
    if (roleCombo->currentText() == QStringLiteral("Client")) {
        if (host.isEmpty()) host = QStringLiteral("localhost");
        if (!isValidNetworkTarget(host)) {
            outputEdit->append(QStringLiteral("Invalid host. Enter a valid IP or hostname."));
            return;
        }
    }

    // ── Security: resolve full binary path (prevents PATH hijacking) ──
    QString const iperfPath = findSystemTool(QStringLiteral("iperf3"));
    if (iperfPath.isEmpty()) {
        outputEdit->append(QStringLiteral("Error: 'iperf3' not found. Install: sudo pacman -S iperf3"));
        return;
    }

    QStringList args;
    if (roleCombo->currentText() == QStringLiteral("Server")) {
        args << QStringLiteral("-s") << QStringLiteral("-P") << QStringLiteral("1");
    } else {
        args << QStringLiteral("-c") << host
             << QStringLiteral("-t") << QStringLiteral("30")
             << QStringLiteral("-i") << QStringLiteral("1")
             << QStringLiteral("-f") << QStringLiteral("m");
    }

    outputEdit->clear();
    outputEdit->append(QStringLiteral("Starting iperf3 test..."));
    outputEdit->append(QStringLiteral("Command: %1 %2").arg(iperfPath, args.join(QLatin1Char(' '))));

    iperf3Process->start(iperfPath, args);
    isRunning = true;
    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    roleCombo->setEnabled(false);
    hostEdit->setEnabled(false);

    totalTransfer = 0.0;
    startTime = QDateTime::currentMSecsSinceEpoch();
    statsTimer->start(1000);
}

// ═══════════════════════════════════════════════════════════════════════════════
void Iperf3Window::onStopClicked()
{
    if (!isRunning) return;

    if (iperf3Process->state() == QProcess::Running) {
        iperf3Process->terminate();
        iperf3Process->waitForFinished(1000);
    }
    statsTimer->stop();
    isRunning = false;
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    roleCombo->setEnabled(true);
    hostEdit->setEnabled(true);
}

// ═══════════════════════════════════════════════════════════════════════════════
void Iperf3Window::onIperf3ReadyRead()
{
    QByteArray const rawOutput = iperf3Process->readAllStandardOutput();
    QString const text = QString::fromLocal8Bit(rawOutput);
    outputEdit->append(text);
    parseLine(text);
}

// ═══════════════════════════════════════════════════════════════════════════════
void Iperf3Window::onIperf3Finished(int exitCode, QProcess::ExitStatus status)
{
    isRunning = false;
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    roleCombo->setEnabled(true);
    hostEdit->setEnabled(true);
    statsTimer->stop();

    if (status == QProcess::CrashExit) {
        outputEdit->append(QStringLiteral("\niperf3 process crashed (segfault or signal)."));
    } else if (exitCode == 0) {
        outputEdit->append(QStringLiteral("\nTest completed successfully."));
    } else {
        outputEdit->append(QStringLiteral("\nTest finished with exit code: %1").arg(exitCode));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
void Iperf3Window::parseLine(const QString &line) noexcept
{
    // iperf3 output format (client):
    // [ ID] Interval       Transfer      Bitrate
    // [  4]   0.00-1.00    sec  1.25 MBytes  10.5 Mbits/sec
    // We look for the final aggregate line with "sender" or "receiver"

    QStringList const parts = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                         Qt::SkipEmptyParts);

    for (qsizetype i = 0; i < parts.size(); ++i) {
        // Parse bitrate
        if (parts[i].contains(QStringLiteral("Mbits"))
            || parts[i].contains(QStringLiteral("Gbits")))
        {
            double const multiplier = parts[i].contains(QStringLiteral("Gbits")) ? 1000.0 : 1.0;
            QString cleaned = QString(parts[i]);
            cleaned.replace(QStringLiteral("Mbits"), QString())
                   .replace(QStringLiteral("Gbits"), QString())
                   .replace(QStringLiteral("/sec"),  QString());

            bool ok = false;
            double const speed = cleaned.toDouble(&ok) * multiplier;
            if (ok) {
                emit speedUpdated(speed);
            }
        }

        // Parse transfer (MBytes, GBytes)
        if (parts[i].contains(QStringLiteral("MBytes"))
            || parts[i].contains(QStringLiteral("GBytes")))
        {
            double const multiplier = parts[i].contains(QStringLiteral("GBytes")) ? 1024.0 : 1.0;
            QString cleaned = QString(parts[i]);
            cleaned.replace(QStringLiteral("MBytes"), QString())
                   .replace(QStringLiteral("GBytes"), QString());

            bool ok = false;
            double const transfer = cleaned.toDouble(&ok) * multiplier;
            if (ok) {
                totalTransfer += transfer;
            }
        }
    }

    // Summary detection
    if (line.contains(QStringLiteral("sender"))
        && line.contains(QStringLiteral("receiver")))
    {
        outputEdit->append(QStringLiteral("── Test summary ──"));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
void Iperf3Window::updateStats()
{
    qint64 const elapsed = QDateTime::currentMSecsSinceEpoch() - startTime;
    QString const timeStr = QStringLiteral("%1:%2")
                                .arg(elapsed / 60000, 2, 10, QLatin1Char('0'))
                                .arg((elapsed % 60000) / 1000, 2, 10, QLatin1Char('0'));
    timeLabel->setText(QStringLiteral("Time: ") + timeStr);
}

// ═══════════════════════════════════════════════════════════════════════════════
void Iperf3Window::handleSpeedUpdated(double speed)
{
    speedLabel->setText(QStringLiteral("Speed: %1 Mbps").arg(speed, 0, 'f', 2));
    speedBar->setValue(static_cast<int>(qMin(100.0, speed / 10.0)));
    transferLabel->setText(QStringLiteral("Transferred: %1 MB")
                               .arg(totalTransfer, 0, 'f', 2));
}

// ═══════════════════════════════════════════════════════════════════════════════
void Iperf3Window::updateVisualization(double speed) noexcept
{
    // Update speed bar with color-coded range:
    //   Green   (100+ Mbps)  — excellent
    //   Cyan    (50-100)     — good
    //   Orange  (10-50)      — fair
    //   Red     (0-10)       — poor
    int const pct = qBound(0, static_cast<int>(speed / 10.0), 100);
    speedBar->setValue(pct);

    QString color;
    if      (speed >= 100.0) color = C_SUCCESS; // green
    else if (speed >= 50.0)  color = C_PRIMARY; // cyan
    else if (speed >= 10.0)  color = C_WARNING; // orange
    else                     color = C_ERROR;   // red

    speedBar->setStyleSheet(QStringLiteral(
        "QProgressBar { background: %1; border: 1px solid %2;"
        "  border-radius: 6px; height: 20px; text-align: center;"
        "  color: %3; font-weight: bold; }"
        "QProgressBar::chunk { background: %2; border-radius: 5px; }"
    ).arg(C_BG_ELEVATED, color, C_TEXT));
}
