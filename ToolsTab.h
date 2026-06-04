// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — ToolsTab.h
//  C++26: default member init, [[nodiscard]], std::array
//  Ping, iPerf3, and sequential multi-target network scan tools
//  with QProcess integration.
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

#include <array>
#include <cstddef>

#include "ScannerModule.h"

class Iperf3Window; // // Forward declaration (Item 8)

class ToolsTab : public QWidget
{
    Q_OBJECT

public:
    explicit ToolsTab(QWidget *parent = nullptr);

    void setTargetIp(const QString &ip) noexcept;

private slots:
    void onPingClicked();
    void onStopPingClicked();
    void onIperfClicked();

    // ── Multi-target network scan ──────────────────
    void onNetScanStartClicked();
    void onNetScanStopClicked();
    void onNetScanPortFound(const IPView::Scanner::ScanResult &result);
    void onNetScanCompleted(const QVector<IPView::Scanner::ScanResult> &results);
    void onNetScanError(const QString &message);
    void onNetScanProgress(int current, int total);

private:
    // Parse the comma-separated target field into a list of
    // sanitised, validated IPs. Caps the result at
    // MAX_NET_SCAN_TARGETS and emits a warning into the status
    // label if the user pasted more.
    [[nodiscard]] QStringList parseNetScanTargets() const noexcept;
    // Start scanning the next entry in mNetScanQueue, or
    // finish the run if the queue is empty.
    void startNextNetScanTarget() noexcept;

    // ── UI elements (Ping / iPerf3 sub-tab) ──────────────────────────────
    QLineEdit   *targetEdit{nullptr};
    QTextEdit   *outputArea{nullptr};
    QPushButton *pingButton{nullptr};
    QPushButton *stopPingButton{nullptr};
    QPushButton *iperfButton{nullptr};

    // ── UI elements (Network-Scan sub-tab) ───────────────────────────────
    QLineEdit      *mNetScanTargetsEdit{nullptr};
    QPushButton    *mNetScanStartBtn{nullptr};
    QPushButton    *mNetScanStopBtn{nullptr};
    QLabel         *mNetScanStatusLbl{nullptr};
    QTableWidget   *mNetScanTable{nullptr};
    IPView::Scanner::ScannerModule *mNetScanner{nullptr};

    // The IPs queued for the current run, in user-entered
    // order. The vector is rebuilt on every Start click.
    QStringList mNetScanQueue;
    // The IP we are currently scanning — set in
    // startNextNetScanTarget() and read by the result
    // handlers to label the row group. Kept locally because
    // ScannerModule::mTargetIp is private.
    QString     mNetScanCurrentTarget;
    bool        mNetScanRunning{false};

    // Hard cap: the user wanted up to 3 IPs. We do not silently
    // truncate the user's input — parseNetScanTargets() reports
    // how many were dropped via the status label.
    static constexpr std::size_t MAX_NET_SCAN_TARGETS = 3;

    // ── Prozess ───────────────────────────────────────────────────────────
    QProcess *pingProcess{nullptr};

    // ── Embedded Iperf3Window (Item 8) ──────────────────────────────
    QTabWidget    *mToolsTabWidget{nullptr};
    Iperf3Window  *mIperfWindow{nullptr};
    int            mIperfTabIndex{-1};
};

#endif // TOOLSTAB_H
