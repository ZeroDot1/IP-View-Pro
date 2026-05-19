// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — ScannerTab.h
//  C++26: [[nodiscard]], noexcept, const-correctness
//  GUI für den asynchronen Port-Scanner (ScannerModule).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef SCANNERTAB_H
#define SCANNERTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QProgressBar>
#include <QSet>

#include "ScannerModule.h"

// ═══════════════════════════════════════════════════════════════════════════════
class ScannerTab : public QWidget
{
    Q_OBJECT

public:
    explicit ScannerTab(QWidget *parent = nullptr);
    ~ScannerTab() override = default;

private slots:
    void onStartScan();
    void onCancelScan();
    void onPortFound(const IPView::Scanner::ScanResult &result);
    void onScanCompleted(const QVector<IPView::Scanner::ScanResult> &results);
    void onScanProgress(int current, int total);
    void onScanError(const QString &message);
    void onScanCancelled();
    void onSelectionChanged();

private:
    void setupUI() noexcept;
    void setScanningState(bool scanning) noexcept;
    [[nodiscard]] QVector<int> collectPorts() const noexcept;

    // ── UI ─────────────────────────────────────────────────────────────────
    QLineEdit    *targetEdit{nullptr};
    QPushButton  *scanButton{nullptr};
    QPushButton  *cancelButton{nullptr};
    QCheckBox    *quickScanCheck{nullptr};
    QSpinBox     *portFromSpin{nullptr};
    QSpinBox     *portToSpin{nullptr};
    QTableWidget *resultTable{nullptr};
    QLabel       *statusLabel{nullptr};
    QProgressBar *progressBar{nullptr};

    // ── Scanner ────────────────────────────────────────────────────────────
    IPView::Scanner::ScannerModule *mScanner{nullptr};
    int mTotalPorts{0};
};

#endif // SCANNERTAB_H
