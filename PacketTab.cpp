// ═══════════════════════════════════════════════════════════════════════════════
// IPView Pro v2.12.0 — PacketTab.cpp
// C++26: structured bindings, noexcept, [[nodiscard]], std::ranges
// Professional UI displaying live connections from PacketModule.
// Features: protocol/state filtering, search, statistics, export, theme.
// Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "PacketTab.h"
#include "Theme.h"
#include "SecurityUtil.h"

#include <QHeaderView>
#include <QTimer>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <ranges>
#include <algorithm>

namespace IPView::UI {

PacketTab::PacketTab(IPView::Packet::PacketModule *module, QWidget *parent) noexcept
    : QWidget(parent), mModule(module)
{
    setupUI();

    // Auto-refresh every 5 seconds
    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(5000);
    connect(mRefreshTimer, &QTimer::timeout, this, &PacketTab::refreshData);
    if (mAutoRefresh) mRefreshTimer->start();

    // Initial data load
    refreshData();
}

void PacketTab::setupUI() noexcept
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // ── Title bar ───────────────────────────────────────────────────────
    auto *titleBar = new QHBoxLayout();
    auto *title = new QLabel(QStringLiteral("Active Network Connections"));
    title->setStyleSheet(QStringLiteral("color: %1; font-size: 16px; font-weight: bold;").arg(C_PRIMARY));
    titleBar->addWidget(title);
    titleBar->addStretch();

    mLastUpdateLabel = new QLabel(QStringLiteral("Last update: --"));
    mLastUpdateLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 10px;").arg(C_TEXT_DIM));
    titleBar->addWidget(mLastUpdateLabel);
    mainLayout->addLayout(titleBar);

    // ── Control bar ─────────────────────────────────────────────────────
    auto *controlBar = new QHBoxLayout();
    controlBar->setSpacing(8);

    // Protocol filter
    controlBar->addWidget(new QLabel(QStringLiteral("Protocol:")));
    mProtoFilter = new QComboBox();
    mProtoFilter->addItems({QStringLiteral("All"), QStringLiteral("TCP"), QStringLiteral("UDP")});
    mProtoFilter->setFixedWidth(100);
    mProtoFilter->setStyleSheet(comboStyle());
    controlBar->addWidget(mProtoFilter);

    // State filter
    controlBar->addWidget(new QLabel(QStringLiteral("State:")));
    mStateFilter = new QComboBox();
    mStateFilter->addItems({
        QStringLiteral("All"), QStringLiteral("ESTABLISHED"), QStringLiteral("LISTEN"),
        QStringLiteral("TIME_WAIT"), QStringLiteral("CLOSE_WAIT"), QStringLiteral("SYN_SENT"),
        QStringLiteral("SYN_RECV"), QStringLiteral("FIN_WAIT1"), QStringLiteral("FIN_WAIT2"),
        QStringLiteral("CLOSE"), QStringLiteral("LAST_ACK"), QStringLiteral("CLOSING"),
        QStringLiteral("UNKNOWN")
    });
    mStateFilter->setFixedWidth(140);
    mStateFilter->setStyleSheet(comboStyle());
    controlBar->addWidget(mStateFilter);

    // Search
    controlBar->addWidget(new QLabel(QStringLiteral("Search:")));
    mSearchEdit = new QLineEdit();
    mSearchEdit->setPlaceholderText(QStringLiteral("IP, port, UID..."));
    mSearchEdit->setFixedWidth(160);
    mSearchEdit->setStyleSheet(inputStyle());
    controlBar->addWidget(mSearchEdit);

    controlBar->addStretch();

    // Action buttons
    mRefreshBtn = new QPushButton(QStringLiteral("\u27F3 Refresh"));
    mRefreshBtn->setStyleSheet(btnSecondaryStyle());
    mRefreshBtn->setFixedWidth(90);
    controlBar->addWidget(mRefreshBtn);

    mAutoRefreshBtn = new QPushButton(QStringLiteral("\u23F8 Auto: ON"));
    mAutoRefreshBtn->setStyleSheet(btnSmallStyle());
    mAutoRefreshBtn->setFixedWidth(100);
    controlBar->addWidget(mAutoRefreshBtn);

    mExportBtn = new QPushButton(QStringLiteral("\u2B07 Export"));
    mExportBtn->setStyleSheet(btnSmallStyle());
    mExportBtn->setFixedWidth(90);
    controlBar->addWidget(mExportBtn);

    mainLayout->addLayout(controlBar);

    // ── Statistics bar ──────────────────────────────────────────────────
    auto *statsBar = new QHBoxLayout();
    statsBar->setSpacing(16);

    auto createStatLabel = [&](const QString &labelText, QLabel *&valueLabel,
                                const QString &color) -> QFrame* {
        auto *card = new QFrame();
        card->setProperty("card", true);
        card->setStyleSheet(cardStyle());
        auto *layout = new QVBoxLayout(card);
        layout->setContentsMargins(12, 8, 12, 8);
        layout->setSpacing(2);

        auto *lbl = new QLabel(labelText);
        lbl->setStyleSheet(QStringLiteral("color: %1; font-size: 10px;").arg(C_TEXT_DIM));
        valueLabel = new QLabel(QStringLiteral("0"));
        valueLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 16px; font-weight: bold;").arg(color));
        valueLabel->setAlignment(Qt::AlignCenter);

        layout->addWidget(lbl, 0, Qt::AlignCenter);
        layout->addWidget(valueLabel, 0, Qt::AlignCenter);
        return card;
    };

    statsBar->addWidget(createStatLabel(QStringLiteral("TCP"), mTcpCountLabel, C_SUCCESS), 1);
    statsBar->addWidget(createStatLabel(QStringLiteral("UDP"), mUdpCountLabel, C_PRIMARY), 1);
    statsBar->addWidget(createStatLabel(QStringLiteral("Total"), mTotalCountLabel, C_TEXT), 1);
    statsBar->addWidget(createStatLabel(QStringLiteral("Established"), mEstablishedLabel, C_INFO), 1);

    mainLayout->addLayout(statsBar);

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
    mTable->setEditTriggers(QTableWidget::NoEditTriggers);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setAlternatingRowColors(true);
    mTable->verticalHeader()->setVisible(false);
    mTable->setStyleSheet(QStringLiteral(
        "QTableWidget { background: %1; color: %2; gridline-color: %3; "
        "border: 1px solid %3; border-radius: %4; }"
        "QTableWidget::item { padding: 4px 8px; }"
        "QTableWidget::item:selected { background: %5; color: %2; }"
        "QHeaderView::section { background: %1; color: %6; padding: 6px; "
        "border: none; border-bottom: 1px solid %3; font-weight: bold; font-size: 11px; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_SM, C_ACCENT, C_TEXT_DIM));

    mainLayout->addWidget(mTable, 1);

    // ── Connections ─────────────────────────────────────────────────────
    connect(mProtoFilter,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PacketTab::onFilterChanged);
    connect(mStateFilter,   QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PacketTab::onFilterChanged);
    connect(mSearchEdit,    &QLineEdit::textChanged,
            this, &PacketTab::onSearchChanged);
    connect(mRefreshBtn,    &QPushButton::clicked,
            this, &PacketTab::refreshData);
    connect(mAutoRefreshBtn, &QPushButton::clicked,
            this, &PacketTab::onToggleAutoRefresh);
    connect(mExportBtn,     &QPushButton::clicked,
            this, &PacketTab::onExportClicked);
}

void PacketTab::refreshData() noexcept
{
    if (!mModule) return;
    auto snapshot = mModule->pollNow();
    updateStats(snapshot);
    populateTable(snapshot);
    mLastUpdateLabel->setText(QStringLiteral("Last update: %1").arg(
        QTime::currentTime().toString(QStringLiteral("hh:mm:ss"))));
}

void PacketTab::onFilterChanged()
{
    refreshData();
}

void PacketTab::onSearchChanged()
{
    refreshData();
}

void PacketTab::onToggleAutoRefresh()
{
    mAutoRefresh = !mAutoRefresh;
    if (mAutoRefresh) {
        mRefreshTimer->start();
        mAutoRefreshBtn->setText(QStringLiteral("\u23F8 Auto: ON"));
    } else {
        mRefreshTimer->stop();
        mAutoRefreshBtn->setText(QStringLiteral("\u25B6 Auto: OFF"));
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
        QMessageBox::warning(this, QStringLiteral("Export Error"),
            QStringLiteral("Could not open file for writing:\n%1").arg(file.errorString()));
        return;
    }

    QTextStream out(&file);
    out << "Protocol,Local Address,Local Port,Remote Address,Remote Port,State,UID\n";

    if (mModule) {
        auto snapshot = mModule->pollNow();
        auto exportEntry = [&](const IPView::Packet::ConnectionEntry &e, const QString &proto) {
            out << proto << ","
                << e.localAddress << "," << e.localPort << ","
                << e.remoteAddress << "," << e.remotePort << ","
                << stateToString(e.state) << "," << e.uid << "\n";
        };
        for (auto const &e : snapshot.tcpConnections) exportEntry(e, QStringLiteral("TCP"));
        for (auto const &e : snapshot.udpConnections) exportEntry(e, QStringLiteral("UDP"));
    }

    file.close();
    QMessageBox::information(this, QStringLiteral("Export Complete"),
        QStringLiteral("Connections exported to:\n%1").arg(fileName));
}

void PacketTab::updateStats(const IPView::Packet::ConnectionSnapshot &snapshot) noexcept
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
}

void PacketTab::populateTable(const IPView::Packet::ConnectionSnapshot &snapshot) noexcept
{
    mTable->setRowCount(0);

    auto addRow = [&](const IPView::Packet::ConnectionEntry &e, const QString &proto) {
        if (!matchesFilter(e, proto)) return;
        if (!matchesSearch(e, proto)) return;

        int row = mTable->rowCount();
        mTable->insertRow(row);

        // Protocol
        auto *protoItem = new QTableWidgetItem(proto);
        protoItem->setForeground(QBrush(QColor(proto == QStringLiteral("TCP") ? C_SUCCESS : C_PRIMARY)));
        protoItem->setTextAlignment(Qt::AlignCenter);
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

        // State (color-coded)
        QString stateStr = stateToString(e.state);
        auto *stateItem = new QTableWidgetItem(stateStr);
        stateItem->setForeground(QBrush(QColor(stateColor(e.state))));
        stateItem->setTextAlignment(Qt::AlignCenter);
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
        tip += QStringLiteral("UID: %1").arg(e.uid);
        for (int col = 0; col < 7; ++col) {
            if (auto *item = mTable->item(row, col)) {
                item->setToolTip(tip);
            }
        }
    };

    for (auto const &e : snapshot.tcpConnections) addRow(e, QStringLiteral("TCP"));
    for (auto const &e : snapshot.udpConnections) addRow(e, QStringLiteral("UDP"));

    mTable->resizeColumnsToContents();
}

bool PacketTab::matchesFilter(const IPView::Packet::ConnectionEntry &e,
                               const QString &proto) const noexcept
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
                               const QString &proto) const noexcept
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

} // namespace IPView::UI
