// ═══════════════════════════════════════════════════════════════════════════════
// IPView Pro v2.10.1 — PacketTab.h
// C++26: constexpr, structured bindings, [[nodiscard]]
// UI component to display live network connections from PacketModule.
// Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef PACKETTAB_H
#define PACKETTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QTimer>
#include "PacketModule.h"

namespace IPView::UI {

class PacketTab : public QWidget {
    Q_OBJECT
public:
    explicit PacketTab(IPView::Packet::PacketModule *module, QWidget *parent = nullptr) noexcept;
    ~PacketTab() override = default;

public slots:
    void refreshData();

private:
    void populateTable(const IPView::Packet::ConnectionSnapshot &snapshot) noexcept;

    IPView::Packet::PacketModule *mModule{nullptr};
    QTableWidget *mTable{nullptr};
    QTimer *mRefreshTimer{nullptr};
};

} // namespace IPView::UI

#endif // PACKETTAB_H
