// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.12.0 — AlertTab.h
//  C++26: noexcept, [[nodiscard]], default member init
//  GUI tab for AlertEngine — displays active alerts with severity colors.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef ALERTTAB_H
#define ALERTTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

#include "AlertEngine.h"

namespace IPView::UI {

class AlertTab : public QWidget {
    Q_OBJECT

public:
    explicit AlertTab(IPView::Alert::AlertEngine *engine, QWidget *parent = nullptr) noexcept;
    ~AlertTab() override = default;

private slots:
    void onAlertTriggered(const IPView::Alert::Alert &alert);
    void onAlertsChanged();
    void onAcknowledgeClicked();
    void onClearClicked();
    void refreshDisplay();

private:
    void setupUI() noexcept;
    void populateTable() noexcept;
    [[nodiscard]] static QString severityIcon(IPView::Alert::Severity sev) noexcept;
    [[nodiscard]] static QString severityColor(IPView::Alert::Severity sev) noexcept;

    IPView::Alert::AlertEngine *mEngine{nullptr};
    QTableWidget *mTable{nullptr};
    QPushButton  *mAckButton{nullptr};
    QPushButton  *mClearButton{nullptr};
    QLabel       *mStatusLabel{nullptr};
    QTimer       *mRefreshTimer{nullptr};
};

} // namespace IPView::UI

#endif // ALERTTAB_H
