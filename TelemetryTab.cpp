// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — TelemetryTab.cpp
//  C++26: structured bindings, constexpr, [[nodiscard]]
//  GUI for real-time network telemetry (TelemetryModule /proc/net/dev)
//  with historical aggregation (TelemetryPersistenceModule).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TelemetryTab.h"
#include "Theme.h"
#include "ConfigManager.h"

#include <QHeaderView>
#include <QFrame>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>

// ═══════════════════════════════════════════════════════════════════════════════
TelemetryTab::TelemetryTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();

    // ── Telemetry module ────────────────────────────────────────────────
    mTelemetry = new IPView::Telemetry::TelemetryModule(this);

    connect(mTelemetry, &IPView::Telemetry::TelemetryModule::telemetryUpdated,
            this, &TelemetryTab::onTelemetryUpdated);
    connect(mTelemetry, &IPView::Telemetry::TelemetryModule::errorOccurred,
            this, [this](const QString &msg) {
        statusLabel->setText(QStringLiteral("Error: %1").arg(msg));
    });

    // ── Persistence module ──────────────────────────────────────────────
    mPersistence = new IPView::Telemetry::TelemetryPersistenceModule(this);

    connect(mPersistence, &IPView::Telemetry::TelemetryPersistenceModule::aggregationStored,
            this, &TelemetryTab::onAggregationStored);
    connect(mPersistence, &IPView::Telemetry::TelemetryPersistenceModule::aggregationError,
            this, [this](const QString &msg) {
        persistStatusLabel->setText(QStringLiteral("Aggregation error: %1").arg(msg));
    });
    connect(mPersistence, &IPView::Telemetry::TelemetryPersistenceModule::aggregationCompleted,
            this, [this]() {
        persistStatusLabel->setText(QStringLiteral("Aggregation completed."));
    });

    // ── Restore saved auto-start state ──────────────────────────────────
    if (IPView::Config::Manager::loadTelemetryAutoStart()) {
        persistCheckBox->setChecked(true);
        mPersistence->start(60000);
        updatePersistenceStatus();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
void TelemetryTab::setupUI() noexcept
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // ── Title + monitoring controls ─────────────────────────────────────
    auto *titleBar = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("Network Telemetry"));
    title->setStyleSheet(QStringLiteral("color: %1; font-size: 16px; font-weight: bold;").arg(C_PRIMARY));

    toggleButton = new QPushButton(QStringLiteral("\u25B6 Start Monitoring"));
    toggleButton->setProperty("accent", true);
    toggleButton->setStyleSheet(btnAccentStyle());
    toggleButton->setFixedWidth(180);

    refreshButton = new QPushButton(QStringLiteral("\u27F3 Refresh"));
    refreshButton->setStyleSheet(btnSecondaryStyle());
    refreshButton->setFixedWidth(120);

    titleBar->addWidget(title);
    titleBar->addStretch();
    titleBar->addWidget(refreshButton);
    titleBar->addWidget(toggleButton);
    mainLayout->addLayout(titleBar);

    // ── Status ──────────────────────────────────────────────────────────
    statusLabel = new QLabel(QStringLiteral("Click 'Start Monitoring' to begin."));
    statusLabel->setStyleSheet(statusLabelStyle());
    mainLayout->addWidget(statusLabel);

    // ── Summary cards ───────────────────────────────────────────────────
    auto *summaryRow = new QHBoxLayout();

    auto *rxCard = new QFrame();
    rxCard->setProperty("card", true);
    rxCard->setStyleSheet(cardStyle());
    auto *rxLayout = new QVBoxLayout(rxCard);
    auto *rxTitle = new QLabel(QStringLiteral("Total Download"));
    rxTitle->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;").arg(C_TEXT_DIM));
    totalRxLabel = new QLabel(QStringLiteral("0 B/s"));
    totalRxLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 22px; font-weight: bold;").arg(C_SUCCESS));
    rxLayout->addWidget(rxTitle);
    rxLayout->addWidget(totalRxLabel);

    auto *txCard = new QFrame();
    txCard->setProperty("card", true);
    txCard->setStyleSheet(cardStyle());
    auto *txLayout = new QVBoxLayout(txCard);
    auto *txTitle = new QLabel(QStringLiteral("Total Upload"));
    txTitle->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;").arg(C_TEXT_DIM));
    totalTxLabel = new QLabel(QStringLiteral("0 B/s"));
    totalTxLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 22px; font-weight: bold;").arg(C_PRIMARY));
    txLayout->addWidget(txTitle);
    txLayout->addWidget(totalTxLabel);

    summaryRow->addWidget(rxCard, 1);
    summaryRow->addSpacing(12);
    summaryRow->addWidget(txCard, 1);
    mainLayout->addLayout(summaryRow);

    // ── Interface table ─────────────────────────────────────────────────
    interfaceTable = new QTableWidget(0, 6);
    interfaceTable->setHorizontalHeaderLabels({
        QStringLiteral("Interface"), QStringLiteral("Download"),
        QStringLiteral("Upload"), QStringLiteral("RX Packets"),
        QStringLiteral("TX Packets"), QStringLiteral("Errors")
    });
    interfaceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    interfaceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    interfaceTable->verticalHeader()->setVisible(false);
    interfaceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    interfaceTable->setAlternatingRowColors(true);
    interfaceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(interfaceTable, 1);

    // ════════════════════════════════════════════════════════════════════
    //  PERSISTENCE SECTION
    // ════════════════════════════════════════════════════════════════════
    auto *persistGroup = new QGroupBox(QStringLiteral("Historical Aggregation"));
    persistGroup->setStyleSheet(QStringLiteral(
        "QGroupBox { color: %1; font-weight: bold; border: 1px solid %2; "
        "border-radius: 6px; margin-top: 10px; padding-top: 16px; "
        "background: transparent; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; "
        "padding: 0 4px; }"
    ).arg(C_TEXT_DIM, C_BORDER));
    auto *persistLayout = new QVBoxLayout(persistGroup);
    persistLayout->setSpacing(8);

    // Row 1: toggle + history button
    auto *persistRow1 = new QHBoxLayout();

    persistCheckBox = new QCheckBox(QStringLiteral("Enable background aggregation (every 60s)"));
    persistCheckBox->setStyleSheet(QStringLiteral(
        "QCheckBox { color: %1; font-size: 11px; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; "
        "border: 1px solid %2; background: %3; }"
        "QCheckBox::indicator:checked { background-color: %4; border-color: %4; }"
    ).arg(C_TEXT, C_BORDER, C_BG_ELEVATED, C_ACCENT));
    persistCheckBox->setToolTip(QStringLiteral("Automatically aggregate telemetry stats into SQLite every 60 seconds. "
                                                "Setting persists across restarts."));

    historyButton = new QPushButton(QStringLiteral("Show History"));
    historyButton->setStyleSheet(btnSecondaryStyle());
    historyButton->setFixedWidth(140);

    persistRow1->addWidget(persistCheckBox);
    persistRow1->addStretch();
    persistRow1->addWidget(historyButton);
    persistLayout->addLayout(persistRow1);

    // Row 2: status + latest aggregated stats
    auto *persistRow2 = new QHBoxLayout();

    persistStatusLabel = new QLabel(QStringLiteral("Persistence: inactive"));
    persistStatusLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 10px;").arg(C_TEXT_DIM));

    auto *latestRxTitle = new QLabel(QStringLiteral("Latest avg RX:"));
    latestRxTitle->setStyleSheet(QStringLiteral("color: %1; font-size: 10px;").arg(C_TEXT_DIM));
    latestRxLabel = new QLabel(QStringLiteral("--"));
    latestRxLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 10px; font-weight: bold;").arg(C_SUCCESS));

    auto *latestTxTitle = new QLabel(QStringLiteral("TX:"));
    latestTxTitle->setStyleSheet(QStringLiteral("color: %1; font-size: 10px;").arg(C_TEXT_DIM));
    latestTxLabel = new QLabel(QStringLiteral("--"));
    latestTxLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 10px; font-weight: bold;").arg(C_PRIMARY));

    persistRow2->addWidget(persistStatusLabel);
    persistRow2->addStretch();
    persistRow2->addWidget(latestRxTitle);
    persistRow2->addWidget(latestRxLabel);
    persistRow2->addSpacing(12);
    persistRow2->addWidget(latestTxTitle);
    persistRow2->addWidget(latestTxLabel);
    persistLayout->addLayout(persistRow2);

    mainLayout->addWidget(persistGroup);

    // ── Connections ─────────────────────────────────────────────────────
    connect(toggleButton,       &QPushButton::clicked, this, &TelemetryTab::onToggleMonitoring);
    connect(refreshButton,      &QPushButton::clicked, this, &TelemetryTab::onRefreshInterfaces);
    connect(persistCheckBox,    &QCheckBox::toggled,   this, &TelemetryTab::onTogglePersistence);
    connect(historyButton,      &QPushButton::clicked, this, &TelemetryTab::onShowAggregationHistory);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Slots — Live monitoring
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryTab::onTelemetryUpdated(const QList<IPView::Telemetry::InterfaceInfo> &interfaces)
{
    updateTable(interfaces);

    double totalRx = 0.0, totalTx = 0.0;
    for (auto const &iface : interfaces) {
        totalRx += iface.rxSpeedBps;
        totalTx += iface.txSpeedBps;
    }

    totalRxLabel->setText(formatSpeed(totalRx));
    totalTxLabel->setText(formatSpeed(totalTx));

    statusLabel->setText(QStringLiteral("Monitoring %1 interfaces \u2013 last update: %2")
                             .arg(interfaces.size())
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))));
}

void TelemetryTab::onToggleMonitoring()
{
    if (mMonitoring) {
        mTelemetry->stopMonitoring();
        toggleButton->setText(QStringLiteral("\u25B6 Start Monitoring"));
        toggleButton->setStyleSheet(btnAccentStyle());
        statusLabel->setText(QStringLiteral("Monitoring stopped."));
        mMonitoring = false;
    } else {
        mTelemetry->startMonitoring(2000);
        toggleButton->setText(QStringLiteral("\u23F9 Stop Monitoring"));
        toggleButton->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: %1; color: %2; "
            "border: none; border-radius: %3; padding: %4; font-weight: bold; }"
        ).arg(C_ERROR, C_TEXT, RADIUS_MD, PADDING_BTN));
        statusLabel->setText(QStringLiteral("Starting monitoring..."));
        mMonitoring = true;
    }
}

void TelemetryTab::onRefreshInterfaces()
{
    if (mMonitoring) {
        statusLabel->setText(QStringLiteral("Refreshing..."));
        mTelemetry->stopMonitoring();
        mTelemetry->startMonitoring(2000);
    } else {
        QStringList const ifaces = mTelemetry->availableInterfaces();
        QList<IPView::Telemetry::InterfaceInfo> infos;
        for (QString const &name : ifaces) {
            IPView::Telemetry::InterfaceInfo info;
            info.name = name;
            auto const result = mTelemetry->fetchStats(name.toStdString());
            if (result.has_value()) {
                info.current = *result;
            }
            infos.append(info);
        }
        updateTable(infos);
        statusLabel->setText(QStringLiteral("Interfaces refreshed \u2013 %1 found.").arg(ifaces.size()));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Slots — Persistence
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryTab::onTogglePersistence()
{
    if (persistCheckBox->isChecked()) {
        mPersistence->start(60000);
        // Trigger first aggregation immediately
        mPersistence->aggregateNow();
    } else {
        mPersistence->stop();
    }

    // Save preference
    IPView::Config::Manager::saveTelemetryAutoStart(persistCheckBox->isChecked());
    IPView::Config::Manager::sync();

    updatePersistenceStatus();
}

void TelemetryTab::onAggregationStored(const IPView::Storage::AggregatedTelemetryEntry &record)
{
    latestRxLabel->setText(formatSpeed(record.avgRxSpeed));
    latestTxLabel->setText(formatSpeed(record.avgTxSpeed));
    persistStatusLabel->setText(QStringLiteral("Last aggregation: %1 \u2013 %2")
                                    .arg(record.interfaceName)
                                    .arg(record.windowEnd.toString(QStringLiteral("hh:mm:ss"))));
}

void TelemetryTab::onShowAggregationHistory()
{
    // Fetch latest aggregation records
    auto const records = mPersistence->getHistory(50);

    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Telemetry Aggregation History"));
    dlg.setMinimumSize(800, 500);
    dlg.setStyleSheet(QStringLiteral(
        "QDialog { background-color: %1; color: %2; }"
    ).arg(C_BG, C_TEXT));

    auto *layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);

    QLabel *header = new QLabel(
        records.empty()
            ? QStringLiteral("No aggregation data yet. Enable background aggregation first.")
            : QStringLiteral("Showing last %1 aggregation records (newest first)").arg(records.size()));
    header->setStyleSheet(QStringLiteral("color: %1; font-size: 12px; font-weight: bold;").arg(C_TEXT));
    layout->addWidget(header);

    if (!records.empty()) {
        QTableWidget *table = new QTableWidget(static_cast<int>(records.size()), 8);
        table->setHorizontalHeaderLabels({
            QStringLiteral("Interface"), QStringLiteral("Avg RX"), QStringLiteral("Avg TX"),
            QStringLiteral("Min RX"), QStringLiteral("Max RX"),
            QStringLiteral("Min TX"), QStringLiteral("Max TX"),
            QStringLiteral("Time")
        });
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setAlternatingRowColors(true);
        table->setStyleSheet(QStringLiteral(
            "QTableWidget { background: %1; color: %2; border: 1px solid %3; "
            "border-radius: 6px; gridline-color: %3; font-size: 11px; }"
            "QTableWidget::item { padding: 4px 8px; }"
            "QTableWidget::item:selected { background: %4; color: white; }"
            "QHeaderView::section { background: %1; color: %2; "
            "border: none; border-bottom: 1px solid %3; padding: 6px; "
            "font-weight: bold; font-size: 11px; }"
        ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, C_ACCENT));

        int row = 0;
        for (auto const &r : records) {
            auto rxAvgColor = (r.avgRxSpeed > 1.0e6) ? C_SUCCESS : C_TEXT_DIM;
            auto txAvgColor = (r.avgTxSpeed > 1.0e6) ? C_PRIMARY : C_TEXT_DIM;

            auto *ifaceItem = new QTableWidgetItem(r.interfaceName);
            ifaceItem->setForeground(QBrush(QColor(C_PRIMARY)));

            auto *avgRxItem = new QTableWidgetItem(formatSpeed(r.avgRxSpeed));
            avgRxItem->setForeground(QBrush(QColor(rxAvgColor)));

            auto *avgTxItem = new QTableWidgetItem(formatSpeed(r.avgTxSpeed));
            avgTxItem->setForeground(QBrush(QColor(txAvgColor)));

            auto *minRxItem = new QTableWidgetItem(formatSpeed(r.minRxSpeed));
            auto *maxRxItem = new QTableWidgetItem(formatSpeed(r.maxRxSpeed));
            auto *minTxItem = new QTableWidgetItem(formatSpeed(r.minTxSpeed));
            auto *maxTxItem = new QTableWidgetItem(formatSpeed(r.maxTxSpeed));

            auto *timeItem = new QTableWidgetItem(
                r.windowEnd.toString(QStringLiteral("yyyy-MM-dd hh:mm")));

            table->setItem(row, 0, ifaceItem);
            table->setItem(row, 1, avgRxItem);
            table->setItem(row, 2, avgTxItem);
            table->setItem(row, 3, minRxItem);
            table->setItem(row, 4, maxRxItem);
            table->setItem(row, 5, minTxItem);
            table->setItem(row, 6, maxTxItem);
            table->setItem(row, 7, timeItem);
            ++row;
        }

        layout->addWidget(table, 1);
    }

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close);
    btns->setStyleSheet(QStringLiteral("QPushButton { color: %1; }").arg(C_TEXT));
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(btns);

    dlg.exec();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private helpers
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryTab::updatePersistenceStatus() noexcept
{
    if (mPersistence->isRunning()) {
        persistStatusLabel->setText(QStringLiteral("Persistence: active (60s interval)"));
        persistStatusLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 10px; font-weight: bold;").arg(C_SUCCESS));
    } else {
        persistStatusLabel->setText(QStringLiteral("Persistence: inactive"));
        persistStatusLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 10px;").arg(C_TEXT_DIM));
        latestRxLabel->setText(QStringLiteral("--"));
        latestTxLabel->setText(QStringLiteral("--"));
    }
}

void TelemetryTab::updateTable(const QList<IPView::Telemetry::InterfaceInfo> &interfaces) noexcept
{
    interfaceTable->setRowCount(static_cast<int>(interfaces.size()));

    for (int i = 0; i < static_cast<int>(interfaces.size()); ++i) {
        auto const &info = interfaces[i];

        auto *nameItem = new QTableWidgetItem(info.name);
        nameItem->setForeground(QBrush(QColor(C_PRIMARY)));
        nameItem->setTextAlignment(Qt::AlignCenter);

        auto *rxItem = new QTableWidgetItem(formatSpeed(info.rxSpeedBps));
        rxItem->setForeground(QBrush(QColor(C_SUCCESS)));
        rxItem->setTextAlignment(Qt::AlignCenter);

        auto *txItem = new QTableWidgetItem(formatSpeed(info.txSpeedBps));
        txItem->setForeground(QBrush(QColor(C_PRIMARY)));
        txItem->setTextAlignment(Qt::AlignCenter);

        auto *rxPackets = new QTableWidgetItem(QString::number(info.current.rxPackets));
        rxPackets->setTextAlignment(Qt::AlignCenter);

        auto *txPackets = new QTableWidgetItem(QString::number(info.current.txPackets));
        txPackets->setTextAlignment(Qt::AlignCenter);

        auto *errItem = new QTableWidgetItem(
            QStringLiteral("%1 / %2").arg(info.current.rxErrors).arg(info.current.txErrors));
        errItem->setForeground(
            (info.current.rxErrors > 0 || info.current.txErrors > 0)
                ? QBrush(QColor(C_ERROR))
                : QBrush(QColor(C_TEXT_DIM)));
        errItem->setTextAlignment(Qt::AlignCenter);

        interfaceTable->setItem(i, 0, nameItem);
        interfaceTable->setItem(i, 1, rxItem);
        interfaceTable->setItem(i, 2, txItem);
        interfaceTable->setItem(i, 3, rxPackets);
        interfaceTable->setItem(i, 4, txPackets);
        interfaceTable->setItem(i, 5, errItem);
    }
}

QString TelemetryTab::formatSpeed(double bytesPerSec) noexcept
{
    if (bytesPerSec < 0.0) bytesPerSec = 0.0;

    static constexpr const char* units[] = {"B/s", "KB/s", "MB/s", "GB/s"};
    int unitIndex = 0;
    double value = bytesPerSec;

    while (value >= 1024.0 && unitIndex < 3) {
        value /= 1024.0;
        ++unitIndex;
    }

    return QStringLiteral("%1 %2")
        .arg(value, 0, 'f', (unitIndex == 0) ? 0 : 1)
        .arg(QLatin1StringView(units[unitIndex]));
}
