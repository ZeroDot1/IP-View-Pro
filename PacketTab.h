// ═══════════════════════════════════════════════════════════════════════════════
// IPView Pro v2.12.0 — PacketTab.h
// C++26: noexcept, [[nodiscard]], default member init, std::ranges
// Professional UI for live network connections from PacketModule.
// Features: filtering, search, statistics, theme integration, state colors.
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
    explicit PacketTab(IPView::Packet::PacketModule *module, QWidget *parent = nullptr) noexcept;
    ~PacketTab() override = default;

public slots:
    void refreshData() noexcept;

private slots:
    void onFilterChanged();
    void onSearchChanged();
    void onExportClicked();
    void onToggleAutoRefresh();

private:
    void setupUI() noexcept;
    void populateTable(const IPView::Packet::ConnectionSnapshot &snapshot) noexcept;
    void updateStats(const IPView::Packet::ConnectionSnapshot &snapshot) noexcept;
    [[nodiscard]] static QString stateToString(IPView::Packet::ConnectionState state) noexcept;
    [[nodiscard]] static QString stateColor(IPView::Packet::ConnectionState state) noexcept;
    [[nodiscard]] bool matchesFilter(const IPView::Packet::ConnectionEntry &e,
                                      const QString &proto) const noexcept;
    [[nodiscard]] bool matchesSearch(const IPView::Packet::ConnectionEntry &e,
                                      const QString &proto) const noexcept;

    IPView::Packet::PacketModule *mModule{nullptr};
    QTableWidget *mTable{nullptr};
    QTimer       *mRefreshTimer{nullptr};

    // Filter controls
    QComboBox    *mProtoFilter{nullptr};
    QComboBox    *mStateFilter{nullptr};
    QLineEdit    *mSearchEdit{nullptr};
    QPushButton  *mRefreshBtn{nullptr};
    QPushButton  *mAutoRefreshBtn{nullptr};
    QPushButton  *mExportBtn{nullptr};

    // Statistics labels
    QLabel *mTcpCountLabel{nullptr};
    QLabel *mUdpCountLabel{nullptr};
    QLabel *mTotalCountLabel{nullptr};
    QLabel *mEstablishedLabel{nullptr};
    QLabel *mLastUpdateLabel{nullptr};

    bool mAutoRefresh{true};
};

} // namespace IPView::UI

#endif // PACKETTAB_H
