// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — HistoryTab.h
//  C++26: default member init, structured bindings
//  Records IP changes during the session (max. 50 entries).
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef HISTORYTAB_H
#define HISTORYTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QJsonObject>
#include <QList>
#include <QPushButton>
#include <QDateTime>

#include <utility>  // std::pair

class HistoryTab : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryTab(QWidget *parent = nullptr);

    void updateHistory(const QList<QJsonObject> &history);

private slots:
    void onClearHistory();

private:
    // ── UI ─────────────────────────────────────────────────────────────────
    QTextEdit   *historyArea{nullptr};
    QPushButton *clearButton{nullptr};

    // ── Data ──────────────────────────────────────────────────────────────
    QList<QPair<QDateTime, QJsonObject>> historyWithTime;
};

#endif // HISTORYTAB_H
