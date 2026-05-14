// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — MainWindow.h
//  C++26: [[nodiscard]], default member init, structured bindings
//  Main window of the IPView application with tab interface and system tray.
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
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QTableWidget>

#include "NetworkManager.h"
#include "FlagLoader.h"
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
    void onCopyAllClicked();
    void onExportJsonClicked();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onExitRequested();

private:
    // ── UI Setup ────────────────────────────────────────────────────────
    void setupUI()  noexcept;
    void setupTray() noexcept;

    void updateTrayTooltip(const QJsonObject &data) noexcept;
    void updateDataDisplay(const QJsonObject &data) noexcept;

    // ── Tab Widget ────────────────────────────────────────────────────────
    QTabWidget *tabWidget{nullptr};

    // ── IP Overview Tab ─────────────────────────────────────────────────
    QWidget     *overviewTab{nullptr};
    QLabel      *ipLabel{nullptr};
    QLabel      *timestampLabel{nullptr};
    QLabel      *flagLabel{nullptr};
    QLabel      *statusLabel{nullptr};
    QLabel      *onlineLabel{nullptr};

    QComboBox   *apiCombo{nullptr};
    QCheckBox   *ipv6CheckBox{nullptr};
    QCheckBox   *autoRefreshCheckBox{nullptr};
    QPushButton *refreshButton{nullptr};
    QPushButton *copyAllButton{nullptr};
    QPushButton *exportJsonButton{nullptr};
    QTableWidget *ipTable{nullptr};

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
