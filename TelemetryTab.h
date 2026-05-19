// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — TelemetryTab.h
//  C++26: [[nodiscard]], noexcept, const-correctness
//  GUI for real-time network telemetry (TelemetryModule).
//  Displays all active interfaces with live RX/TX rates.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef TELEMETRYTAB_H
#define TELEMETRYTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QTimer>

#include "TelemetryModule.h"

// ═══════════════════════════════════════════════════════════════════════════════
class TelemetryTab : public QWidget
{
    Q_OBJECT

public:
    explicit TelemetryTab(QWidget *parent = nullptr);
    ~TelemetryTab() override = default;

private slots:
    void onTelemetryUpdated(const QList<IPView::Telemetry::InterfaceInfo> &interfaces);
    void onToggleMonitoring();
    void onRefreshInterfaces();

private:
    void setupUI() noexcept;
    void updateTable(const QList<IPView::Telemetry::InterfaceInfo> &interfaces) noexcept;
    [[nodiscard]] static QString formatSpeed(double bytesPerSec) noexcept;

    // ── UI ─────────────────────────────────────────────────────────────────
    QPushButton  *toggleButton{nullptr};
    QPushButton  *refreshButton{nullptr};
    QLabel       *statusLabel{nullptr};
    QTableWidget *interfaceTable{nullptr};
    QLabel       *totalRxLabel{nullptr};
    QLabel       *totalTxLabel{nullptr};

    // ── Telemetry ──────────────────────────────────────────────────────────
    IPView::Telemetry::TelemetryModule *mTelemetry{nullptr};
    bool mMonitoring{false};
};

#endif // TELEMETRYTAB_H
