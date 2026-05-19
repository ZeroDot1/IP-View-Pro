// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — MainWindow.h
//  C++26: [[nodiscard]], default member init, structured bindings
//  Main window der IPView-Anwendung mit Tab-Schnittstelle und System Tray.
//  Die Dashboard-Übersicht wurde in die eigenständige Klasse DashboardView
//  ausgelagert (siehe IPView::UI::DashboardView).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QTabWidget>
#include <QSystemTrayIcon>
#include <QList>
#include <QJsonObject>
#include <QTimer>
#include <QMenu>
#include <QAction>
#include <QCloseEvent>
#include <QLabel>

#include "NetworkManager.h"
#include "FlagLoader.h"
#include "DashboardView.h"
#include "WhoisTab.h"
#include "ToolsTab.h"
#include "HistoryTab.h"
#include "SpeedtestTab.h"
#include "AboutTab.h"

// ═══════════════════════════════════════════════════════════════════════════════
class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    // C++26: Default destructor (RAII-compliant due to parent-child management)
    ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onDataReceived(const QJsonObject &data);
    void onRefreshClicked();
    void onApiChanged(int index);
    void onIPv6Toggled(bool checked);
    void onAutoRefreshToggled(bool checked);
    void onCopyAllRequested();
    void onExportJsonRequested();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onExitRequested();

private:
    // ── UI Setup ────────────────────────────────────────────────────────
    void setupUI()  noexcept;
    void setupTray() noexcept;

    void updateTrayTooltip(const QJsonObject &data) noexcept;
    void updateHistory(const QJsonObject &data) noexcept;

    // ── Tab Widget ────────────────────────────────────────────────────────
    QTabWidget *tabWidget{nullptr};

    // ── Dashboard (Overview) ─────────────────────────────────────────────
    IPView::UI::DashboardView *dashboardView{nullptr};
    QLabel      *statusLabel{nullptr};

    // ── Sub-Tabs ──────────────────────────────────────────────────────────
    WhoisTab     *whoisTab{nullptr};
    ToolsTab     *toolsTab{nullptr};
    HistoryTab   *historyTab{nullptr};
    SpeedtestTab *speedtestTab{nullptr};
    AboutTab     *aboutTab{nullptr};

    // ── Network & Data ──────────────────────────────────────────────────
    NetworkManager *networkManager{nullptr};
    FlagLoader     *flagLoader{nullptr};

    // ─── System Tray ──────────────────────────────────────────────────────
    QSystemTrayIcon *trayIcon{nullptr};
    QMenu           *trayMenu{nullptr};
    QAction         *restoreAction{nullptr};
    QAction         *exitAction{nullptr};
    bool             reallyQuit{false};

    // ── Timer & History ───────────────────────────────────────────────────
    QTimer              *autoRefreshTimer{nullptr};
    QList<QJsonObject>   history;
    QJsonObject          currentData;
};

#endif // MAINWINDOW_H
