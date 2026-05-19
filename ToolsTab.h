// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — ToolsTab.h
//  C++26: default member init, [[nodiscard]]
//  Ping & iPerf3 tools with QProcess integration.
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

private:
    // ── UI elements ───────────────────────────────────────────────────────
    QLineEdit   *targetEdit{nullptr};
    QTextEdit   *outputArea{nullptr};
    QPushButton *pingButton{nullptr};
    QPushButton *stopPingButton{nullptr};
    QPushButton *iperfButton{nullptr};

    // ── Prozess ───────────────────────────────────────────────────────────
    QProcess *pingProcess{nullptr};

    // ── Embedded Iperf3Window (Item 8) ──────────────────────────────
    QTabWidget    *mToolsTabWidget{nullptr};
    Iperf3Window  *mIperfWindow{nullptr};
    int            mIperfTabIndex{-1};
};

#endif // TOOLSTAB_H
