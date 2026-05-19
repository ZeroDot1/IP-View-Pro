// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — SpeedtestTab.cpp
//  C++26: auto, QStringLiteral, const-correctness, structured bindings
//  Professional speed test via Python speedtest-cli (sivel) with --json.
//  Uses ServerSelectionModule for server browsing and filtering.
// ═══════════════════════════════════════════════════════════════════════════════

#include "SpeedtestTab.h"
#include "Theme.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QHeaderView>
#include <QLineEdit>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTableWidget>
#include <QVBoxLayout>

// ═══════════════════════════════════════════════════════════════════════════════
SpeedtestTab::SpeedtestTab(QWidget *parent)
    : QWidget(parent)
    , process(new QProcess(this))
    , progressTimer(new QTimer(this))
    , serverSelector(new IPView::Speedtest::ServerSelectionModule(this))
{
    setupUI();

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &SpeedtestTab::onSpeedtestFinished);
    connect(process, &QProcess::readyReadStandardOutput,
            this, &SpeedtestTab::onReadyRead);
    connect(process, &QProcess::readyReadStandardError,
            this, &SpeedtestTab::onReadyRead);
    connect(progressTimer, &QTimer::timeout,
            this, &SpeedtestTab::onProgressTick);
}

SpeedtestTab::~SpeedtestTab()
{
    if (process->state() == QProcess::Running) {
        process->kill();
        process->waitForFinished(2000);
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  UI Setup
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]]
QString SpeedtestTab::appStyleSheet() const noexcept
{
    return QStringLiteral(
        "QWidget { background-color: %1; color: %2; font-family: 'Segoe UI'; }"
    ).arg(C_BG, C_TEXT);
}

[[nodiscard]]
QFrame *SpeedtestTab::createMetricCard(
    const QString &title, QLabel *&valueLabel,
    const QString &unit, const QString &color)
{
    auto *card = new QFrame();
    card->setStyleSheet(QStringLiteral(
        "QFrame { background-color: %1; border: 1px solid %2; "
        "border-radius: 14px; padding: 14px; }"
        "QFrame:hover { border-color: %3; }"
    ).arg(C_BG_ELEVATED, C_BORDER, color));

    auto *lay = new QVBoxLayout(card);
    lay->setSpacing(6);
    lay->setContentsMargins(10, 10, 10, 10);

    auto *t = new QLabel(title);
    t->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 10px; font-weight: bold; "
        "letter-spacing: 1px; background: transparent;"
    ).arg(C_TEXT_MUTED));
    t->setAlignment(Qt::AlignCenter);
    lay->addWidget(t);

    valueLabel = new QLabel(QStringLiteral("\u2014"));
    valueLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 32px; font-weight: bold; background: transparent;"
    ).arg(color));
    valueLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(valueLabel);

    auto *u = new QLabel(unit);
    u->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 9px; background: transparent;"
    ).arg(C_TEXT_MUTED));
    u->setAlignment(Qt::AlignCenter);
    lay->addWidget(u);

    return card;
}

void SpeedtestTab::setupUI()
{
    setStyleSheet(appStyleSheet());

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 16, 24, 16);
    root->setSpacing(14);

    // ══════════════════════════════════════════════════════════════════════
    //  OPTIONS PANEL
    // ══════════════════════════════════════════════════════════════════════
    QGroupBox *optGroup = new QGroupBox(QStringLiteral("Test Options"));
    optGroup->setStyleSheet(QString(
        "QGroupBox { font-weight: bold; color: %1; border: 1px solid %2; "
        "border-radius: 8px; margin-top: 12px; padding-top: 18px; "
        "background-color: %3; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 14px; "
        "padding: 0 6px; background-color: %3; }"
    ).arg(C_TEXT_MUTED, C_BORDER, C_BG));

    QHBoxLayout *optRow = new QHBoxLayout(optGroup);
    optRow->setSpacing(18);

    const QString cbStyle = QString(
        "QCheckBox { color: %1; font-size: 11px; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; "
        "border: 1px solid %2; background: %3; }"
        "QCheckBox::indicator:checked { background-color: %4; border-color: %4; }"
        "QCheckBox::indicator:hover { border-color: %4; }"
    ).arg(C_TEXT, C_BORDER, C_BG_ELEVATED, C_ACCENT);

    singleConnCheck = new QCheckBox(QStringLiteral("Single Connection"));
    secureCheck     = new QCheckBox(QStringLiteral("Secure (HTTPS)"));
    shareCheck      = new QCheckBox(QStringLiteral("Share Result"));
    singleConnCheck->setStyleSheet(cbStyle);
    secureCheck->setStyleSheet(cbStyle);
    shareCheck->setStyleSheet(cbStyle);

    optRow->addWidget(singleConnCheck);
    optRow->addWidget(secureCheck);
    optRow->addWidget(shareCheck);
    optRow->addStretch();

    // Server selection
    QLabel *srvLabel = new QLabel(QStringLiteral("Server:"));
    srvLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; background: transparent;").arg(C_TEXT));

    serverCombo = new QComboBox();
    serverCombo->setEditable(false);
    serverCombo->setMinimumWidth(260);
    serverCombo->addItem(QStringLiteral("\u27F2 Auto (fastest)"), 0);
    serverCombo->setStyleSheet(QString(
        "QComboBox { background: %1; color: %2; border: 1px solid %3; "
        "border-radius: 5px; padding: 5px 10px; font-size: 11px; }"
        "QComboBox:hover { border-color: %4; }"
        "QComboBox::drop-down { border: none; width: 24px; }"
        "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; "
        "border-right: 5px solid transparent; border-top: 6px solid %2; "
        "margin-right: 6px; }"
        "QComboBox QAbstractItemView { background: %1; color: %2; "
        "selection-background-color: %4; border: 1px solid %3; outline: none; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, C_ACCENT));

    browseServersButton = new QPushButton(QStringLiteral("Browse\u2026"));
    browseServersButton->setCursor(Qt::PointingHandCursor);
    browseServersButton->setFixedHeight(28);
    browseServersButton->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; "
        "border-radius: 5px; padding: 4px 14px; font-size: 11px; }"
        "QPushButton:hover { background: %4; border-color: %4; color: white; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, C_ACCENT));

    optRow->addWidget(srvLabel);
    optRow->addWidget(serverCombo);
    optRow->addWidget(browseServersButton);

    root->addWidget(optGroup);

    // ══════════════════════════════════════════════════════════════════════
    //  ISP / SERVER INFO
    // ══════════════════════════════════════════════════════════════════════
    ispLabel = new QLabel(QStringLiteral("ISP: \u2014"));
    ispLabel->setAlignment(Qt::AlignCenter);
    ispLabel->setStyleSheet(QString(
        "color: %1; font-size: 15px; font-weight: bold; background: transparent;"
    ).arg(C_PRIMARY));

    serverLabel = new QLabel(QStringLiteral("No server selected \u2013 will use auto-select"));
    serverLabel->setAlignment(Qt::AlignCenter);
    serverLabel->setStyleSheet(QString(
        "color: %1; font-size: 11px; background: transparent;"
    ).arg(C_TEXT_MUTED));

    root->addWidget(ispLabel);
    root->addWidget(serverLabel);

    // ══════════════════════════════════════════════════════════════════════
    //  METRIC CARDS
    // ══════════════════════════════════════════════════════════════════════
    QHBoxLayout *meterRow = new QHBoxLayout();
    meterRow->setSpacing(16);
    meterRow->addWidget(createMetricCard(QStringLiteral("PING"),     pingLabel,     QStringLiteral("ms"),    C_ACCENT));
    meterRow->addWidget(createMetricCard(QStringLiteral("DOWNLOAD"), downloadLabel, QStringLiteral("Mbps"),  C_SUCCESS));
    meterRow->addWidget(createMetricCard(QStringLiteral("UPLOAD"),   uploadLabel,   QStringLiteral("Mbps"),  C_PRIMARY));
    root->addLayout(meterRow);

    // ══════════════════════════════════════════════════════════════════════
    //  SHARE LINK
    // ══════════════════════════════════════════════════════════════════════
    shareLinkLabel = new QLabel();
    shareLinkLabel->setAlignment(Qt::AlignCenter);
    shareLinkLabel->setOpenExternalLinks(true);
    shareLinkLabel->setStyleSheet(QStringLiteral("background: transparent;"));
    root->addWidget(shareLinkLabel);

    // ══════════════════════════════════════════════════════════════════════
    //  PROGRESS BAR
    // ══════════════════════════════════════════════════════════════════════
    progressGauge = new QProgressBar();
    progressGauge->setRange(0, 100);
    progressGauge->setValue(0);
    progressGauge->setTextVisible(true);
    progressGauge->setFixedHeight(26);
    progressGauge->setStyleSheet(QString(
        "QProgressBar { background: %1; border-radius: 5px; border: none; "
        "color: %2; font-size: 11px; font-weight: bold; text-align: center; }"
        "QProgressBar::chunk { "
        "background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 %3, stop:0.5 %4, stop:1 %5); "
        "border-radius: 5px; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_ACCENT, C_PRIMARY, C_SUCCESS));
    root->addWidget(progressGauge);

    // ══════════════════════════════════════════════════════════════════════
    //  GO / STOP BUTTONS
    // ══════════════════════════════════════════════════════════════════════
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();

    startButton = new QPushButton(QStringLiteral("GO"));
    startButton->setFixedSize(150, 150);
    startButton->setCursor(Qt::PointingHandCursor);
    startButton->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; "
        "border: 4px solid %1; border-radius: 75px; "
        "font-size: 34px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(233, 69, 96, 0.12); "
        "border-color: %2; color: %2; }"
        "QPushButton:pressed { background: %1; color: white; }"
    ).arg(C_ACCENT, C_TEXT));

    stopButton = new QPushButton(QStringLiteral("STOP"));
    stopButton->setFixedSize(110, 44);
    stopButton->setCursor(Qt::PointingHandCursor);
    stopButton->setEnabled(false);
    stopButton->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; "
        "border: 3px solid %1; border-radius: 22px; "
        "font-size: 16px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(233, 69, 96, 0.15); "
        "border-color: %1; }"
        "QPushButton:pressed { background: %1; color: white; }"
        "QPushButton:disabled { color: %2; border-color: %3; }"
    ).arg(C_ACCENT, C_TEXT_MUTED, C_BORDER));

    btnRow->addWidget(startButton);
    btnRow->addSpacing(24);
    btnRow->addWidget(stopButton);
    btnRow->addSpacing(24);

    // ── Multi-Server Speedtest (Item 38) ─────────────────────────────────
    mMultiButton = new QPushButton(QStringLiteral("MULTI"));
    mMultiButton->setFixedSize(110, 44);
    mMultiButton->setCursor(Qt::PointingHandCursor);
    mMultiButton->setToolTip(QStringLiteral("Run parallel speedtests on multiple servers"));
    mMultiButton->setStyleSheet(QString(
        "QPushButton { background: %1; color: white; font-weight: bold; "
        "border: 2px solid %1; border-radius: 8px; font-size: 15px; }"
        "QPushButton:hover { background: %2; }"
        "QPushButton:disabled { color: %3; border-color: %4; background: transparent; }"
    ).arg(C_SUCCESS, C_SUCCESS_HVR, C_TEXT_MUTED, C_BORDER));
    btnRow->addWidget(mMultiButton);

    btnRow->addStretch();
    root->addLayout(btnRow);

    // ══════════════════════════════════════════════════════════════════════
    //  STATUS
    // ══════════════════════════════════════════════════════════════════════
    statusLabel = new QLabel(QStringLiteral("Ready \u2013 press GO to start"));
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(QString(
        "color: %1; font-size: 13px; font-weight: bold; background: transparent;"
    ).arg(C_ACCENT));
    root->addWidget(statusLabel);

    // ══════════════════════════════════════════════════════════════════════
    //  LOG
    // ══════════════════════════════════════════════════════════════════════
    logArea = new QTextEdit();
    logArea->setReadOnly(true);
    logArea->setMaximumHeight(130);
    logArea->setPlaceholderText(QStringLiteral("Test log will appear here..."));
    logArea->setStyleSheet(QString(
        "QTextEdit { background: %3; color: %1; font-size: 10px; "
        "border-radius: 8px; border: 1px solid %2; padding: 6px; "
        "font-family: 'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace; }"
    ).arg(C_TEXT_MUTED, C_BORDER, C_BG_SUNKEN));
    root->addWidget(logArea);

    // ── Connections ──────────────────────────────────────────────────────
    connect(startButton,         &QPushButton::clicked, this, &SpeedtestTab::onStartClicked);
    connect(stopButton,          &QPushButton::clicked, this, &SpeedtestTab::onStopClicked);
    connect(browseServersButton, &QPushButton::clicked, this, &SpeedtestTab::onBrowseServers);
    connect(mMultiButton,        &QPushButton::clicked, this, &SpeedtestTab::onMultiTestClicked);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Control helpers
// ═══════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::setControlsEnabled(bool enabled) noexcept
{
    startButton->setEnabled(enabled);
    stopButton->setEnabled(!enabled);
    browseServersButton->setEnabled(enabled);
    serverCombo->setEnabled(enabled);
    singleConnCheck->setEnabled(enabled);
    secureCheck->setEnabled(enabled);
    shareCheck->setEnabled(enabled);
}

void SpeedtestTab::resetDisplay() noexcept
{
    pingLabel->setText(QStringLiteral("\u2014"));
    downloadLabel->setText(QStringLiteral("\u2014"));
    uploadLabel->setText(QStringLiteral("\u2014"));
    ispLabel->setText(QStringLiteral("ISP: \u2014"));
    serverLabel->setText(serverCombo->currentIndex() == 0
                         ? QStringLiteral("Auto-select (fastest server)")
                         : QStringLiteral("Server #%1 selected").arg(serverCombo->currentData().toInt()));
    shareLinkLabel->setText(QString());
    progressGauge->setValue(0);
    progressGauge->setFormat(QString());
    logArea->clear();
    jsonBuffer.clear();
    isJsonMode       = false;
    isServerListMode = false;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Speedtest start
// ═══════════════════════════════════════════════════════════════════════════════

QString SpeedtestTab::findSpeedtest() const noexcept
{
    return IPView::Speedtest::ServerSelectionModule::findSpeedtestBinary();
}

void SpeedtestTab::startProcess(const QStringList &args)
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished(2000);
    }
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start(findSpeedtest(), args);
}

void SpeedtestTab::onStartClicked()
{
    if (process->state() != QProcess::NotRunning) {
        process->kill();
        process->waitForFinished(2000);
    }

    QString const program = findSpeedtest();
    if (program.isEmpty()) {
        statusLabel->setText(QStringLiteral("speedtest-cli not found! Install: sudo pacman -S speedtest-cli"));
        logArea->append(QStringLiteral("Install: sudo pacman -S speedtest-cli"));
        return;
    }

    resetDisplay();

    // Args: --json is required for structured output
    QStringList args = {QStringLiteral("--json")};
    if (secureCheck->isChecked())     args << QStringLiteral("--secure");
    if (singleConnCheck->isChecked()) args << QStringLiteral("--single");
    if (shareCheck->isChecked())      args << QStringLiteral("--share");

    // Server selection from combo box
    int const serverId = serverCombo->currentData().toInt();
    if (serverId > 0) {
        args << QStringLiteral("--server") << QString::number(serverId);
        serverLabel->setText(QStringLiteral("Server #%1 selected").arg(serverId));
    } else {
        serverLabel->setText(QStringLiteral("Auto-select (fastest server)"));
    }

    logArea->append(QStringLiteral("$ %1 %2").arg(program, args.join(QLatin1Char(' '))));
    logArea->append(QStringLiteral("Contacting speedtest.net \u2026"));

    setControlsEnabled(false);
    statusLabel->setText(QStringLiteral("Initializing \u2026"));
    progressGauge->setValue(4);
    progressGauge->setFormat(QStringLiteral("Initializing \u2026"));
    progressDot = 0;
    isJsonMode      = true;
    isServerListMode = false;

    elapsedTimer.start();
    progressTimer->start(200);

    startProcess(args);

    if (!process->waitForStarted(5000)) {
        statusLabel->setText(QStringLiteral("Failed to start process"));
        setControlsEnabled(true);
        progressTimer->stop();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Multi-Server Parallel Speedtest (Item 38)
// ═══════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::onMultiTestClicked()
{
    QString const program = findSpeedtest();
    if (program.isEmpty()) {
        logArea->append(QStringLiteral("speedtest-cli not found! Install: sudo pacman -S speedtest-cli"));
        return;
    }

    // ── Server-Liste laden ──────────────────────────────────────────────
    if (serverCache.empty()) {
        logArea->append(QStringLiteral("Fetching server list\u2026"));
        auto servers = serverSelector->getAvailableServers(8000);
        if (!servers.has_value()) {
            logArea->append(QStringLiteral("Failed to fetch servers: ") + servers.error());
            return;
        }
        serverCache = std::move(*servers);
    }

    if (serverCache.empty()) {
        logArea->append(QStringLiteral("No servers available"));
        return;
    }

    // ── Top 3 nächstgelegene Server auswählen ──────────────────────────
    auto sorted = serverCache;
    serverSelector->sortServers(sorted, IPView::Speedtest::ServerSelectionModule::SortBy::Distance);
    int const count = std::min(3, static_cast<int>(sorted.size()));

    resetDisplay();
    logArea->clear();
    logArea->append(QStringLiteral("\u2500\u2500 Multi-Server Speedtest (%1 servers) \u2500\u2500").arg(count));

    // ── Alle vorherigen Prozesse bereinigen ────────────────────────────
    for (auto *p : mMultiProcesses) {
        if (p) { p->kill(); p->deleteLater(); }
    }
    mMultiProcesses.clear();
    mMultiResults.clear();
    mMultiCompleted = 0;
    mMultiTotal = count;
    mMultiMode = true;

    setControlsEnabled(false);
    statusLabel->setText(QStringLiteral("Running %1 parallel tests\u2026").arg(count));
    progressGauge->setValue(0);
    progressGauge->setFormat(QStringLiteral("0 / %1").arg(count));

    // ── Timer für Fortschrittsanzeige ──────────────────────────────────
    if (!mMultiTimer) {
        mMultiTimer = new QTimer(this);
        connect(mMultiTimer, &QTimer::timeout, this, &SpeedtestTab::onMultiProgressTick);
    }
    mMultiTimer->start(500);

    // ── Prozesse parallel starten ──────────────────────────────────────
    for (int i = 0; i < count; ++i) {
        auto const &srv = sorted[static_cast<std::size_t>(i)];

        QStringList args = {QStringLiteral("--json"), QStringLiteral("--server"), QString::number(srv.id)};
        if (secureCheck->isChecked())     args << QStringLiteral("--secure");
        if (singleConnCheck->isChecked()) args << QStringLiteral("--single");

        logArea->append(QStringLiteral("  [%1] Server #%2 (%3, %4)")
                            .arg(i + 1).arg(srv.id).arg(srv.sponsor, srv.location));

        auto *p = new QProcess(this);
        p->setProcessChannelMode(QProcess::MergedChannels);

        connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, i](int exitCode, QProcess::ExitStatus) {
            onMultiProcessFinished(i, exitCode);
        });

        mMultiProcesses.append(p);
        p->start(program, args);
    }
}

void SpeedtestTab::onMultiProgressTick()
{
    auto const running = std::count_if(mMultiProcesses.begin(), mMultiProcesses.end(),
        [](QProcess *p) { return p && p->state() == QProcess::Running; });

    statusLabel->setText(QStringLiteral("Running (%1 active, %2/%3 done)")
                             .arg(static_cast<int>(running)).arg(mMultiCompleted).arg(mMultiTotal));
    progressGauge->setValue(mMultiCompleted);
    progressGauge->setMaximum(mMultiTotal);
    progressGauge->setFormat(QStringLiteral("%1 / %2").arg(mMultiCompleted).arg(mMultiTotal));
}

void SpeedtestTab::onMultiProcessFinished(int index, int exitCode)
{
    if (!mMultiMode) return;

    QProcess *p = mMultiProcesses.value(index);
    if (!p) return;

    QByteArray const output = p->readAll();

    if (exitCode == 0 && !output.isEmpty()) {
        QJsonParseError err;
        QJsonDocument const doc = QJsonDocument::fromJson(output, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            mMultiResults.append(doc.object());
        }
    }

    ++mMultiCompleted;

    // Alle Prozesse fertig?
    if (mMultiCompleted >= mMultiTotal) {
        mMultiTimer->stop();
        mMultiMode = false;
        aggregateMultiResults();
        setControlsEnabled(true);
    }
}

void SpeedtestTab::aggregateMultiResults()
{
    if (mMultiResults.isEmpty()) {
        statusLabel->setText(QStringLiteral("All multi-server tests failed"));
        progressGauge->setFormat(QStringLiteral("Failed"));
        return;
    }

    // Beste Ergebnisse aus allen parallelen Tests aggregieren
    double bestPing    = 9999.0;
    double bestDl      = 0.0;
    double bestUl      = 0.0;
    double totalDl     = 0.0;
    double totalUl     = 0.0;
    QString servers;

    for (int i = 0; i < mMultiResults.size(); ++i) {
        auto const &obj = mMultiResults[i];
        double const ping = obj[QStringLiteral("ping")].toDouble();
        double const dl   = obj[QStringLiteral("download")].toDouble() / 1.0e6;
        double const ul   = obj[QStringLiteral("upload")].toDouble()   / 1.0e6;

        bestPing = std::min(bestPing, ping);
        bestDl   = std::max(bestDl,   dl);
        bestUl   = std::max(bestUl,   ul);
        totalDl += dl;
        totalUl += ul;

        QString const srv = obj[QStringLiteral("server")].toObject()
                                [QStringLiteral("sponsor")].toString();
        if (!srv.isEmpty()) {
            if (!servers.isEmpty()) servers += QStringLiteral(", ");
            servers += srv;
        }

        logArea->append(QStringLiteral("  [%1] Ping: %2 ms | DL: %3 Mbit/s | UL: %4 Mbit/s | %5")
                            .arg(i + 1).arg(ping, 0, 'f', 1).arg(dl, 0, 'f', 1)
                            .arg(ul, 0, 'f', 1).arg(srv));
    }

    // Anzeige: Bester Ping, max DL, max UL + Gesamt-DL
    pingLabel->setText(QString::number(bestPing, 'f', 1));
    downloadLabel->setText(QStringLiteral("%1 / %2")
                               .arg(bestDl, 0, 'f', 1).arg(totalDl, 0, 'f', 1));
    uploadLabel->setText(QStringLiteral("%1 / %2")
                             .arg(bestUl, 0, 'f', 1).arg(totalUl, 0, 'f', 1));
    serverLabel->setText(QStringLiteral("Multi (%1 servers)").arg(mMultiResults.size()));
    ispLabel->setText(QStringLiteral("Aggregated from: %1").arg(servers));

    statusLabel->setText(QStringLiteral("Multi-server test completed"));
    progressGauge->setValue(mMultiTotal);
    progressGauge->setFormat(QStringLiteral("Completed"));
    logArea->append(QStringLiteral("\u2500\u2500 Multi test finished \u2500\u2500"));

    // Bereinigen
    mMultiResults.clear();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Server browser — uses ServerSelectionModule
// ═══════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::onBrowseServers()
{
    QString const program = findSpeedtest();
    if (program.isEmpty()) {
        statusLabel->setText(QStringLiteral("speedtest-cli not found!"));
        return;
    }

    // ── Use cached servers or fetch via ServerSelectionModule ────────────
    std::vector<IPView::Speedtest::ServerInfo> servers;

    if (serverCache.empty()) {
        auto const result = serverSelector->getAvailableServers(30000);
        if (result.has_value()) {
            servers = *result;
        } else {
            statusLabel->setText(QStringLiteral("Server fetch failed: %1").arg(result.error()));
            return;
        }
    } else {
        servers = serverCache;
    }

    // ── Dialog ──────────────────────────────────────────────────────────
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Browse Speedtest Servers"));
    dlg.setMinimumSize(640, 480);
    dlg.setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
    ).arg(C_BG, C_TEXT));

    QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
    dlgLayout->setSpacing(12);

    QLabel *info = new QLabel(QStringLiteral("%1 servers found. Double-click or select + Accept.")
                                  .arg(static_cast<int>(servers.size())));
    info->setStyleSheet(QString("color: %1; font-size: 12px;").arg(C_TEXT_MUTED));
    dlgLayout->addWidget(info);

    // Filter row
    auto *filterRow = new QHBoxLayout();
    auto *filterLabel = new QLabel(QStringLiteral("Filter:"));
    filterLabel->setStyleSheet(QString("color: %1; font-size: 11px;").arg(C_TEXT));
    auto *filterEdit = new QLineEdit();
    filterEdit->setPlaceholderText(QStringLiteral("Search by sponsor or location..."));
    filterEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; border: 1px solid %3; "
        "border-radius: 4px; padding: 4px 8px; font-size: 11px; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER));
    filterRow->addWidget(filterLabel);
    filterRow->addWidget(filterEdit, 1);
    dlgLayout->addLayout(filterRow);

    // Table
    QTableWidget *table = new QTableWidget(0, 4);
    table->setObjectName(QStringLiteral("serverTable"));
    table->setHorizontalHeaderLabels({
        QStringLiteral("ID"), QStringLiteral("Sponsor"),
        QStringLiteral("Location"), QStringLiteral("Distance (km)")
    });
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setSortingEnabled(true);
    table->verticalHeader()->setVisible(false);
    table->setStyleSheet(QString(
        "QTableWidget { background: %1; color: %2; border: 1px solid %3; "
        "border-radius: 6px; gridline-color: %3; font-size: 11px; }"
        "QTableWidget::item { padding: 4px 8px; }"
        "QTableWidget::item:selected { background: %4; color: white; }"
        "QHeaderView::section { background: %1; color: %2; "
        "border: none; border-bottom: 1px solid %3; padding: 6px; "
        "font-weight: bold; font-size: 11px; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, C_ACCENT));
    dlgLayout->addWidget(table);

    // Buttons
    QDialogButtonBox *btns = new QDialogButtonBox();
    QPushButton *selectBtn = btns->addButton(QStringLiteral("Select Server"), QDialogButtonBox::AcceptRole);
    selectBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: white; border: none; "
        "border-radius: 5px; padding: 8px 20px; font-weight: bold; }"
        "QPushButton:hover { background: %2; }"
    ).arg(C_ACCENT, C_ACCENT_ACT));
    btns->addButton(QDialogButtonBox::Cancel);
    btns->setStyleSheet(QStringLiteral("QPushButton { color: white; }"));
    dlgLayout->addWidget(btns);

    // ── Populate table ──────────────────────────────────────────────────
    auto populateTable = [&](const std::vector<IPView::Speedtest::ServerInfo> &sv) {
        table->setRowCount(0);
        for (auto const &s : sv) {
            int const row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(QString::number(s.id)));
            table->setItem(row, 1, new QTableWidgetItem(s.sponsor));
            table->setItem(row, 2, new QTableWidgetItem(s.location));
            table->setItem(row, 3,
                new QTableWidgetItem(QString::number(s.distanceKm, 'f', 1)));
        }
        info->setText(QStringLiteral("%1 servers shown (of %2 total)")
                          .arg(table->rowCount())
                          .arg(static_cast<int>(servers.size())));
    };

    populateTable(servers);

    // ── Filter live ─────────────────────────────────────────────────────
    connect(filterEdit, &QLineEdit::textChanged, &dlg, [&](const QString &text) {
        if (text.isEmpty()) {
            populateTable(servers);
        } else {
            auto const filtered = IPView::Speedtest::ServerSelectionModule::filterByQuery(servers, text);
            populateTable(filtered);
        }
    });

    // Double-click = selection
    connect(table, &QTableWidget::cellDoubleClicked, &dlg, [&](int row, int) {
        if (row >= 0) {
            int const sid = table->item(row, 0)->text().toInt();
            setSelectedServer(sid);
            dlg.accept();
        }
    });

    // Accept
    connect(btns, &QDialogButtonBox::accepted, &dlg, [&]() {
        int const row = table->currentRow();
        if (row >= 0) {
            int const sid = table->item(row, 0)->text().toInt();
            setSelectedServer(sid);
        }
    });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // Cache for next time
    if (serverCache.empty()) {
        serverCache = std::move(servers);
    }

    dlg.exec();
}

void SpeedtestTab::setSelectedServer(int serverId)
{
    // Find the server in cache
    for (auto const &s : serverCache) {
        if (s.id == serverId) {
            serverCombo->setCurrentIndex(serverCombo->findData(serverId));
            serverLabel->setText(QStringLiteral("Server: %1 (%2) \u2014 #%3, %4 km")
                                     .arg(s.sponsor, s.location)
                                     .arg(s.id)
                                     .arg(s.distanceKm, 0, 'f', 0));
            statusLabel->setText(QStringLiteral("Server #%1 selected").arg(serverId));
            logArea->append(QStringLiteral("Selected server: %1 (%2) \u2014 ID %3, %4 km")
                                .arg(s.sponsor, s.location)
                                .arg(s.id)
                                .arg(s.distanceKm, 0, 'f', 0));
            return;
        }
    }
    // Fallback: add manually to combo box if not in cache
    for (int i = 0; i < serverCombo->count(); ++i) {
        if (serverCombo->itemData(i).toInt() == serverId) {
            serverCombo->setCurrentIndex(i);
            serverLabel->setText(QStringLiteral("Server #%1 selected").arg(serverId));
            return;
        }
    }
    // Add new entry
    serverCombo->addItem(QStringLiteral("Server #%1").arg(serverId), serverId);
    serverCombo->setCurrentIndex(serverCombo->count() - 1);
    serverLabel->setText(QStringLiteral("Server #%1 selected (custom)").arg(serverId));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Data I/O
// ═══════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::onReadyRead()
{
    QByteArray const rawData = process->readAll();

    if (isServerListMode) {
        QString const text = QString::fromUtf8(rawData);
        logArea->append(text.trimmed());
        return;
    }

    if (isJsonMode) {
        jsonBuffer.append(rawData);

        // Display intermediate output
        QString const text = QString::fromUtf8(rawData).trimmed();
        if (!text.isEmpty() && !text.startsWith(QLatin1Char('{'))
            && !text.startsWith(QLatin1Char('['))) {
            logArea->append(text);
        }

        // ISP detection from "Testing from ISP (IP)..."
        static QRegularExpression const ispRe(QStringLiteral(R"(Testing from (.*?)\s*\()"));
        auto const m = ispRe.match(text);
        if (m.hasMatch()) {
            QString const isp = m.captured(1).trimmed();
            if (!isp.isEmpty() && isp != QStringLiteral("Unknown")) {
                ispLabel->setText(QStringLiteral("ISP: ") + isp);
            }
        }
    }
}

void SpeedtestTab::onProgressTick()
{
    if (isServerListMode) return;

    qint64 elapsed = elapsedTimer.elapsed();
    int pseudo = qMin(95, static_cast<int>(elapsed / 280));
    progressGauge->setValue(qMax(progressGauge->value(), pseudo));

    progressDot = (progressDot + 1) % 8;
    QString dots = QString(progressDot + 1, QLatin1Char('.'));

    QString stage;
    if      (elapsed <  4000) stage = QStringLiteral("Selecting server");
    else if (elapsed < 12000) stage = QStringLiteral("Testing ping");
    else if (elapsed < 30000) stage = QStringLiteral("Testing download");
    else if (elapsed < 50000) stage = QStringLiteral("Testing upload");
    else                      stage = QStringLiteral("Finishing");

    statusLabel->setText(stage % QStringLiteral(" ") % dots);
    progressGauge->setFormat(stage % QStringLiteral(" ") % dots);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Finished
// ═══════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::onSpeedtestFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    progressTimer->stop();
    setControlsEnabled(true);

    if (exitStatus == QProcess::CrashExit) {
        statusLabel->setText(QStringLiteral("Process crashed (signal)"));
        logArea->append(QStringLiteral("speedtest-cli crashed or was killed."));
        progressGauge->setFormat(QStringLiteral("Crashed"));
        isJsonMode = false;
        isServerListMode = false;
        return;
    }

    // Retrieve remaining data from pipe
    if (isJsonMode && process) {
        jsonBuffer.append(process->readAll());
    }

    // Server list mode
    if (isServerListMode) {
        statusLabel->setText(QStringLiteral("Server list loaded into log"));
        progressGauge->setValue(100);
        progressGauge->setFormat(QStringLiteral("Done"));
        isServerListMode = false;
        return;
    }

    // JSON mode
    if (isJsonMode && (exitCode == 0 || jsonBuffer.size() > 20)) {
        QJsonParseError err;
        QJsonDocument const doc = QJsonDocument::fromJson(jsonBuffer, &err);

        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject const obj = doc.object();
            updateDisplayFromJson(obj);
            progressGauge->setValue(100);
            progressGauge->setFormat(QStringLiteral("Completed"));
            statusLabel->setText(QStringLiteral("Speed test completed"));
            logArea->append(QStringLiteral("\u2500\u2500 Test finished \u2500\u2500"));
            isJsonMode = false;
            return;
        }

        statusLabel->setText(QStringLiteral("JSON error: ") + err.errorString());
        logArea->append(QStringLiteral("JSON parse error: ") + err.errorString());
        logArea->append(QStringLiteral("\u2500\u2500 Raw output \u2500\u2500"));
        logArea->append(QString::fromUtf8(jsonBuffer));
        progressGauge->setFormat(QStringLiteral("Parse error"));
        isJsonMode = false;
        return;
    }

    if (exitCode != 0) {
        statusLabel->setText(QStringLiteral("Test failed (exit %1)").arg(exitCode));
        logArea->append(QStringLiteral("speedtest-cli exited with code %1").arg(exitCode));
        progressGauge->setFormat(QStringLiteral("Failed"));
    } else {
        statusLabel->setText(QStringLiteral("No JSON data received"));
        progressGauge->setFormat(QStringLiteral("No data"));
    }
    isJsonMode = false;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Display
// ═══════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::updateDisplayFromJson(const QJsonObject &obj) noexcept
{
    // Ping
    double const ping = obj[QStringLiteral("ping")].toDouble();
    pingLabel->setText(QString::number(ping, 'f', 1));

    // Download / Upload (bits/s to Mbit/s)
    double const dl = obj[QStringLiteral("download")].toDouble() / 1.0e6;
    double const ul = obj[QStringLiteral("upload")].toDouble()   / 1.0e6;
    downloadLabel->setText(QString::number(dl, 'f', 1));
    uploadLabel->setText(QString::number(ul, 'f', 1));

    // Server details
    if (obj.contains(QStringLiteral("server"))) {
        QJsonObject const s = obj[QStringLiteral("server")].toObject();
        QString const sponsor = s[QStringLiteral("sponsor")].toString();
        QString const name    = s[QStringLiteral("name")].toString();
        QString const cc      = s[QStringLiteral("cc")].toString();
        int const sid         = s[QStringLiteral("id")].toString().toInt();
        double const lat      = s[QStringLiteral("latency")].toDouble();

        serverLabel->setText(QStringLiteral("Server: %1 (%2) \u00B7 %3 \u00B7 ID %4 \u00B7 latency %5 ms")
                                 .arg(sponsor, name, cc)
                                 .arg(sid)
                                 .arg(lat, 0, 'f', 1));
    }

    // ISP
    if (obj.contains(QStringLiteral("client"))) {
        QJsonObject const c = obj[QStringLiteral("client")].toObject();
        QString const isp = c[QStringLiteral("isp")].toString();
        if (!isp.isEmpty() && isp != QStringLiteral("Unknown")) {
            ispLabel->setText(QStringLiteral("ISP: ") + isp);
        }
    }

    // Share link
    if (obj.contains(QStringLiteral("share")) && !obj[QStringLiteral("share")].isNull()) {
        QString const url = obj[QStringLiteral("share")].toString();
        if (!url.isEmpty()) {
            shareLinkLabel->setText(
                QStringLiteral("<a href='%1' style='color: %2; text-decoration: none; "
                               "font-weight: bold; font-size: 12px;'>"
                               "Share Result Image</a>").arg(url, C_SUCCESS));
        }
    }

    // Log
    logArea->append(QStringLiteral("Ping: %1 ms \u00B7 Download: %2 Mbps \u00B7 Upload: %3 Mbps")
                        .arg(ping, 0, 'f', 1)
                        .arg(dl, 0, 'f', 1)
                        .arg(ul, 0, 'f', 1));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Stop
// ═══════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::onStopClicked()
{
    if (mMultiMode) {
        for (auto *p : mMultiProcesses) {
            if (p && p->state() == QProcess::Running) {
                p->kill();
            }
        }
        logArea->append(QStringLiteral("Multi-server test cancelled by user."));
        statusLabel->setText(QStringLiteral("Cancelled"));
        mMultiMode = false;
        setControlsEnabled(true);
        return;
    }

    if (process->state() == QProcess::Running) {
        process->kill();
        logArea->append(QStringLiteral("Cancelled by user."));
        statusLabel->setText(QStringLiteral("Cancelled"));
        progressGauge->setFormat(QStringLiteral("Cancelled"));
    }
    progressTimer->stop();
    setControlsEnabled(true);
    isJsonMode = false;
    isServerListMode = false;
}
