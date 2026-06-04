// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — MainWindow.h
//  C++26: [[nodiscard]], default member init, structured bindings
//  Main window of the IPView application with tab interface and system tray.
//  Features: Dashboard, Whois, Tools (Ping/Traceroute/Scanner),
//  History (SQLite-persistent), Speedtest, Telemetry, About.
//  Public Domain — No License — No Restrictions.
//
//  v2.15.4: System tray is now a full "tray view" with status header,
//  refresh / show-hide / quit actions, a submenu to jump to any of
//  the 12 tabs, and a QSystemTrayIcon::message() notification on
//  every IP change. Before this revision the tray only had
//  Restore + Exit and a static tooltip, which made the tray icon
//  feel disconnected from the rest of the app.
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
#include <QPointer>
#include <QHash>

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
#include "PacketTab.h"
#include "AlertTab.h"
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
    // ── Event-based tab synchronization (Item 2) ──────────────────────────
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
    void onTrayShowHideTriggered();
    void onTraySwitchTabTriggered();

private:
    // ── UI Setup ────────────────────────────────────────────────────────
    void setupUI()  noexcept;
    void setupTray() noexcept;

    // Build the full tray menu (status header, actions, tab submenu,
    // quit). Called once in setupTray() and whenever the tab
    // registry changes (it doesn't after construction, but the
    // indirection keeps setupTray() readable).
    void buildTrayMenu() noexcept;
    // Populate the "Go to tab" submenu from the current TabRegistry.
    // Each entry carries a QWidget* payload so onTraySwitchTabTriggered
    // can switch to it without an indexOf() lookup.
    void populateTrayTabsSubmenu() noexcept;

    // Update the status header (the disabled first QAction) so the
    // user sees "IP 1.2.3.4 / 🇩🇪 Germany" at the top of the menu
    // without hovering.
    void updateTrayMenuStatus(const QJsonObject &data) noexcept;

    void updateTrayTooltip(const QJsonObject &data) noexcept;
    void updateHistory(const QJsonObject &data) noexcept;

    // Notify the OS shell of an IP change via the tray. Silently
    // no-ops if the system doesn't support notifications.
    void notifyIpChange(const QJsonObject &oldData,
                        const QJsonObject &newData) noexcept;

    // ── Per-user configuration (XDG-compliant: ~/.config/IPView/) ─────────
    void saveSettings() noexcept;
    void loadSettings() noexcept;

    // ── Tab Registry (Item 1) ─────────────────────────────────────────────
    IPView::UI::TabRegistry mTabRegistry;

    // ── Tab Widget ────────────────────────────────────────────────────────
    QTabWidget *tabWidget{nullptr};

    // ── Dashboard (Overview) ─────────────────────────────────────────────
    IPView::UI::DashboardView *dashboardView{nullptr};
    QLabel      *statusLabel{nullptr};

    // ── Tab access (convenience pointers into the registry) ─────────────
    WhoisTab     *whoisTab{nullptr};
    ToolsTab     *toolsTab{nullptr};
    ScannerTab   *scannerTab{nullptr};
    HistoryTab   *historyTab{nullptr};
    SpeedtestTab *speedtestTab{nullptr};
    TelemetryTab *telemetryTab{nullptr};
    AboutTab     *aboutTab{nullptr};
    TopologyTab  *topologyTab{nullptr};
    IPView::UI::PacketTab *packetTab{nullptr};
    IPView::UI::AlertTab  *alertTab{nullptr};

    // ── Network & Data ──────────────────────────────────────────────────
    NetworkManager *networkManager{nullptr};
    FlagLoader     *flagLoader{nullptr};

    // ─── System Tray ──────────────────────────────────────────────────────
    //  v2.15.4: full tray view. See setupTray() for the build order
    //  and buildTrayMenu() / populateTrayTabsSubmenu() for the
    //  component construction.
    QSystemTrayIcon *trayIcon{nullptr};

    // The menu and its components. All heap-allocated because the
    // QMenu may be detached (and the actions with it) when the
    // user closes the context menu — Qt deletes them automatically
    // when the QSystemTrayIcon is destroyed, but raw pointer
    // ownership is simpler than QPointer here.
    QMenu           *trayMenu{nullptr};
    QAction         *trayStatusAction{nullptr};   // Disabled, shows IP/CC
    QAction         *trayRefreshAction{nullptr}; // Refresh now
    QAction         *trayShowHideAction{nullptr};// Show / hide window
    QMenu           *trayTabsMenu{nullptr};      // Submenu: jump to tab
    QAction         *trayAutoRefreshAction{nullptr}; // Toggle 5-min timer
    QAction         *trayExitAction{nullptr};    // Quit

    // The last IP we notified about. We only fire a notification
    // when the IP actually changes, not on every refresh.
    QString          mLastNotifiedIp;
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
