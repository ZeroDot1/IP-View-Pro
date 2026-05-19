// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — HistoryTab.cpp
//  C++26: structured bindings, auto, QStringLiteral, const-correctness
//  Displays IP changes with correct timestamps.
//  New: Persistence via DatabaseModule (SQLite) – History persists across sessions
//  and is loaded on startup.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "HistoryTab.h"
#include "DatabaseModule.h"
#include "Theme.h"

#include <QDateTime>
#include <QLabel>
#include <QTextCursor>
#include <QJsonDocument>

// ═══════════════════════════════════════════════════════════════════════════════
HistoryTab::HistoryTab(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    // ── Header ────────────────────────────────────────────────────────────
    auto *headerRow = new QHBoxLayout();
    auto *titleIcon = new QLabel();
    titleIcon->setPixmap(QPixmap(QStringLiteral(":/svgs/scroll.svg"))
                             .scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto *title = new QLabel(QStringLiteral("IP Change History"));
    title->setStyleSheet(QStringLiteral("color: %1; font-size: 14px; font-weight: bold;").arg(C_ACCENT));

    // ── Info label: entries from DB ────────────────────────────────────────
    auto *countLabel = new QLabel();
    countLabel->setStyleSheet(QStringLiteral("color: %1; font-size: 11px;").arg(C_TEXT_DIM));

    // ── Persistence toggle ─────────────────────────────────────────────────
    persistCheckBox = new QCheckBox(QStringLiteral("SQLite persist"));
    persistCheckBox->setChecked(true);
    persistCheckBox->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT_SEC));

    // ── Clear Button ──────────────────────────────────────────────────────
    clearButton = new QPushButton(QIcon(QStringLiteral(":/svgs/wastebasket.svg")),
                                  QStringLiteral(" Clear"));
    clearButton->setStyleSheet(btnSmallStyle());

    headerRow->addWidget(titleIcon);
    headerRow->addWidget(title);
    headerRow->addSpacing(8);
    headerRow->addWidget(countLabel);
    headerRow->addStretch();
    headerRow->addWidget(persistCheckBox);
    headerRow->addWidget(clearButton);

    // ── History View ───────────────────────────────────────────────────────
    historyArea = new QTextEdit();
    historyArea->setReadOnly(true);
    historyArea->setStyleSheet(monoStyle());
    historyArea->setPlaceholderText(QStringLiteral("IP changes will appear here..."));

    layout->addLayout(headerRow);
    layout->addWidget(historyArea);

    connect(clearButton, &QPushButton::clicked, this, &HistoryTab::onClearHistory);

    // ── Load history from SQLite on startup ───────────────────────────
    loadPersistedHistory();
}

// ═══════════════════════════════════════════════════════════════════════════════
void HistoryTab::loadPersistedHistory() noexcept
{
    if (!IPView::Storage::DatabaseModule::isInitialized()) return;

    auto const entries = IPView::Storage::DatabaseModule::getHistory(50);
    if (entries.empty()) return;

    // Adopt saved history into historyWithTime
    QString text;
    int idx = 1;

    for (auto const &entry : entries) {
        QPair<QDateTime, QJsonObject> pair;
        pair.first = entry.timestamp;

        // JSON-Payload parsen
        QJsonDocument const doc = QJsonDocument::fromJson(entry.jsonPayload.toUtf8());
        if (doc.isObject()) {
            pair.second = doc.object();
        } else {
            // Fallback: minimale Daten aus den DB-Feldern
            QJsonObject fallback;
            fallback[QStringLiteral("ip")]           = entry.ip;
            fallback[QStringLiteral("country_name")] = entry.countryName;
            fallback[QStringLiteral("country_code")] = entry.countryCode;
            fallback[QStringLiteral("city")]         = entry.city;
            fallback[QStringLiteral("org")]           = entry.org;
            fallback[QStringLiteral("asn")]           = entry.asn;
            pair.second = fallback;
        }

        historyWithTime.append(pair);

        // Text aufbauen
        QString const ip  = entry.ip;
        QString const org = entry.org;
        QString const city = entry.city;

        text += QStringLiteral("  #%1  %2\n")
                    .arg(idx++, 2)
                    .arg(entry.timestamp.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        text += QStringLiteral("       IP:     %1\n")
                    .arg(ip.isEmpty()  ? QStringLiteral("N/A") : ip);
        text += QStringLiteral("       Org:    %1\n")
                    .arg(org.isEmpty() ? QStringLiteral("N/A") : org);
        text += QStringLiteral("       City:   %1\n")
                    .arg(city.isEmpty() ? QStringLiteral("N/A") : city);
        text += QStringLiteral("       CC:     %1\n")
                    .arg(entry.countryCode.isEmpty() ? QStringLiteral("N/A") : entry.countryCode);
        text += QStringLiteral("       ") + QString(50, QChar(0x2500)) + QStringLiteral("\n");
    }

    historyArea->setPlainText(text);
    historyArea->moveCursor(QTextCursor::Start);
}

// ═══════════════════════════════════════════════════════════════════════════════
void HistoryTab::onClearHistory()
{
    historyWithTime.clear();
    historyArea->clear();

    // Also clear SQLite if persistence is active
    if (IPView::Storage::DatabaseModule::isInitialized()) {
        IPView::Storage::DatabaseModule::clearHistory();
        IPView::Storage::DatabaseModule::vacuum();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
void HistoryTab::updateHistory(const QList<QJsonObject> &history)
{
    // Save current timestamp for the latest entry
    if (!history.isEmpty()) {
        QDateTime const now = QDateTime::currentDateTime();
        historyWithTime.prepend(qMakePair(now, history.first()));

        // Save to SQLite (if enabled)
        if (persistCheckBox->isChecked() && IPView::Storage::DatabaseModule::isInitialized()) {
            IPView::Storage::DatabaseModule::storeResult(history.first());
        }

        // Keep maximum of 50 entries
        static constexpr int MAX_ENTRIES = 50;
        while (historyWithTime.size() > MAX_ENTRIES) {
            historyWithTime.removeLast();
        }
    }

    // ── Rebuild display ────────────────────────────────────────────────────
    QString text;
    int idx = 1;

    // C++26: structured bindings for QPair decomposition
    for (auto const& [timestamp, entry] : historyWithTime) {
        QString const ip  = entry[QStringLiteral("ip")].toString();
        QString const org = entry[QStringLiteral("org")].toString();
        QString const city = entry[QStringLiteral("city")].toString();
        QString const cc  = entry[QStringLiteral("country_code")].toString();

        text += QStringLiteral("  #%1  %2\n")
                    .arg(idx++, 2)
                    .arg(timestamp.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        text += QStringLiteral("       IP:     %1\n")
                    .arg(ip.isEmpty()  ? QStringLiteral("N/A") : ip);
        text += QStringLiteral("       Org:    %1\n")
                    .arg(org.isEmpty() ? QStringLiteral("N/A") : org);
        text += QStringLiteral("       City:   %1\n")
                    .arg(city.isEmpty() ? QStringLiteral("N/A") : city);
        text += QStringLiteral("       CC:     %1\n")
                    .arg(cc.isEmpty()  ? QStringLiteral("N/A") : cc);
        // Horizontal separator line (Unicode U+2500)
        text += QStringLiteral("       ") + QString(50, QChar(0x2500)) + QStringLiteral("\n");
    }

    historyArea->setPlainText(text);

    // Scroll to the beginning
    historyArea->moveCursor(QTextCursor::Start);
}
