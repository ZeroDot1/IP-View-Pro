// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — MainWindow.h
//  C++26: [[nodiscard]], default member init, structured bindings
//  Main window of the IPView application with tab interface and system tray.
//  Features: Dashboard, Whois, Tools (Ping/Traceroute/Scanner),
//  History (SQLite-persistent), Speedtest, Telemetry, About.
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
#include "ConfigManager.h"
#include "DashboardView.h"
#include "WhoisTab.h"
#include "ToolsTab.h"
#include "ScannerTab.h"
#include "HistoryTab.h"
#include "SpeedtestTab.h"
#include "TelemetryTab.h"
#include "AboutTab.h"
#include "AuditorTab.h"
#include "TopologyTab.h"
#include "AlertEngine.h"
#include "PacketModule.h"
#include "TabRegistry.h"
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

signals:
    // ── Event-basierte Tab-Synchronisation (Item 2) ─────────────────────
    void dataRefreshed(const QJsonObject &data);
    void historyUpdated(const QList<QJsonObject> &history);

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

    // ── Config (per-user, XDG-konform) ─────────────────────────────────
    void saveSettings() noexcept;
    void loadSettings() noexcept;

    // ── Tab Registry (Item 1) ─────────────────────────────────────────────
    IPView::UI::TabRegistry mTabRegistry;

    // ── Tab Widget ────────────────────────────────────────────────────────
    QTabWidget *tabWidget{nullptr};

    // ── Dashboard (Overview) ─────────────────────────────────────────────
    IPView::UI::DashboardView *dashboardView{nullptr};
    QLabel      *statusLabel{nullptr};

    // ── Tab-Zugriff (Convenience-Zeiger, zeigen in die Registry) ────────
    WhoisTab     *whoisTab{nullptr};
    ToolsTab     *toolsTab{nullptr};
    ScannerTab   *scannerTab{nullptr};
    HistoryTab   *historyTab{nullptr};
    SpeedtestTab *speedtestTab{nullptr};
    TelemetryTab *telemetryTab{nullptr};
    AboutTab     *aboutTab{nullptr};
    TopologyTab  *topologyTab{nullptr};

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

    // ── Packet Module (Item 47) ────────────────────────────────────────────
    IPView::Packet::PacketModule *mPacketModule{nullptr};
    void setupPacketModule() noexcept;

    // ── Alert Engine (Item 49) ─────────────────────────────────────────────
    IPView::Alert::AlertEngine *mAlertEngine{nullptr};
    void setupAlertEngine() noexcept;
};

#endif // MAINWINDOW_H
