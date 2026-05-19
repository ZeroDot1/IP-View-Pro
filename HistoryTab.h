// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — HistoryTab.h
//  C++26: default member init, structured bindings
//  Records IP changes during the session (max. 50 entries).
//  Persistence via DatabaseModule (SQLite) — load on startup, save on
//  every new entry.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef HISTORYTAB_H
#define HISTORYTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QJsonObject>
#include <QList>
#include <QPushButton>
#include <QDateTime>
#include <QLabel>
#include <QCheckBox>

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
    void loadPersistedHistory() noexcept;

    // ── UI ─────────────────────────────────────────────────────────────────
    QTextEdit   *historyArea{nullptr};
    QPushButton *clearButton{nullptr};
    QCheckBox   *persistCheckBox{nullptr};

    // ── Data ──────────────────────────────────────────────────────────────
    QList<QPair<QDateTime, QJsonObject>> historyWithTime;
};

#endif // HISTORYTAB_H
