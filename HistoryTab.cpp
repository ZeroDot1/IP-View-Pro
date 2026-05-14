// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — HistoryTab.cpp
//  C++26: structured bindings, auto, QStringLiteral, const-correctness
//  Displays IP changes with correct timestamps.
// ═══════════════════════════════════════════════════════════════════════════════

#include "HistoryTab.h"
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextCursor>

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
    title->setStyleSheet(QStringLiteral(
        "color: #e94560; font-size: 14px; font-weight: bold;"
    ));
    headerRow->addWidget(titleIcon);
    headerRow->addWidget(title);
    clearButton = new QPushButton(QIcon(QStringLiteral(":/svgs/wastebasket.svg")),
                                  QStringLiteral(" Clear"));
    clearButton->setStyleSheet(QStringLiteral(
        "background-color: #1a1a2e; color: #888888; border: 1px solid #333; "
        "border-radius: 4px; padding: 4px 10px; font-size: 11px;"
    ));
    headerRow->addWidget(title);
    headerRow->addStretch();
    headerRow->addWidget(clearButton);

    // ── History View ───────────────────────────────────────────────────────
    historyArea = new QTextEdit();
    historyArea->setReadOnly(true);
    historyArea->setStyleSheet(QStringLiteral(
        "background-color: #1a1a2e; color: #ffffff; border: none; "
        "border-radius: 8px; padding: 10px; font-family: monospace; font-size: 12px;"
    ));
    historyArea->setPlaceholderText(QStringLiteral("IP changes will appear here..."));

    layout->addLayout(headerRow);
    layout->addWidget(historyArea);

    connect(clearButton, &QPushButton::clicked, this, &HistoryTab::onClearHistory);
}

// ═══════════════════════════════════════════════════════════════════════════════
void HistoryTab::onClearHistory()
{
    historyWithTime.clear();
    historyArea->clear();
}

// ═══════════════════════════════════════════════════════════════════════════════
void HistoryTab::updateHistory(const QList<QJsonObject> &history)
{
    // Save current timestamp for the latest entry
    if (!history.isEmpty()) {
        QDateTime const now = QDateTime::currentDateTime();
        historyWithTime.prepend(qMakePair(now, history.first()));

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
