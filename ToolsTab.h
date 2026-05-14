// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — ToolsTab.h
//  C++26: default member init, [[nodiscard]]
//  Ping & iPerf3-Tools mit QProcess-Integration.
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
#include <QProcess>

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
    // ── UI-Elemente ───────────────────────────────────────────────────────
    QLineEdit   *targetEdit{nullptr};
    QTextEdit   *outputArea{nullptr};
    QPushButton *pingButton{nullptr};
    QPushButton *stopPingButton{nullptr};
    QPushButton *iperfButton{nullptr};

    // ── Prozess ───────────────────────────────────────────────────────────
    QProcess *pingProcess{nullptr};
};

#endif // TOOLSTAB_H
