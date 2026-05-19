// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — TelemetryTab.h
//  C++26: [[nodiscard]], noexcept, const-correctness
//  GUI for real-time network telemetry (TelemetryModule) with
//  historical aggregation (TelemetryPersistenceModule).
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
#include <QCheckBox>

#include "TelemetryModule.h"
#include "TelemetryPersistenceModule.h"

// ═══════════════════════════════════════════════════════════════════════════════
class TelemetryTab : public QWidget
{
    Q_OBJECT

public:
    explicit TelemetryTab(QWidget *parent = nullptr);
    ~TelemetryTab() override = default;

    // ── Module access (for AlertEngine in MainWindow) ─────────────────────
    [[nodiscard]] IPView::Telemetry::TelemetryModule* telemetryModule() const noexcept
    {
        return mTelemetry;
    }

private slots:
    // ── Live monitoring ──────────────────────────────────────────────────
    void onTelemetryUpdated(const QList<IPView::Telemetry::InterfaceInfo> &interfaces);
    void onToggleMonitoring();
    void onRefreshInterfaces();

    // ── Persistence ──────────────────────────────────────────────────────
    void onTogglePersistence();
    void onShowAggregationHistory();
    void onAggregationStored(const IPView::Storage::AggregatedTelemetryEntry &record);

private:
    void setupUI() noexcept;
    void updateTable(const QList<IPView::Telemetry::InterfaceInfo> &interfaces) noexcept;
    [[nodiscard]] static QString formatSpeed(double bytesPerSec) noexcept;
    void updatePersistenceStatus() noexcept;

    // ── Live monitoring UI ───────────────────────────────────────────────
    QPushButton  *toggleButton{nullptr};
    QPushButton  *refreshButton{nullptr};
    QLabel       *statusLabel{nullptr};
    QTableWidget *interfaceTable{nullptr};
    QLabel       *totalRxLabel{nullptr};
    QLabel       *totalTxLabel{nullptr};

    // ── Persistence UI ───────────────────────────────────────────────────
    QCheckBox    *persistCheckBox{nullptr};
    QPushButton  *historyButton{nullptr};
    QLabel       *persistStatusLabel{nullptr};
    QLabel       *latestRxLabel{nullptr};
    QLabel       *latestTxLabel{nullptr};

    // ── Modules ──────────────────────────────────────────────────────────
    IPView::Telemetry::TelemetryModule              *mTelemetry{nullptr};
    IPView::Telemetry::TelemetryPersistenceModule    *mPersistence{nullptr};
    bool mMonitoring{false};
};

#endif // TELEMETRYTAB_H
