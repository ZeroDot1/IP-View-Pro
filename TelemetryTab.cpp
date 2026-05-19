// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — TelemetryTab.cpp
//  C++26: structured bindings, constexpr, [[nodiscard]]
//  GUI for real-time network telemetry (TelemetryModule /proc/net/dev).
//  Shows interfaces with RX/TX rates, packets, and errors.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TelemetryTab.h"
#include "Theme.h"

#include <QHeaderView>
#include <QFrame>
#include <QDateTime>

// ═══════════════════════════════════════════════════════════════════════════════
TelemetryTab::TelemetryTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    mTelemetry = new IPView::Telemetry::TelemetryModule(this);

    connect(mTelemetry, &IPView::Telemetry::TelemetryModule::telemetryUpdated,
            this, &TelemetryTab::onTelemetryUpdated);
    connect(mTelemetry, &IPView::Telemetry::TelemetryModule::errorOccurred,
            this, [this](const QString &msg) {
        statusLabel->setText(QStringLiteral("Error: %1").arg(msg));
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
void TelemetryTab::setupUI() noexcept
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // ── Title ──────────────────────────────────────────────────────────────
    auto *titleBar = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("Network Telemetry"));
    title->setStyleSheet(QStringLiteral("color: %1; font-size: 16px; font-weight: bold;").arg(C_PRIMARY));

    toggleButton = new QPushButton(QStringLiteral("▶ Start Monitoring"));
    toggleButton->setProperty("accent", true);
    toggleButton->setStyleSheet(btnAccentStyle());
    toggleButton->setFixedWidth(180);

    refreshButton = new QPushButton(QStringLiteral("⟳ Refresh"));
    refreshButton->setStyleSheet(btnSecondaryStyle());
    refreshButton->setFixedWidth(120);

    titleBar->addWidget(title);
    titleBar->addStretch();
    titleBar->addWidget(refreshButton);
    titleBar->addWidget(toggleButton);
    mainLayout->addLayout(titleBar);

    // ── Status ─────────────────────────────────────────────────────────────
    statusLabel = new QLabel(QStringLiteral("Click 'Start Monitoring' to begin."));
    statusLabel->setStyleSheet(statusLabelStyle());
    mainLayout->addWidget(statusLabel);

    // ── Summary cards ──────────────────────────────────────────────────────
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

    // ── Interface table ────────────────────────────────────────────────────
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

    // ── Connections ────────────────────────────────────────────────────────
    connect(toggleButton,  &QPushButton::clicked, this, &TelemetryTab::onToggleMonitoring);
    connect(refreshButton, &QPushButton::clicked, this, &TelemetryTab::onRefreshInterfaces);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Slots
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryTab::onTelemetryUpdated(const QList<IPView::Telemetry::InterfaceInfo> &interfaces)
{
    updateTable(interfaces);

    // Calculate total RX/TX
    double totalRx = 0.0, totalTx = 0.0;
    for (auto const &iface : interfaces) {
        totalRx += iface.rxSpeedBps;
        totalTx += iface.txSpeedBps;
    }

    totalRxLabel->setText(formatSpeed(totalRx));
    totalTxLabel->setText(formatSpeed(totalTx));

    statusLabel->setText(QStringLiteral("Monitoring %1 interfaces – last update: %2")
                             .arg(interfaces.size())
                             .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"))));
}

void TelemetryTab::onToggleMonitoring()
{
    if (mMonitoring) {
        mTelemetry->stopMonitoring();
        toggleButton->setText(QStringLiteral("▶ Start Monitoring"));
        toggleButton->setStyleSheet(btnAccentStyle());
        statusLabel->setText(QStringLiteral("Monitoring stopped."));
        mMonitoring = false;
    } else {
        mTelemetry->startMonitoring(2000);
        toggleButton->setText(QStringLiteral("⏹ Stop Monitoring"));
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
        // Manual refresh triggers the next telemetry update
        statusLabel->setText(QStringLiteral("Refreshing..."));
        // Das TelemetryModule aktualisiert automatisch via Timer
        // We can reset the timer briefly for an immediate update
        mTelemetry->stopMonitoring();
        mTelemetry->startMonitoring(2000);
    } else {
        // Fetch current values once
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
        statusLabel->setText(QStringLiteral("Interfaces refreshed – %1 found.").arg(ifaces.size()));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private Helpers
// ═══════════════════════════════════════════════════════════════════════════════

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

    // Automatic units: B/s, KB/s, MB/s, GB/s
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
