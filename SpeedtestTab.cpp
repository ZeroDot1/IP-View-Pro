// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — SpeedtestTab.cpp
//  C++26: auto, QStringLiteral, const-correctness, structured bindings
//  Professional speed test via Python speedtest-cli (sivel) with --json.
//  Features: Server browser, animated progress, real-time metrics.
// ═══════════════════════════════════════════════════════════════════════════════

#include "SpeedtestTab.h"

#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFont>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTableWidget>
#include <QVBoxLayout>

// ── App-wide style constants (C++26: constexpr std::string_view) ──────────
//  Uses char arrays for minimal runtime overhead
static constexpr auto BG_DARK    = "#0f0f1a";
static constexpr auto BG_CARD    = "#1a1a2e";
static constexpr auto BORDER     = "#2a2a3e";
static constexpr auto ACCENT     = "#e94560";
static constexpr auto TEXT_DIM   = "#888888";
static constexpr auto TEXT_WHITE = "#ffffff";
static constexpr auto GREEN      = "#00ff88";
static constexpr auto CYAN       = "#00d4ff";

// ═══════════════════════════════════════════════════════════════════════════════
SpeedtestTab::SpeedtestTab(QWidget *parent)
    : QWidget(parent)
    , process(new QProcess(this))
    , progressTimer(new QTimer(this))
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

// ════════════════════════════════════════════════════════════════════════════
//  UI Setup
// ════════════════════════════════════════════════════════════════════════════

[[nodiscard]]
QString SpeedtestTab::appStyleSheet() const noexcept
{
    // C++26: QStringLiteral + constexpr char arrays for maximum efficiency
    return QStringLiteral(
        "QWidget { background-color: %1; color: %2; font-family: 'Segoe UI'; }"
    ).arg(QString::fromLatin1(BG_DARK), QString::fromLatin1(TEXT_WHITE));
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
    ).arg(QString::fromLatin1(BG_CARD), QString::fromLatin1(BORDER), color));

    auto *lay = new QVBoxLayout(card);
    lay->setSpacing(6);
    lay->setContentsMargins(10, 10, 10, 10);

    auto *t = new QLabel(title);
    t->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 10px; font-weight: bold; "
        "letter-spacing: 1px; background: transparent;"
    ).arg(QString::fromLatin1(TEXT_DIM)));
    t->setAlignment(Qt::AlignCenter);
    lay->addWidget(t);

    valueLabel = new QLabel(QStringLiteral("—"));
    valueLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 32px; font-weight: bold; background: transparent;"
    ).arg(color));
    valueLabel->setAlignment(Qt::AlignCenter);
    lay->addWidget(valueLabel);

    auto *u = new QLabel(unit);
    u->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 9px; background: transparent;"
    ).arg(QString::fromLatin1(TEXT_DIM)));
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

    // ══════════════════════════════════════════════════════════════════
    //  OPTIONS PANEL
    // ══════════════════════════════════════════════════════════════════
    QGroupBox *optGroup = new QGroupBox("Test Options");
    optGroup->setStyleSheet(QString(
        "QGroupBox { font-weight: bold; color: %1; border: 1px solid %2; "
        "border-radius: 8px; margin-top: 12px; padding-top: 18px; "
        "background-color: %3; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 14px; "
        "padding: 0 6px; background-color: %3; }"
    ).arg(TEXT_DIM, BORDER, BG_DARK));

    QHBoxLayout *optRow = new QHBoxLayout(optGroup);
    optRow->setSpacing(18);

    // Checkboxes – uniformly styled
    const QString cbStyle = QString(
        "QCheckBox { color: %1; font-size: 11px; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; "
        "border: 1px solid %2; background: %3; }"
        "QCheckBox::indicator:checked { background-color: %4; border-color: %4; }"
        "QCheckBox::indicator:hover { border-color: %4; }"
    ).arg(TEXT_WHITE, BORDER, BG_CARD, ACCENT);

    singleConnCheck = new QCheckBox("Single Connection");
    secureCheck     = new QCheckBox("Secure (HTTPS)");
    shareCheck      = new QCheckBox("Share Result");
    singleConnCheck->setStyleSheet(cbStyle);
    secureCheck->setStyleSheet(cbStyle);
    shareCheck->setStyleSheet(cbStyle);

    optRow->addWidget(singleConnCheck);
    optRow->addWidget(secureCheck);
    optRow->addWidget(shareCheck);
    optRow->addStretch();

    // Server selection
    QLabel *srvLabel = new QLabel("Server:");
    srvLabel->setStyleSheet(QString("color: %1; font-size: 11px; font-weight: bold; background: transparent;").arg(TEXT_WHITE));

    serverCombo = new QComboBox();
    serverCombo->setEditable(false);
    serverCombo->setMinimumWidth(260);
    serverCombo->addItem("⟲ Auto (fastest)", 0);
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
    ).arg(BG_CARD, TEXT_WHITE, BORDER, ACCENT));

    browseServersButton = new QPushButton("Browse…");
    browseServersButton->setCursor(Qt::PointingHandCursor);
    browseServersButton->setFixedHeight(28);
    browseServersButton->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3; "
        "border-radius: 5px; padding: 4px 14px; font-size: 11px; }"
        "QPushButton:hover { background: %4; border-color: %4; color: white; }"
    ).arg(BG_CARD, TEXT_WHITE, BORDER, ACCENT));

    optRow->addWidget(srvLabel);
    optRow->addWidget(serverCombo);
    optRow->addWidget(browseServersButton);

    root->addWidget(optGroup);

    // ══════════════════════════════════════════════════════════════════
    //  ISP / SERVER INFO
    // ══════════════════════════════════════════════════════════════════
    ispLabel = new QLabel("ISP: —");
    ispLabel->setAlignment(Qt::AlignCenter);
    ispLabel->setStyleSheet(QString(
        "color: %1; font-size: 15px; font-weight: bold; background: transparent;"
    ).arg(CYAN));

    serverLabel = new QLabel("No server selected – will use auto-select");
    serverLabel->setAlignment(Qt::AlignCenter);
    serverLabel->setStyleSheet(QString(
        "color: %1; font-size: 11px; background: transparent;"
    ).arg(TEXT_DIM));

    root->addWidget(ispLabel);
    root->addWidget(serverLabel);

    // ══════════════════════════════════════════════════════════════════
    //  METRIC CARDS
    // ══════════════════════════════════════════════════════════════════
    QHBoxLayout *meterRow = new QHBoxLayout();
    meterRow->setSpacing(16);
    meterRow->addWidget(createMetricCard("PING",     pingLabel,     "ms",    ACCENT));
    meterRow->addWidget(createMetricCard("DOWNLOAD", downloadLabel, "Mbps",  GREEN));
    meterRow->addWidget(createMetricCard("UPLOAD",   uploadLabel,   "Mbps",  CYAN));
    root->addLayout(meterRow);

    // ══════════════════════════════════════════════════════════════════
    //  SHARE LINK
    // ══════════════════════════════════════════════════════════════════
    shareLinkLabel = new QLabel();
    shareLinkLabel->setAlignment(Qt::AlignCenter);
    shareLinkLabel->setOpenExternalLinks(true);
    shareLinkLabel->setStyleSheet("background: transparent;");
    root->addWidget(shareLinkLabel);

    // ══════════════════════════════════════════════════════════════════
    //  PROGRESS BAR
    // ══════════════════════════════════════════════════════════════════
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
    ).arg(BG_CARD, TEXT_WHITE, ACCENT, CYAN, GREEN));
    root->addWidget(progressGauge);

    // ══════════════════════════════════════════════════════════════════
    //  GO / STOP BUTTONS
    // ══════════════════════════════════════════════════════════════════
    QHBoxLayout *btnRow = new QHBoxLayout();
    btnRow->addStretch();

    startButton = new QPushButton("GO");
    startButton->setFixedSize(150, 150);
    startButton->setCursor(Qt::PointingHandCursor);
    startButton->setStyleSheet(QString(
        "QPushButton { background: transparent; color: %1; "
        "border: 4px solid %1; border-radius: 75px; "
        "font-size: 34px; font-weight: bold; }"
        "QPushButton:hover { background: rgba(233, 69, 96, 0.12); "
        "border-color: %2; color: %2; }"
        "QPushButton:pressed { background: %1; color: white; }"
    ).arg(ACCENT, TEXT_WHITE));

    stopButton = new QPushButton("STOP");
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
        "QPushButton:disabled { color: #444; border-color: #333; }"
    ).arg(ACCENT));

    btnRow->addWidget(startButton);
    btnRow->addSpacing(24);
    btnRow->addWidget(stopButton);
    btnRow->addStretch();
    root->addLayout(btnRow);

    // ══════════════════════════════════════════════════════════════════
    //  STATUS
    // ══════════════════════════════════════════════════════════════════
    statusLabel = new QLabel("Ready – press GO to start");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet(QString(
        "color: %1; font-size: 13px; font-weight: bold; background: transparent;"
    ).arg(ACCENT));
    root->addWidget(statusLabel);

    // ══════════════════════════════════════════════════════════════════
    //  LOG
    // ══════════════════════════════════════════════════════════════════
    logArea = new QTextEdit();
    logArea->setReadOnly(true);
    logArea->setMaximumHeight(130);
    logArea->setPlaceholderText("Test log will appear here...");
    logArea->setStyleSheet(QString(
        "QTextEdit { background: #08081a; color: %1; font-size: 10px; "
        "border-radius: 8px; border: 1px solid %2; padding: 6px; "
        "font-family: 'JetBrains Mono', 'Cascadia Code', 'Consolas', monospace; }"
    ).arg(TEXT_DIM, BORDER));
    root->addWidget(logArea);

    // ── Connections ─────────────────────────────────────────────────
    connect(startButton,       &QPushButton::clicked, this, &SpeedtestTab::onStartClicked);
    connect(stopButton,        &QPushButton::clicked, this, &SpeedtestTab::onStopClicked);
    connect(browseServersButton, &QPushButton::clicked, this, &SpeedtestTab::onBrowseServers);
}

// ════════════════════════════════════════════════════════════════════════════
//  CONTROL HELPERS
// ════════════════════════════════════════════════════════════════════════════

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
    pingLabel->setText("—");
    downloadLabel->setText("—");
    uploadLabel->setText("—");
    ispLabel->setText("ISP: —");
    serverLabel->setText(serverCombo->currentIndex() == 0
                         ? "Auto-select (fastest server)"
                         : QString("Server #%1 selected").arg(serverCombo->currentData().toInt()));
    shareLinkLabel->setText("");
    progressGauge->setValue(0);
    progressGauge->setFormat("");
    logArea->clear();
    jsonBuffer.clear();
    isJsonMode       = false;
    isServerListMode = false;
}

// ════════════════════════════════════════════════════════════════════════════
//  SPEEDTEST START
// ════════════════════════════════════════════════════════════════════════════

QString SpeedtestTab::findSpeedtest() const noexcept
{
    QString p = QStandardPaths::findExecutable("speedtest-cli");
    if (p.isEmpty()) p = QStandardPaths::findExecutable("speedtest");
    return p;
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

    QString program = findSpeedtest();
    if (program.isEmpty()) {
        statusLabel->setText(QStringLiteral("<img src=':/svgs/warning.svg' width='16' height='16'> speedtest-cli not found!"));
        logArea->append("Install: sudo pacman -S speedtest-cli");
        return;
    }

    resetDisplay();

    // Args: --json is required for structured output
    QStringList args = {"--json"};
    if (secureCheck->isChecked())     args << "--secure";
    if (singleConnCheck->isChecked()) args << "--single";
    if (shareCheck->isChecked())      args << "--share";

    // Server selection from combo box
    int serverId = serverCombo->currentData().toInt();
    if (serverId > 0) {
        args << "--server" << QString::number(serverId);
        serverLabel->setText(QString("Server #%1 selected").arg(serverId));
    } else {
        serverLabel->setText("Auto-select (fastest server)");
    }

    logArea->append(QString("$ %1 %2").arg(program, args.join(' ')));
    logArea->append("Contacting speedtest.net …");

    setControlsEnabled(false);
    statusLabel->setText("Initializing …");
    progressGauge->setValue(4);
    progressGauge->setFormat("Initializing …");
    progressDot = 0;
    isJsonMode      = true;
    isServerListMode = false;

    elapsedTimer.start();
    progressTimer->start(200);

    startProcess(args);

    if (!process->waitForStarted(5000)) {
        statusLabel->setText(QStringLiteral("<img src=':/svgs/warning.svg' width='16' height='16'> Failed to start process"));
        setControlsEnabled(true);
        progressTimer->stop();
    }
}

// ════════════════════════════════════════════════════════════════════════════
//  SERVER BROWSER
// ════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::onBrowseServers()
{
    QString program = findSpeedtest();
    if (program.isEmpty()) {
        statusLabel->setText(QStringLiteral("<img src=':/svgs/warning.svg' width='16' height='16'> speedtest-cli not found!"));
        return;
    }

    // ── Dialog ──────────────────────────────────────────────────────────
    QDialog dlg(this);
    dlg.setWindowTitle("Browse Speedtest Servers");
    dlg.setMinimumSize(640, 480);
    dlg.setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
    ).arg(BG_DARK, TEXT_WHITE));

    QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
    dlgLayout->setSpacing(12);

    QLabel *info = new QLabel("Fetching server list from speedtest.net …");
    info->setStyleSheet(QString("color: %1; font-size: 12px;").arg(TEXT_DIM));
    dlgLayout->addWidget(info);

    // Table
    QTableWidget *table = new QTableWidget(0, 4);
    table->setObjectName("serverTable");
    table->setHorizontalHeaderLabels({"ID", "Sponsor", "Location", "Distance (km)"});
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
    ).arg(BG_CARD, TEXT_WHITE, BORDER, ACCENT));
    dlgLayout->addWidget(table);

    // Buttons
    QDialogButtonBox *btns = new QDialogButtonBox();
    QPushButton *selectBtn = btns->addButton("Select Server", QDialogButtonBox::AcceptRole);
    selectBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: white; border: none; "
        "border-radius: 5px; padding: 8px 20px; font-weight: bold; }"
        "QPushButton:hover { background: #c0392b; }"
    ).arg(ACCENT));
    btns->addButton(QDialogButtonBox::Cancel);
    btns->setStyleSheet("QPushButton { color: white; }");
    dlgLayout->addWidget(btns);

    // ── Parse – locally from --list or cached ─────────────────────────
    // C++26: Lambda with auto parameters
    auto populateTable = [&](const QString &raw) {
        table->setRowCount(0);
        QStringList const lines = raw.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

        static QRegularExpression const re(
            QStringLiteral(R"(^\s*(\d+)\)\s+(.*)\s+\((.*)\)\s+\[([\d.]+)\s*km\]$)"));
        // Example: "34441) Telenor AB (Stockholm, Sweden) [7726.23 km]"

        std::vector<ServerInfo> parsed;

        for (QString const& line : lines) {
            auto const m = re.match(line.trimmed());
            if (!m.hasMatch()) continue;

            ServerInfo s;
            s.id         = m.captured(1).toInt();
            s.sponsor    = m.captured(2).trimmed();
            s.location   = m.captured(3).trimmed();
            s.distanceKm = m.captured(4).toDouble();

            int const row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(QString::number(s.id)));
            table->setItem(row, 1, new QTableWidgetItem(s.sponsor));
            table->setItem(row, 2, new QTableWidgetItem(s.location));
            table->setItem(row, 3,
                new QTableWidgetItem(QString::number(s.distanceKm, 'f', 1)));
            parsed.push_back(s);
        }

        if (parsed.empty()) {
            info->setText(QStringLiteral("No servers parsed. Check your internet connection."));
        } else {
            info->setText(QStringLiteral("%1 servers found. Double-click or select + Accept.")
                              .arg(static_cast<int>(parsed.size())));
            serverCache = std::move(parsed);  // C++11: Move semantics
        }
    };

    // Do we already have cached servers?
    if (!serverCache.empty()) {
        // Populate from cache
        for (const auto &s : serverCache) {
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(QString::number(s.id)));
            table->setItem(row, 1, new QTableWidgetItem(s.sponsor));
            table->setItem(row, 2, new QTableWidgetItem(s.location));
            table->setItem(row, 3, new QTableWidgetItem(QString::number(s.distanceKm, 'f', 1)));
        }
        info->setText(QString("%1 servers (cached). Double-click or select + Accept.").arg(serverCache.size()));
    } else {
        // Fetch live
        table->setRowCount(0);
        info->setText("Fetching server list …");

        QProcess listProc;
        listProc.setProcessChannelMode(QProcess::MergedChannels);
        listProc.start(program, QStringList{"--list"});

        if (listProc.waitForFinished(30000)) {
            QByteArray raw = listProc.readAll();
            QString text = QString::fromUtf8(raw);
            populateTable(text);
        } else {
            info->setText("Server list fetch timed out.");
        }
    }

    // Double-click = selection
    connect(table, &QTableWidget::cellDoubleClicked, &dlg, [&](int row, int) {
        if (row >= 0) {
            int sid = table->item(row, 0)->text().toInt();
            setSelectedServer(sid);
            dlg.accept();
        }
    });

    // Accept
    connect(btns, &QDialogButtonBox::accepted, &dlg, [&]() {
        int row = table->currentRow();
        if (row >= 0) {
            int sid = table->item(row, 0)->text().toInt();
            setSelectedServer(sid);
        }
    });
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    dlg.exec();
}

void SpeedtestTab::setSelectedServer(int serverId)
{
    // Find the server in cache
    for (const auto &s : serverCache) {
        if (s.id == serverId) {
            serverCombo->setCurrentIndex(
                serverCombo->findData(serverId));
            serverLabel->setText(QString("Server: %1 (%2) — #%3, %4 km")
                                     .arg(s.sponsor, s.location)
                                     .arg(s.id)
                                     .arg(s.distanceKm, 0, 'f', 0));
            statusLabel->setText(QString("Server #%1 selected").arg(serverId));
            logArea->append(QString("Selected server: %1 (%2) — ID %3, %4 km")
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
            serverLabel->setText(QString("Server #%1 selected").arg(serverId));
            return;
        }
    }
    // Add new entry
    serverCombo->addItem(QString("Server #%1").arg(serverId), serverId);
    serverCombo->setCurrentIndex(serverCombo->count() - 1);
    serverLabel->setText(QString("Server #%1 selected (custom)").arg(serverId));
}

// ════════════════════════════════════════════════════════════════════════════
//  DATA I/O
// ════════════════════════════════════════════════════════════════════════════

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

        // Display intermediate output (e.g., "Retrieving...", "Testing from...")
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
    QString dots = QString(progressDot + 1, '.');

    QString stage;
    if      (elapsed <  4000) stage = "Selecting server";
    else if (elapsed < 12000) stage = "Testing ping";
    else if (elapsed < 30000) stage = "Testing download";
    else if (elapsed < 50000) stage = "Testing upload";
    else                      stage = "Finishing";

    statusLabel->setText(stage % " " % dots);
    progressGauge->setFormat(stage % " " % dots);
}

// ════════════════════════════════════════════════════════════════════════════
//  FINISHED
// ════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::onSpeedtestFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    progressTimer->stop();
    setControlsEnabled(true);

    // Log crash exit (e.g. SIGSEGV, SIGKILL) vs normal exit
    if (exitStatus == QProcess::CrashExit) {
        statusLabel->setText(QStringLiteral("<img src=':/svgs/warning.svg' width='16' height='16'> Process crashed (signal)"));
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
        statusLabel->setText(QStringLiteral("<img src=':/svgs/checkmark.svg' width='16' height='16'> Server list loaded into log"));
        progressGauge->setValue(100);
        progressGauge->setFormat(QStringLiteral("Done"));
        isServerListMode = false;
        return;
    }

    // JSON mode – evaluate
    if (isJsonMode && (exitCode == 0 || jsonBuffer.size() > 20)) {
        QJsonParseError err;
        QJsonDocument const doc = QJsonDocument::fromJson(jsonBuffer, &err);

        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            QJsonObject const obj = doc.object();
            updateDisplayFromJson(obj);
            progressGauge->setValue(100);
            progressGauge->setFormat(QStringLiteral("Completed"));
            statusLabel->setText(QStringLiteral("<img src=':/svgs/checkmark.svg' width='16' height='16'> Speed test completed"));
            logArea->append(QStringLiteral("── Test finished ──"));
            isJsonMode = false;
            return;
        }

        // JSON parse error – display raw data
        statusLabel->setText(QStringLiteral("<img src=':/svgs/warning.svg' width='16' height='16'> JSON error: ") + err.errorString());
        logArea->append(QStringLiteral("JSON parse error: ") + err.errorString());
        logArea->append(QStringLiteral("── Raw output ──"));
        logArea->append(QString::fromUtf8(jsonBuffer));
        progressGauge->setFormat(QStringLiteral("Parse error"));
        isJsonMode = false;
        return;
    }

    // Error
    if (exitCode != 0) {
        statusLabel->setText(QStringLiteral("<img src=':/svgs/warning.svg' width='16' height='16'> Test failed (exit %1)")
                                 .arg(exitCode));
        logArea->append(QStringLiteral("speedtest-cli exited with code %1")
                            .arg(exitCode));
        progressGauge->setFormat(QStringLiteral("Failed"));
    } else {
        statusLabel->setText(QStringLiteral("<img src=':/svgs/warning.svg' width='16' height='16'> No JSON data received"));
        progressGauge->setFormat(QStringLiteral("No data"));
    }
    isJsonMode = false;
}

// ════════════════════════════════════════════════════════════════════════════
//  DISPLAY
// ════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::updateDisplayFromJson(const QJsonObject &obj) noexcept
{
    // Ping
    double const ping = obj[QStringLiteral("ping")].toDouble();
    pingLabel->setText(QString::number(ping, 'f', 1));

    // Download / Upload (bits/s → Mbit/s)
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

        serverLabel->setText(QStringLiteral("Server: %1 (%2) · %3 · ID %4 · latency %5 ms")
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
                               "Share Result Image</a>").arg(url, QString::fromLatin1(GREEN)));
        }
    }

    // Log
    logArea->append(QStringLiteral("Ping: %1 ms · Download: %2 Mbps · Upload: %3 Mbps")
                        .arg(ping, 0, 'f', 1)
                        .arg(dl, 0, 'f', 1)
                        .arg(ul, 0, 'f', 1));
}

// ════════════════════════════════════════════════════════════════════════════
//  STOP
// ════════════════════════════════════════════════════════════════════════════

void SpeedtestTab::onStopClicked()
{
    if (process->state() == QProcess::Running) {
        process->kill();
        logArea->append("Cancelled by user.");
            statusLabel->setText(QStringLiteral("<img src=':/svgs/warning.svg' width='16' height='16'> Cancelled"));
        progressGauge->setFormat("Cancelled");
    }
    progressTimer->stop();
    setControlsEnabled(true);
    isJsonMode = false;
    isServerListMode = false;
}
