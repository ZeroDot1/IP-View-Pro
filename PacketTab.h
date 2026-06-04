// ═══════════════════════════════════════════════════════════════════════════════
// IPView Pro v2.15.0 — PacketTab.h
// C++26: noexcept, [[nodiscard]], default member init, std::ranges
// Professional UI for live network connections from PacketModule.
// Features: filtering, search, statistics, theme integration, state colors,
//           signal-driven updates, detail dialogs, connection counting.
// Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef PACKETTAB_H
#define PACKETTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QFrame>

#include "PacketModule.h"

namespace IPView::UI {

class PacketTab : public QWidget {
    Q_OBJECT
public:
    explicit PacketTab(IPView::Packet::PacketModule *module, QWidget *parent = nullptr);
    ~PacketTab() override = default;

public slots:
    void refreshData();

private slots:
    void onSnapshotReceived(const IPView::Packet::ConnectionSnapshot &snapshot);
    void onFilterChanged();
    void onSearchChanged();
    void onExportClicked();
    void onToggleAutoRefresh();
    void onRowDoubleClicked(int row, int col);

private:
    void setupUI();
    void setupConnections();
    void populateTable(const IPView::Packet::ConnectionSnapshot &snapshot);
    void updateStats(const IPView::Packet::ConnectionSnapshot &snapshot);
    void showEmptyState();
    void showConnectionDetail(const IPView::Packet::ConnectionEntry &entry, const QString &proto);
    [[nodiscard]] static QString stateToString(IPView::Packet::ConnectionState state) noexcept;
    [[nodiscard]] static QString stateColor(IPView::Packet::ConnectionState state) noexcept;
    [[nodiscard]] static QString stateIcon(IPView::Packet::ConnectionState state) noexcept;
    [[nodiscard]] bool matchesFilter(const IPView::Packet::ConnectionEntry &e,
                                      const QString &proto) const;
    [[nodiscard]] bool matchesSearch(const IPView::Packet::ConnectionEntry &e,
                                      const QString &proto) const;

    IPView::Packet::PacketModule *mModule{nullptr};
    QTableWidget *mTable{nullptr};
    QTimer       *mRefreshTimer{nullptr};

    // Filter controls
    QFrame     *mFilterBar{nullptr};
    QComboBox  *mProtoFilter{nullptr};
    QComboBox  *mStateFilter{nullptr};
    QLineEdit  *mSearchEdit{nullptr};

    // Action buttons
    QPushButton  *mRefreshBtn{nullptr};
    QPushButton  *mAutoRefreshBtn{nullptr};
    QPushButton  *mExportBtn{nullptr};

    // Statistics labels
    QLabel *mTcpCountLabel{nullptr};
    QLabel *mUdpCountLabel{nullptr};
    QLabel *mTotalCountLabel{nullptr};
    QLabel *mEstablishedLabel{nullptr};
    QLabel *mLastUpdateLabel{nullptr};
    QLabel *mStatusMsg{nullptr};

    // Empty state label
    QLabel *mEmptyLabel{nullptr};

    bool mAutoRefresh{true};
    IPView::Packet::ConnectionSnapshot mLastSnapshot;
};

} // namespace IPView::UI

#endif // PACKETTAB_H
