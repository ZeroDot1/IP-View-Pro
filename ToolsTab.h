// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.3 — ToolsTab.h
//  C++26: default member init, [[nodiscard]], std::array
//  Ping, iPerf3, Traceroute, and the local-network device discovery
//  sub-tab. The discovery sub-tab uses IPView::Scanner::NetworkDiscovery
//  to ping-sweep a /24 subnet, look up MACs in /proc/net/arp, resolve
//  hostnames via reverse DNS, and identify vendors from the MAC OUI.
//  Each discovered device row carries a "Port scan" button that
//  delegates to ScannerTab::setTargetAndStart() and switches focus.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef TOOLSTAB_H
#define TOOLSTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QTabWidget>
#include <QProcess>
#include <QTableWidget>
#include <QStringList>
#include <QVector>
#include <QTimer>
#include <QProgressBar>

#include <array>
#include <cstddef>
#include <vector>

#include "ScannerModule.h"
#include "NetworkDiscovery.h"

class Iperf3Window; // Forward declaration

class ToolsTab : public QWidget
{
    Q_OBJECT

public:
    explicit ToolsTab(QWidget *parent = nullptr);

    void setTargetIp(const QString &ip) noexcept;

signals:
    // Emitted when the user clicks the "Port scan" button next
    // to a discovered device in the network-scan sub-tab. The
    // main window picks this up, switches to the Port Scanner
    // tab, and starts a quick scan.
    void portScanRequested(const QString &ip);

private slots:
    void onPingClicked();
    void onStopPingClicked();
    void onIperfClicked();

    // ── Network-scan (device discovery) slots ────────────────────────
    void onNetScanStartClicked();
    void onNetScanStopClicked();
    void onNetScanDeviceFound(const IPView::Scanner::DiscoveredDevice &device) noexcept;
    void onNetScanProgress(int scanned, int total) noexcept;
    void onNetScanCompleted() noexcept;
    void onNetScanError(const QString &message) noexcept;
    void onNetScanCancelled() noexcept;

    // The "Port scan" button in a device row calls this.
    void onPortScanButtonClicked();

private:
    // Build the network-scan sub-tab UI (separate from the
    // ping/iperf sub-tab).
    void setupNetworkScanTab(QVBoxLayout *parentLayout) noexcept;
    // Refresh the auto-detected subnet suggestion shown as
    // placeholder text. Called once on construction and on
    // every deviceFound() so the user sees the local /24
    // without having to type it.
    void refreshSubnetPlaceholder() noexcept;
    // Append a "Port scan" QPushButton to the given row and
    // connect it to onPortScanButtonClicked().
    void addPortScanButton(int row, const QString &ip) noexcept;

    // ── UI elements (Ping / iPerf3 sub-tab) ──────────────────────────────
    QLineEdit   *targetEdit{nullptr};
    QTextEdit   *outputArea{nullptr};
    QPushButton *pingButton{nullptr};
    QPushButton *stopPingButton{nullptr};
    QPushButton *iperfButton{nullptr};

    // ── UI elements (Network-scan / device discovery sub-tab) ───────────
    QLineEdit              *mNetScanSubnetEdit{nullptr};
    QPushButton            *mNetScanStartBtn{nullptr};
    QPushButton            *mNetScanStopBtn{nullptr};
    QLabel                 *mNetScanStatusLbl{nullptr};
    QLabel                 *mNetScanSubnetLbl{nullptr};
    QProgressBar           *mNetScanProgress{nullptr};
    QTableWidget           *mNetScanTable{nullptr};
    IPView::Scanner::NetworkDiscovery *mNetDiscovery{nullptr};
    int                     mNetScanFoundCount{0};

    // ── Process ──────────────────────────────────────────────────────────
    QProcess *pingProcess{nullptr};

    // ── Embedded Iperf3Window ────────────────────────────────────────────
    QTabWidget    *mToolsTabWidget{nullptr};
    Iperf3Window  *mIperfWindow{nullptr};
    int            mIperfTabIndex{-1};
};

#endif // TOOLSTAB_H
