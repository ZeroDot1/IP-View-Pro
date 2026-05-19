// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — AuditorTab.h
//  C++26: noexcept, [[nodiscard]], default member init
//  TLS-Auditor Tab — GUI for certificate auditing (Item 48).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef AUDITORTAB_H
#define AUDITORTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QProgressBar>
#include <QSplitter>
#include <QTabWidget>

#include "AuditorModule.h"

class AuditorTab : public QWidget
{
    Q_OBJECT

public:
    explicit AuditorTab(QWidget *parent = nullptr);

    // ── Module access (for AlertEngine in MainWindow) ─────────────────────
    [[nodiscard]] IPView::Auditor::AuditorModule* auditorModule() const noexcept
    {
        return mAuditor;
    }

private slots:
    void onAuditClicked();
    void onHostSelectionChanged();
    void onBatchProgress(int current, int total);

private:
    void setupUI();
    void addResultToTable(const IPView::Auditor::AuditResult &result);
    void showCertificateDetails(const IPView::Auditor::AuditResult &result);

    // ── UI elements ────────────────────────────────────────────────────────
    QLineEdit       *hostInput{nullptr};
    QPushButton     *auditButton{nullptr};
    QTableWidget    *resultTable{nullptr};
    QTextEdit       *detailView{nullptr};
    QProgressBar    *progressBar{nullptr};
    QSplitter       *splitter{nullptr};

    // ── Module ────────────────────────────────────────────────────────────
    IPView::Auditor::AuditorModule *mAuditor{nullptr};
    bool mBatchActive{false};
};

#endif // AUDITORTAB_H
