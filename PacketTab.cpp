// ═══════════════════════════════════════════════════════════════════════════════
// IPView Pro v2.10.1 — PacketTab.cpp
// C++26: structured bindings, noexcept, [[nodiscard]]
// UI component displaying live connections from PacketModule.
// Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "PacketTab.h"
#include "SecurityUtil.h" // optional security utilities
#include <QHeaderView>
#include <QTimer>

namespace IPView::UI {

PacketTab::PacketTab(IPView::Packet::PacketModule *module, QWidget *parent) noexcept
    : QWidget(parent), mModule(module)
{
    // Layout setup
    auto *layout = new QVBoxLayout(this);
    mTable = new QTableWidget(this);
    mTable->setColumnCount(7);
    mTable->setHorizontalHeaderLabels({
        QStringLiteral("Protocol"),
        QStringLiteral("Local Addr"),
        QStringLiteral("Local Port"),
        QStringLiteral("Remote Addr"),
        QStringLiteral("Remote Port"),
        QStringLiteral("State"),
        QStringLiteral("UID")
    });
    mTable->horizontalHeader()->setStretchLastSection(true);
    mTable->setEditTriggers(QTableWidget::NoEditTriggers);
    layout->addWidget(mTable);

    // Refresh timer (default 5 s)
    mRefreshTimer = new QTimer(this);
    mRefreshTimer->setInterval(5000);
    connect(mRefreshTimer, &QTimer::timeout, this, &PacketTab::refreshData);
    mRefreshTimer->start();
}

void PacketTab::refreshData()
{
    if (!mModule) return;
    auto snapshot = mModule->pollNow();
    populateTable(snapshot);
}

void PacketTab::populateTable(const IPView::Packet::ConnectionSnapshot &snapshot) noexcept
{
    // Clear existing rows
    mTable->setRowCount(0);

    auto addRow = [&](const IPView::Packet::ConnectionEntry &e, const QString &proto) {
        int row = mTable->rowCount();
        mTable->insertRow(row);
        mTable->setItem(row, 0, new QTableWidgetItem(proto));
        mTable->setItem(row, 1, new QTableWidgetItem(e.localAddress));
        mTable->setItem(row, 2, new QTableWidgetItem(QString::number(e.localPort)));
        mTable->setItem(row, 3, new QTableWidgetItem(e.remoteAddress));
        mTable->setItem(row, 4, new QTableWidgetItem(QString::number(e.remotePort)));
        // Translate enum to human‑readable string
        QString stateStr;
        switch (e.state) {
            case IPView::Packet::ConnectionState::Established:   stateStr = QStringLiteral("ESTABLISHED"); break;
            case IPView::Packet::ConnectionState::SynSent:       stateStr = QStringLiteral("SYN_SENT"); break;
            case IPView::Packet::ConnectionState::SynReceived:   stateStr = QStringLiteral("SYN_RECV"); break;
            case IPView::Packet::ConnectionState::FinWait1:      stateStr = QStringLiteral("FIN_WAIT1"); break;
            case IPView::Packet::ConnectionState::FinWait2:      stateStr = QStringLiteral("FIN_WAIT2"); break;
            case IPView::Packet::ConnectionState::TimeWait:      stateStr = QStringLiteral("TIME_WAIT"); break;
            case IPView::Packet::ConnectionState::Close:         stateStr = QStringLiteral("CLOSE"); break;
            case IPView::Packet::ConnectionState::CloseWait:     stateStr = QStringLiteral("CLOSE_WAIT"); break;
            case IPView::Packet::ConnectionState::LastAck:       stateStr = QStringLiteral("LAST_ACK"); break;
            case IPView::Packet::ConnectionState::Listen:        stateStr = QStringLiteral("LISTEN"); break;
            case IPView::Packet::ConnectionState::Closing:       stateStr = QStringLiteral("CLOSING"); break;
            default:                                            stateStr = QStringLiteral("UNKNOWN"); break;
        }
        mTable->setItem(row, 5, new QTableWidgetItem(stateStr));
        mTable->setItem(row, 6, new QTableWidgetItem(QString::number(e.uid)));
    };

    for (const auto &e : snapshot.tcpConnections) {
        addRow(e, QStringLiteral("TCP"));
    }
    for (const auto &e : snapshot.udpConnections) {
        addRow(e, QStringLiteral("UDP"));
    }
    mTable->resizeColumnsToContents();
}

} // namespace IPView::UI
