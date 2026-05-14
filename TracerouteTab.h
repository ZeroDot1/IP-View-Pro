// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — TracerouteTab.h
//  C++26: default member init, [[nodiscard]]
//  Traceroute/tracepath/tracert execution via QProcess.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef TRACEROUTETAB_H
#define TRACEROUTETAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QProcess>

class TracerouteTab : public QWidget
{
    Q_OBJECT

public:
    explicit TracerouteTab(QWidget *parent = nullptr);

    void setTargetIp(const QString &ip) noexcept;

private slots:
    void onTraceClicked();
    void onStopTraceClicked();
    void onDataReceived();
    void onTraceFinished(int exitCode);

private:
    [[nodiscard]] QString findTraceroute() const noexcept;

    // ── UI ─────────────────────────────────────────────────────────────────
    QLineEdit   *targetEdit{nullptr};
    QTextEdit   *outputArea{nullptr};
    QPushButton *traceButton{nullptr};
    QPushButton *stopButton{nullptr};

    // ── Process ────────────────────────────────────────────────────────────
    QProcess *process{nullptr};
};

#endif // TRACEROUTETAB_H
