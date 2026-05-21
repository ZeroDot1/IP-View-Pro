// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.12.0 — AlertTab.cpp
//  C++26: structured bindings, noexcept, [[nodiscard]]
//  GUI tab displaying active alerts from AlertEngine.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "AlertTab.h"
#include "Theme.h"

#include <QHeaderView>
#include <QDateTime>
#include <QMessageBox>

namespace IPView::UI {

AlertTab::AlertTab(IPView::Alert::AlertEngine *engine, QWidget *parent) noexcept
    : QWidget(parent), mEngine(engine)
{
    setupUI();

    if (mEngine) {
        connect(mEngine, &IPView::Alert::AlertEngine::alertTriggered,
                this, &AlertTab::onAlertTriggered);
        connect(mEngine, &IPView::Alert::AlertEngine::alertsChanged,
                this, &AlertTab::onAlertsChanged);
    }

    // Auto-refresh every 3 seconds
    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(3000);
    connect(mRefreshTimer, &QTimer::timeout, this, &AlertTab::refreshDisplay);
    mRefreshTimer->start();

    refreshDisplay();
}

void AlertTab::setupUI() noexcept
{
    auto *mainLayout = new QVBoxLayout(this);

    // ── Status bar ──────────────────────────────────────────────────────
    auto *statusRow = new QHBoxLayout();
    mStatusLabel = new QLabel(QStringLiteral("No active alerts"));
    mStatusLabel->setStyleSheet(onlineLabelStyle());
    statusRow->addWidget(mStatusLabel);
    statusRow->addStretch();

    mAckButton = new QPushButton(QStringLiteral(" Acknowledge Selected"));
    mAckButton->setStyleSheet(btnSecondaryStyle());
    statusRow->addWidget(mAckButton);

    mClearButton = new QPushButton(QStringLiteral(" Clear Acknowledged"));
    mClearButton->setStyleSheet(btnSmallStyle());
    statusRow->addWidget(mClearButton);

    mainLayout->addLayout(statusRow);

    // ── Alert table ─────────────────────────────────────────────────────
    mTable = new QTableWidget(0, 6);
    mTable->setHorizontalHeaderLabels({
        QStringLiteral("Time"),
        QStringLiteral("Severity"),
        QStringLiteral("Category"),
        QStringLiteral("Title"),
        QStringLiteral("Message"),
        QStringLiteral("Source")
    });
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    mTable->verticalHeader()->setVisible(false);
    mTable->setAlternatingRowColors(true);
    mTable->setStyleSheet(QStringLiteral(
        "QTableWidget { background: %1; color: %2; gridline-color: %3; "
        "border-radius: %4; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section { background: %3; color: %2; padding: 6px; "
        "border: none; font-weight: bold; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_SM));

    mainLayout->addWidget(mTable, 1);

    // ── Connections ─────────────────────────────────────────────────────
    connect(mAckButton, &QPushButton::clicked, this, &AlertTab::onAcknowledgeClicked);
    connect(mClearButton, &QPushButton::clicked, this, &AlertTab::onClearClicked);
}

void AlertTab::onAlertTriggered(const IPView::Alert::Alert &alert)
{
    Q_UNUSED(alert)
    refreshDisplay();
}

void AlertTab::onAlertsChanged()
{
    refreshDisplay();
}

void AlertTab::onAcknowledgeClicked()
{
    int const row = mTable->currentRow();
    if (row < 0) {
        QMessageBox::information(this, QStringLiteral("No Selection"),
            QStringLiteral("Please select an alert to acknowledge."));
        return;
    }

    if (mEngine) {
        auto const alerts = mEngine->allAlerts();
        if (row < static_cast<int>(alerts.size())) {
            mEngine->acknowledgeAlert(QString::fromStdString(alerts[static_cast<std::size_t>(row)].id.toStdString()));
            refreshDisplay();
        }
    }
}

void AlertTab::onClearClicked()
{
    if (mEngine) {
        mEngine->clearAcknowledged();
        refreshDisplay();
    }
}

void AlertTab::refreshDisplay()
{
    if (!mEngine) return;
    populateTable();
}

void AlertTab::populateTable() noexcept
{
    if (!mEngine || !mTable) return;

    auto const alerts = mEngine->allAlerts();
    mTable->setRowCount(0);

    int activeCount = 0;
    for (auto const &alert : alerts) {
        if (alert.acknowledged) continue;
        ++activeCount;

        int const row = mTable->rowCount();
        mTable->insertRow(row);

        mTable->setItem(row, 0, new QTableWidgetItem(
            alert.timestamp.toString(QStringLiteral("hh:mm:ss"))));

        auto *sevItem = new QTableWidgetItem(severityIcon(alert.severity) + QStringLiteral(" ") +
            [alert]() {
                switch (alert.severity) {
                    case IPView::Alert::Severity::Critical: return QStringLiteral("Critical");
                    case IPView::Alert::Severity::Warning:  return QStringLiteral("Warning");
                    default: return QStringLiteral("Info");
                }
            }());
        sevItem->setForeground(QColor(severityColor(alert.severity)));
        mTable->setItem(row, 1, sevItem);

        auto *catItem = new QTableWidgetItem(
            [alert]() {
                switch (alert.category) {
                    case IPView::Alert::Category::Telemetry: return QStringLiteral("Telemetry");
                    case IPView::Alert::Category::Security:  return QStringLiteral("Security");
                    case IPView::Alert::Category::System:    return QStringLiteral("System");
                    default: return QStringLiteral("Custom");
                }
            }());
        mTable->setItem(row, 2, catItem);

        mTable->setItem(row, 3, new QTableWidgetItem(alert.title));
        mTable->setItem(row, 4, new QTableWidgetItem(alert.message));
        mTable->setItem(row, 5, new QTableWidgetItem(alert.source));
    }

    // Update status
    if (activeCount > 0) {
        mStatusLabel->setText(QStringLiteral("%1 active alert%2").arg(activeCount).arg(activeCount == 1 ? "" : "s"));
        mStatusLabel->setStyleSheet(statusLabelStyle());
    } else {
        mStatusLabel->setText(QStringLiteral("No active alerts"));
        mStatusLabel->setStyleSheet(onlineLabelStyle());
    }

    mTable->resizeColumnsToContents();
}

QString AlertTab::severityIcon(IPView::Alert::Severity sev) noexcept
{
    switch (sev) {
        case IPView::Alert::Severity::Critical: return QStringLiteral("\u26A0");
        case IPView::Alert::Severity::Warning:  return QStringLiteral("\u26A1");
        default: return QStringLiteral("\u2139");
    }
}

QString AlertTab::severityColor(IPView::Alert::Severity sev) noexcept
{
    switch (sev) {
        case IPView::Alert::Severity::Critical: return C_ERROR;
        case IPView::Alert::Severity::Warning:  return C_WARNING;
        default: return C_INFO;
    }
}

} // namespace IPView::UI
