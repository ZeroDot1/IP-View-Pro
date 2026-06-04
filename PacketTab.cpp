// ═══════════════════════════════════════════════════════════════════════════════
// IPView Pro v2.15.0 — PacketTab.cpp
// C++26: structured bindings, noexcept, [[nodiscard]], std::ranges
// Professional UI displaying live connections from PacketModule.
// Features: protocol/state filtering, search, statistics, export, theme,
//           signal-driven updates, detail dialogs, empty state, connection counting.
// Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "PacketTab.h"
#include "Theme.h"
#include "SecurityUtil.h"
#include "Timeouts.hpp"

#include <QHeaderView>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QDialog>
#include <QFormLayout>
#include <QScrollArea>
#include <ranges>
#include <algorithm>
#include "ErrorDialog.h"

namespace IPView::UI {

PacketTab::PacketTab(IPView::Packet::PacketModule *module, QWidget *parent)
    : QWidget(parent), mModule(module)
{
    setupUI();
    setupConnections();

    // Auto-refresh at the default packet polling interval
    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(IPView::Timeouts::POLL_PACKET_DEFAULT);
    connect(mRefreshTimer, &QTimer::timeout, this, &PacketTab::refreshData);
    if (mAutoRefresh) mRefreshTimer->start();

    // Initial data load
    refreshData();
}

void PacketTab::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // ── Title bar ───────────────────────────────────────────────────────
    auto *titleBar = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("Active Network Connections"));
    title->setStyleSheet(QStringLiteral("color: %1; font-size: 18px; font-weight: bold;").arg(C_PRIMARY));
    titleBar->addWidget(title);
    titleBar->addStretch();

    mLastUpdateLabel = new QLabel(QStringLiteral("Last update: --"));
    mLastUpdateLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;").arg(C_TEXT_DIM));
    titleBar->addWidget(mLastUpdateLabel);
    mainLayout->addLayout(titleBar);

    // ── Status message ──────────────────────────────────────────────────
    mStatusMsg = new QLabel();
    mStatusMsg->setStyleSheet(QStringLiteral("color: %1; font-size: 11px; font-weight: bold; padding: 4px 8px; border-radius: 4px;").arg(C_SUCCESS));
    mStatusMsg->hide();
    mainLayout->addWidget(mStatusMsg);

    // ── Control bar ─────────────────────────────────────────────────────
    mFilterBar = new QFrame();
    mFilterBar->setProperty("card", true);
    mFilterBar->setStyleSheet(cardStyle());
    auto *filterLayout = new QHBoxLayout(mFilterBar);
    filterLayout->setSpacing(10);
    filterLayout->setContentsMargins(14, 10, 14, 10);

    // Protocol filter
    auto *protoLbl = new QLabel(QStringLiteral("Protocol:"));
    protoLbl->setStyleSheet(QStringLiteral("color: %1; font-size: 12px; font-weight: bold;").arg(C_TEXT_SEC));
    filterLayout->addWidget(protoLbl);
    mProtoFilter = new QComboBox();
    mProtoFilter->addItems({QStringLiteral("All"), QStringLiteral("TCP"), QStringLiteral("UDP")});
    mProtoFilter->setFixedWidth(110);
    mProtoFilter->setStyleSheet(comboStyle());
    filterLayout->addWidget(mProtoFilter);

    // State filter
    auto *stateLbl = new QLabel(QStringLiteral("State:"));
    stateLbl->setStyleSheet(QStringLiteral("color: %1; font-size: 12px; font-weight: bold;").arg(C_TEXT_SEC));
    filterLayout->addWidget(stateLbl);
    mStateFilter = new QComboBox();
    mStateFilter->addItems({
        QStringLiteral("All"), QStringLiteral("ESTABLISHED"), QStringLiteral("LISTEN"),
        QStringLiteral("TIME_WAIT"), QStringLiteral("CLOSE_WAIT"), QStringLiteral("SYN_SENT"),
        QStringLiteral("SYN_RECV"), QStringLiteral("FIN_WAIT1"), QStringLiteral("FIN_WAIT2"),
        QStringLiteral("CLOSE"), QStringLiteral("LAST_ACK"), QStringLiteral("CLOSING"),
        QStringLiteral("UNKNOWN")
    });
    mStateFilter->setFixedWidth(150);
    mStateFilter->setStyleSheet(comboStyle());
    filterLayout->addWidget(mStateFilter);

    // Search
    auto *searchLbl = new QLabel(QStringLiteral("Search:"));
    searchLbl->setStyleSheet(QStringLiteral("color: %1; font-size: 12px; font-weight: bold;").arg(C_TEXT_SEC));
    filterLayout->addWidget(searchLbl);
    mSearchEdit = new QLineEdit();
    mSearchEdit->setPlaceholderText(QStringLiteral("IP, port, UID..."));
    mSearchEdit->setFixedWidth(180);
    mSearchEdit->setStyleSheet(inputStyle());
    filterLayout->addWidget(mSearchEdit);

    filterLayout->addStretch();

    // Action buttons
    mRefreshBtn = new QPushButton(QStringLiteral("\u21BB Refresh"));
    mRefreshBtn->setStyleSheet(btnAccentStyle());
    mRefreshBtn->setFixedWidth(100);
    mRefreshBtn->setCursor(Qt::PointingHandCursor);
    filterLayout->addWidget(mRefreshBtn);

    mAutoRefreshBtn = new QPushButton(QStringLiteral("\u23F8 Auto: ON"));
    mAutoRefreshBtn->setStyleSheet(btnSecondaryStyle());
    mAutoRefreshBtn->setFixedWidth(110);
    mAutoRefreshBtn->setCursor(Qt::PointingHandCursor);
    filterLayout->addWidget(mAutoRefreshBtn);

    mExportBtn = new QPushButton(QStringLiteral("\u2B07 Export CSV"));
    mExportBtn->setStyleSheet(btnSecondaryStyle());
    mExportBtn->setFixedWidth(110);
    mExportBtn->setCursor(Qt::PointingHandCursor);
    filterLayout->addWidget(mExportBtn);

    mainLayout->addWidget(mFilterBar);

    // ── Statistics bar ──────────────────────────────────────────────────
    auto *statsBar = new QHBoxLayout();
    statsBar->setSpacing(12);

    auto createStatCard = [&](const QString &labelText, QLabel *&valueLabel,
                               const QString &color, const QString &icon) -> QFrame* {
        auto *card = new QFrame();
        card->setProperty("card", true);
        card->setStyleSheet(cardStyle());
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(14, 10, 14, 10);
        layout->setSpacing(4);

        auto *iconLbl = new QLabel(icon);
        iconLbl->setStyleSheet(QStringLiteral("color: %1; font-size: 14px;").arg(color));
        iconLbl->setAlignment(Qt::AlignCenter);

        auto *lbl = new QLabel(labelText);
        lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 10px; font-weight: bold; text-transform: uppercase;").arg(C_TEXT_DIM));
        lbl->setAlignment(Qt::AlignCenter);

        valueLabel = new QLabel(QStringLiteral("0"));
        valueLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 20px; font-weight: bold;").arg(color));
        valueLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(iconLbl, 0, Qt::AlignCenter);
        layout->addWidget(lbl, 0, Qt::AlignCenter);
        layout->addWidget(valueLabel, 0, Qt::AlignCenter);
        return card;
    };

    statsBar->addWidget(createStatCard(QStringLiteral("TCP"), mTcpCountLabel, C_SUCCESS, QStringLiteral("\u25CF")), 1);
    statsBar->addWidget(createStatCard(QStringLiteral("UDP"), mUdpCountLabel, C_PRIMARY, QStringLiteral("\u25CF")), 1);
    statsBar->addWidget(createStatCard(QStringLiteral("Total"), mTotalCountLabel, C_TEXT, QStringLiteral("\u25CF")), 1);
    statsBar->addWidget(createStatCard(QStringLiteral("Established"), mEstablishedLabel, C_INFO, QStringLiteral("\u25CF")), 1);

    mainLayout->addLayout(statsBar);

    // ── Empty state label ───────────────────────────────────────────────
    mEmptyLabel = new QLabel();
    mEmptyLabel->setAlignment(Qt::AlignCenter);
    mEmptyLabel->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 14px; padding: 40px; }"
    ).arg(C_TEXT_DIM));
    mEmptyLabel->hide();
    mainLayout->addWidget(mEmptyLabel, 1);

    // ── Connection table ────────────────────────────────────────────────
    mTable = new QTableWidget(0, 7);
    mTable->setHorizontalHeaderLabels({
        QStringLiteral("Protocol"),
        QStringLiteral("Local Address"),
        QStringLiteral("Local Port"),
        QStringLiteral("Remote Address"),
        QStringLiteral("Remote Port"),
        QStringLiteral("State"),
        QStringLiteral("UID")
    });
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    mTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    mTable->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    mTable->verticalHeader()->setVisible(false);
    applyTableStyle(mTable);
    mTable->setStyleSheet(QStringLiteral(
        "QTableWidget { background: %1; color: %2; gridline-color: %3; "
        "border: 1px solid %3; border-radius: %4; }"
        "QTableWidget::item { padding: 6px 8px; }"
        "QTableWidget::item:selected { background: %5; color: %2; }"
        "QHeaderView::section { background: %1; color: %6; padding: 8px; "
        "border: none; border-bottom: 1px solid %3; font-weight: bold; font-size: 11px; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_SM, C_ACCENT, C_TEXT_DIM));

    mainLayout->addWidget(mTable, 1);
}

void PacketTab::setupConnections()
{
    // ── Signal-driven updates from PacketModule ─────────────────────────
    if (mModule) {
        connect(mModule, &IPView::Packet::PacketModule::connectionsUpdated,
                this, &PacketTab::onSnapshotReceived);
    }

    // ── Filter / Search ─────────────────────────────────────────────────
    connect(mProtoFilter,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PacketTab::onFilterChanged);
    connect(mStateFilter,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PacketTab::onFilterChanged);
    connect(mSearchEdit,    &QLineEdit::textChanged,
            this, &PacketTab::onSearchChanged);

    // ── Buttons ─────────────────────────────────────────────────────────
    connect(mRefreshBtn,    &QPushButton::clicked,
            this, &PacketTab::refreshData);
    connect(mAutoRefreshBtn, &QPushButton::clicked,
            this, &PacketTab::onToggleAutoRefresh);
    connect(mExportBtn,     &QPushButton::clicked,
            this, &PacketTab::onExportClicked);

    // ── Double-click for detail view ────────────────────────────────────
    connect(mTable, &QTableWidget::cellDoubleClicked,
            this, &PacketTab::onRowDoubleClicked);
}

void PacketTab::onSnapshotReceived(const IPView::Packet::ConnectionSnapshot &snapshot)
{
    mLastSnapshot = snapshot;
    updateStats(snapshot);
    populateTable(snapshot);
    mLastUpdateLabel->setText(QStringLiteral("Last update: %1").arg(
        QTime::currentTime().toString(QStringLiteral("hh:mm:ss"))));
}

void PacketTab::refreshData()
{
    if (!mModule) return;
    auto snapshot = mModule->pollNow();
    onSnapshotReceived(snapshot);
}

void PacketTab::onFilterChanged()
{
    populateTable(mLastSnapshot);
}

void PacketTab::onSearchChanged()
{
    populateTable(mLastSnapshot);
}

void PacketTab::onToggleAutoRefresh()
{
    mAutoRefresh = !mAutoRefresh;
    if (mAutoRefresh) {
        mRefreshTimer->start();
        mAutoRefreshBtn->setText(QStringLiteral("\u23F8 Auto: ON"));
        mAutoRefreshBtn->setStyleSheet(btnSecondaryStyle());
    } else {
        mRefreshTimer->stop();
        mAutoRefreshBtn->setText(QStringLiteral("\u25B6 Auto: OFF"));
        mAutoRefreshBtn->setStyleSheet(btnSecondaryStyle());
    }
}

void PacketTab::onExportClicked()
{
    QString const fileName = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export Connections"),
        QStringLiteral("connections_%1.csv").arg(
            QDate::currentDate().toString(QStringLiteral("yyyyMMdd"))),
        QStringLiteral("CSV Files (*.csv)"));

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        IPView::UI::ErrorDialog::showError(this, QStringLiteral("Export Error"),
            QStringLiteral("Could not open file for writing:\n%1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << "Protocol,Local Address,Local Port,Remote Address,Remote Port,State,UID,TX Queue,RX Queue\n";

    auto exportEntry = [&](const IPView::Packet::ConnectionEntry &e, const QString &proto) {
        out << proto << ","
            << e.localAddress << "," << e.localPort << ","
            << e.remoteAddress << "," << e.remotePort << ","
            << stateToString(e.state) << "," << e.uid << ","
            << e.txQueue << "," << e.rxQueue << "\n";
    };
    for (auto const &e : mLastSnapshot.tcpConnections) exportEntry(e, QStringLiteral("TCP"));
    for (auto const &e : mLastSnapshot.udpConnections) exportEntry(e, QStringLiteral("UDP"));

    file.close();
    IPView::UI::ErrorDialog::showInfo(this, QStringLiteral("Export Complete"),
        QStringLiteral("Connections exported to:\n%1").arg(fileName));
}

void PacketTab::onRowDoubleClicked(int row, int /*col*/)
{
    if (row < 0 || row >= mTable->rowCount()) return;

    // Find matching entry from snapshot
    auto findEntry = [&](const QString &proto) -> const IPView::Packet::ConnectionEntry* {
        auto const &list = (proto == QStringLiteral("TCP"))
            ? mLastSnapshot.tcpConnections : mLastSnapshot.udpConnections;
        for (auto const &e : list) {
            if (matchesFilter(e, proto) && matchesSearch(e, proto)) {
                // Check if this entry matches the row data
                auto *localAddrItem = mTable->item(row, 1);
                auto *remoteAddrItem = mTable->item(row, 3);
                if (localAddrItem && remoteAddrItem) {
                    if (e.localAddress == localAddrItem->text() &&
                        e.remoteAddress == remoteAddrItem->text()) {
                        return &e;
                    }
                }
            }
        }
        return nullptr;
    };

    auto *protoItem = mTable->item(row, 0);
    if (!protoItem) return;
    QString proto = protoItem->text();

    auto *entry = findEntry(proto);
    if (entry) {
        showConnectionDetail(*entry, proto);
    }
}

void PacketTab::showConnectionDetail(const IPView::Packet::ConnectionEntry &entry, const QString &proto)
{
    auto *dialog = new QDialog(this);
    dialog->setWindowTitle(QStringLiteral("Connection Details"));
    dialog->setMinimumWidth(420);
    dialog->setStyleSheet(QStringLiteral(
        "QDialog { background: %1; color: %2; }"
        "QLabel { color: %2; font-size: 13px; }"
        "QLabel[title=\"true\"] { color: %3; font-size: 16px; font-weight: bold; }"
        "QLabel[value=\"true\"] { color: %4; font-weight: bold; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_PRIMARY, C_SUCCESS));

    auto *mainLayout = new QVBoxLayout(dialog);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Title
    auto *titleLbl = new QLabel(QStringLiteral("Connection Details"));
    titleLbl->setProperty("title", true);
    titleLbl->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLbl);

    // Separator
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet(QStringLiteral("background-color: %1; border: none; height: 1px;").arg(C_BORDER));
    mainLayout->addWidget(sep);

    // Details
    auto *form = new QFormLayout();
    form->setSpacing(8);
    form->setContentsMargins(0, 8, 0, 8);

    auto addRow = [&](const QString &label, const QString &value) {
        auto *lbl = new QLabel(label);
        lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 12px; font-weight: bold;").arg(C_TEXT_DIM));
        auto *val = new QLabel(value);
        val->setProperty("value", true);
        val->setStyleSheet(QStringLiteral("color: %1; font-size: 13px; font-weight: bold;").arg(C_TEXT));
        form->addRow(lbl, val);
    };

    addRow(QStringLiteral("Protocol:"), proto);
    addRow(QStringLiteral("Local Address:"), entry.localAddress);
    addRow(QStringLiteral("Local Port:"), QString::number(entry.localPort));
    addRow(QStringLiteral("Remote Address:"), entry.remoteAddress);
    addRow(QStringLiteral("Remote Port:"), QString::number(entry.remotePort));
    addRow(QStringLiteral("State:"), stateToString(entry.state));
    addRow(QStringLiteral("UID:"), QString::number(entry.uid));
    addRow(QStringLiteral("TX Queue:"), QString::number(entry.txQueue));
    addRow(QStringLiteral("RX Queue:"), QString::number(entry.rxQueue));

    mainLayout->addLayout(form);

    // Close button
    auto *closeBtn = new QPushButton(QStringLiteral("Close"));
    closeBtn->setStyleSheet(btnAccentStyle());
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignCenter);

    dialog->exec();
    dialog->deleteLater();
}

void PacketTab::updateStats(const IPView::Packet::ConnectionSnapshot &snapshot)
{
    int tcpCount = static_cast<int>(snapshot.tcpConnections.size());
    int udpCount = static_cast<int>(snapshot.udpConnections.size());
    int established = 0;

    for (auto const &e : snapshot.tcpConnections) {
        if (e.state == IPView::Packet::ConnectionState::Established) ++established;
    }

    mTcpCountLabel->setText(QString::number(tcpCount));
    mUdpCountLabel->setText(QString::number(udpCount));
    mTotalCountLabel->setText(QString::number(tcpCount + udpCount));
    mEstablishedLabel->setText(QString::number(established));

    // Show/hide empty state
    int total = tcpCount + udpCount;
    if (total == 0) {
        showEmptyState();
    } else {
        mEmptyLabel->hide();
        mTable->show();
        mStatusMsg->setText(QStringLiteral("\u2713 %1 active connections").arg(total));
        mStatusMsg->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 11px; font-weight: bold; padding: 4px 8px; border-radius: 4px;"
        ).arg(C_SUCCESS));
        mStatusMsg->show();
    }
}

void PacketTab::showEmptyState()
{
    mTable->hide();
    mEmptyLabel->show();
    mEmptyLabel->setText(
        QStringLiteral("\u23F3 No active connections found\n"
                       "Connections will appear here when detected.\n"
                       "Click Refresh to scan now."));
    mStatusMsg->setText(QStringLiteral("\u26A0 No connections detected"));
    mStatusMsg->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 11px; font-weight: bold; padding: 4px 8px; border-radius: 4px;"
    ).arg(C_WARNING));
    mStatusMsg->show();
}

void PacketTab::populateTable(const IPView::Packet::ConnectionSnapshot &snapshot)
{
    mTable->setRowCount(0);

    int visibleRows = 0;

    auto addRow = [&](const IPView::Packet::ConnectionEntry &e, const QString &proto) {
        if (!matchesFilter(e, proto)) return;
        if (!matchesSearch(e, proto)) return;

        int row = mTable->rowCount();
        mTable->insertRow(row);
        ++visibleRows;

        // Protocol
        auto *protoItem = new QTableWidgetItem(proto);
        protoItem->setForeground(QBrush(QColor(proto == QStringLiteral("TCP") ? C_SUCCESS : C_PRIMARY)));
        protoItem->setTextAlignment(Qt::AlignCenter);
        protoItem->setFont(QFont(QStringLiteral("Segoe UI"), 11, QFont::Bold));
        mTable->setItem(row, 0, protoItem);

        // Local Address
        auto *localAddrItem = new QTableWidgetItem(e.localAddress);
        localAddrItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(row, 1, localAddrItem);

        // Local Port
        auto *localPortItem = new QTableWidgetItem(QString::number(e.localPort));
        localPortItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(row, 2, localPortItem);

        // Remote Address
        auto *remoteAddrItem = new QTableWidgetItem(e.remoteAddress);
        remoteAddrItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(row, 3, remoteAddrItem);

        // Remote Port
        auto *remotePortItem = new QTableWidgetItem(QString::number(e.remotePort));
        remotePortItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(row, 4, remotePortItem);

        // State (color-coded with icon)
        QString stateStr = stateToString(e.state);
        QString icon = stateIcon(e.state);
        auto *stateItem = new QTableWidgetItem(icon + QStringLiteral("  ") + stateStr);
        stateItem->setForeground(QBrush(QColor(stateColor(e.state))));
        stateItem->setTextAlignment(Qt::AlignCenter);
        stateItem->setFont(QFont(QStringLiteral("Segoe UI"), 11, QFont::Bold));
        mTable->setItem(row, 5, stateItem);

        // UID
        auto *uidItem = new QTableWidgetItem(QString::number(e.uid));
        uidItem->setTextAlignment(Qt::AlignCenter);
        mTable->setItem(row, 6, uidItem);

        // Tooltip with full details
        QString tip;
        tip += QStringLiteral("Protocol: %1\n").arg(proto);
        tip += QStringLiteral("Local: %1:%2\n").arg(e.localAddress).arg(e.localPort);
        tip += QStringLiteral("Remote: %1:%2\n").arg(e.remoteAddress).arg(e.remotePort);
        tip += QStringLiteral("State: %1\n").arg(stateStr);
        tip += QStringLiteral("UID: %1\n").arg(e.uid);
        tip += QStringLiteral("TX Queue: %1\n").arg(e.txQueue);
        tip += QStringLiteral("RX Queue: %1").arg(e.rxQueue);
        for (int col = 0; col < 7; ++col) {
            if (auto *item = mTable->item(row, col)) {
                item->setToolTip(tip);
            }
        }
    };

    for (auto const &e : snapshot.tcpConnections) addRow(e, QStringLiteral("TCP"));
    for (auto const &e : snapshot.udpConnections) addRow(e, QStringLiteral("UDP"));

    mTable->resizeColumnsToContents();

    // Update empty state visibility
    if (visibleRows == 0 && (static_cast<int>(snapshot.tcpConnections.size()) +
                              static_cast<int>(snapshot.udpConnections.size())) > 0) {
        // Has connections but none match filter
        mTable->show();
        mEmptyLabel->hide();
        mStatusMsg->setText(QStringLiteral("\u26A0 No connections match current filters"));
        mStatusMsg->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 11px; font-weight: bold; padding: 4px 8px; border-radius: 4px;"
        ).arg(C_WARNING));
        mStatusMsg->show();
    }
}

bool PacketTab::matchesFilter(const IPView::Packet::ConnectionEntry &e,
                               const QString &proto) const
{
    // Protocol filter
    QString const protoFilter = mProtoFilter->currentText();
    if (protoFilter != QStringLiteral("All") && proto != protoFilter) return false;

    // State filter
    QString const stateFilter = mStateFilter->currentText();
    if (stateFilter != QStringLiteral("All")) {
        QString const stateStr = stateToString(e.state);
        if (stateStr != stateFilter) return false;
    }

    return true;
}

bool PacketTab::matchesSearch(const IPView::Packet::ConnectionEntry &e,
                               const QString &proto) const
{
    QString const search = mSearchEdit->text().trimmed();
    if (search.isEmpty()) return true;

    QString const searchable = QStringLiteral("%1 %2 %3 %4 %5 %6 %7")
        .arg(proto)
        .arg(e.localAddress)
        .arg(e.localPort)
        .arg(e.remoteAddress)
        .arg(e.remotePort)
        .arg(stateToString(e.state))
        .arg(e.uid);

    return searchable.contains(search, Qt::CaseInsensitive);
}

QString PacketTab::stateToString(IPView::Packet::ConnectionState state) noexcept
{
    switch (state) {
        case IPView::Packet::ConnectionState::Established:   return QStringLiteral("ESTABLISHED");
        case IPView::Packet::ConnectionState::SynSent:       return QStringLiteral("SYN_SENT");
        case IPView::Packet::ConnectionState::SynReceived:   return QStringLiteral("SYN_RECV");
        case IPView::Packet::ConnectionState::FinWait1:      return QStringLiteral("FIN_WAIT1");
        case IPView::Packet::ConnectionState::FinWait2:      return QStringLiteral("FIN_WAIT2");
        case IPView::Packet::ConnectionState::TimeWait:      return QStringLiteral("TIME_WAIT");
        case IPView::Packet::ConnectionState::Close:         return QStringLiteral("CLOSE");
        case IPView::Packet::ConnectionState::CloseWait:     return QStringLiteral("CLOSE_WAIT");
        case IPView::Packet::ConnectionState::LastAck:       return QStringLiteral("LAST_ACK");
        case IPView::Packet::ConnectionState::Listen:        return QStringLiteral("LISTEN");
        case IPView::Packet::ConnectionState::Closing:       return QStringLiteral("CLOSING");
        default:                                            return QStringLiteral("UNKNOWN");
    }
}

QString PacketTab::stateColor(IPView::Packet::ConnectionState state) noexcept
{
    switch (state) {
        case IPView::Packet::ConnectionState::Established:   return C_SUCCESS;  // green
        case IPView::Packet::ConnectionState::Listen:        return C_PRIMARY;  // blue
        case IPView::Packet::ConnectionState::TimeWait:      return C_WARNING;  // orange
        case IPView::Packet::ConnectionState::CloseWait:     return C_WARNING;
        case IPView::Packet::ConnectionState::SynSent:       return C_INFO;     // cyan
        case IPView::Packet::ConnectionState::SynReceived:   return C_INFO;
        case IPView::Packet::ConnectionState::FinWait1:      return C_TEXT_DIM; // gray
        case IPView::Packet::ConnectionState::FinWait2:      return C_TEXT_DIM;
        case IPView::Packet::ConnectionState::Close:         return C_TEXT_DIM;
        case IPView::Packet::ConnectionState::LastAck:       return C_TEXT_DIM;
        case IPView::Packet::ConnectionState::Closing:       return C_TEXT_DIM;
        default:                                            return C_TEXT_MUTED;
    }
}

QString PacketTab::stateIcon(IPView::Packet::ConnectionState state) noexcept
{
    switch (state) {
        case IPView::Packet::ConnectionState::Established:   return QStringLiteral("\u25CF");
        case IPView::Packet::ConnectionState::Listen:        return QStringLiteral("\u25CF");
        case IPView::Packet::ConnectionState::TimeWait:      return QStringLiteral("\u25CB");
        case IPView::Packet::ConnectionState::CloseWait:     return QStringLiteral("\u25CB");
        case IPView::Packet::ConnectionState::SynSent:       return QStringLiteral("\u25CC");
        case IPView::Packet::ConnectionState::SynReceived:   return QStringLiteral("\u25CC");
        case IPView::Packet::ConnectionState::FinWait1:      return QStringLiteral("\u25CC");
        case IPView::Packet::ConnectionState::FinWait2:      return QStringLiteral("\u25CC");
        case IPView::Packet::ConnectionState::Close:         return QStringLiteral("\u25CB");
        case IPView::Packet::ConnectionState::LastAck:       return QStringLiteral("\u25CB");
        case IPView::Packet::ConnectionState::Closing:       return QStringLiteral("\u25CB");
        default:                                            return QStringLiteral("\u25CB");
    }
}

} // namespace IPView::UI
