// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — AuditorTab.cpp
//  C++26: QStringLiteral, auto, structured bindings
//  TLS-Auditor Tab — GUI for certificate auditing (Item 48).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "AuditorTab.h"
#include "Theme.h"

#include <QHeaderView>
#include <QDateTime>

// ═══════════════════════════════════════════════════════════════════════════════
AuditorTab::AuditorTab(QWidget *parent)
    : QWidget(parent)
{
    mAuditor = new IPView::Auditor::AuditorModule(this);
    setupUI();

    connect(mAuditor, &IPView::Auditor::AuditorModule::batchProgress,
            this, &AuditorTab::onBatchProgress);
}

// ═══════════════════════════════════════════════════════════════════════════════
void AuditorTab::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // ── Top: Host input ─────────────────────────────────────────────────
    auto *inputRow = new QHBoxLayout();

    auto *hostLabel = new QLabel(QStringLiteral("Host:"));
    hostInput = new QLineEdit();
    hostInput->setPlaceholderText(QStringLiteral("example.com or multiple lines (batch)"));
    hostInput->setStyleSheet(inputStyle());

    auditButton = new QPushButton(QStringLiteral("🔍 Audit"));
    auditButton->setProperty("accent", true);
    auditButton->setStyleSheet(btnAccentStyle());

    inputRow->addWidget(hostLabel);
    inputRow->addWidget(hostInput, 1);
    inputRow->addWidget(auditButton);

    mainLayout->addLayout(inputRow);

    // ── Progress-Bar ─────────────────────────────────────────────────────
    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setVisible(false);
    progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { border: 1px solid %1; border-radius: %2; "
        "text-align: center; background: %3; height: 20px; }"
        "QProgressBar::chunk { background-color: %4; border-radius: %5; }"
    ).arg(C_BORDER, RADIUS_SM, C_BG_ELEVATED, C_ACCENT, RADIUS_SM));
    mainLayout->addWidget(progressBar);

    // ── Splitter: Tabelle oben, Details unten ────────────────────────────
    splitter = new QSplitter(Qt::Vertical);

    // ── Result table ───────────────────────────────────────────────────
    resultTable = new QTableWidget(0, 6);
    resultTable->setHorizontalHeaderLabels({
        QStringLiteral("Host"),
        QStringLiteral("Port"),
        QStringLiteral("Status"),
        QStringLiteral("Issued To"),
        QStringLiteral("Aussteller"),
        QStringLiteral("Days Until Expiry")
    });
    resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultTable->setSelectionMode(QAbstractItemView::SingleSelection);
    resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultTable->horizontalHeader()->setStretchLastSection(true);
    resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    resultTable->verticalHeader()->setVisible(false);
    resultTable->setAlternatingRowColors(true);
    resultTable->setStyleSheet(QStringLiteral(
        "QTableWidget { background: %1; color: %2; gridline-color: %3; "
        "border-radius: %4; }"
        "QTableWidget::item { padding: 4px; }"
        "QHeaderView::section { background: %3; color: %2; padding: 6px; "
        "border: none; font-weight: bold; }"
    ).arg(C_BG_ELEVATED, C_TEXT, C_BORDER, RADIUS_SM));

    connect(resultTable, &QTableWidget::itemSelectionChanged,
            this, &AuditorTab::onHostSelectionChanged);

    splitter->addWidget(resultTable);

    // ── Detail view ─────────────────────────────────────────────────────
    detailView = new QTextEdit();
    detailView->setReadOnly(true);
    detailView->setStyleSheet(monoStyle());
    detailView->setPlaceholderText(QStringLiteral(
        "Select a host in the table above to view certificate details..."
    ));
    splitter->addWidget(detailView);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    // ── Verbindungen ─────────────────────────────────────────────────────
    connect(auditButton, &QPushButton::clicked, this, &AuditorTab::onAuditClicked);
    connect(hostInput, &QLineEdit::returnPressed, this, &AuditorTab::onAuditClicked);
}

// ═══════════════════════════════════════════════════════════════════════════════
void AuditorTab::onAuditClicked()
{
    QString const input = hostInput->text().trimmed();
    if (input.isEmpty()) return;

    auditButton->setEnabled(false);
    resultTable->setRowCount(0);
    detailView->clear();

    // Check if multiple lines (batch mode)
    QStringList const lines = input.split(QLatin1Char('\n'), Qt::SkipEmptyParts);

    if (lines.size() > 1) {
        // ── Batch mode ──────────────────────────────────────────────────
        progressBar->setVisible(true);
        progressBar->setValue(0);

        auto const results = mAuditor->auditHostList(input);
        for (auto const &r : results) {
            addResultToTable(r);
        }

        progressBar->setValue(progressBar->maximum());
    } else {
        // ── Single host ──────────────────────────────────────────────────
        progressBar->setVisible(false);

        int port = 443;
        QString host = input;
        int const colonPos = static_cast<int>(input.lastIndexOf(QLatin1Char(':')));
        if (colonPos > 0) {
            bool ok = false;
            int const p = input.mid(colonPos + 1).toInt(&ok);
            if (ok && p > 0 && p < 65536) {
                port = p;
                host = input.left(colonPos);
            }
        }

        auto result = mAuditor->auditHost(host, port);
        if (result.has_value()) {
            addResultToTable(*result);
            showCertificateDetails(*result);
        }
    }

    auditButton->setEnabled(true);
    resultTable->resizeColumnsToContents();
}

// ═══════════════════════════════════════════════════════════════════════════════
void AuditorTab::onHostSelectionChanged()
{
    int const row = resultTable->currentRow();
    if (row < 0) return;

    // Reconstruct data from table for detail view
    QString const host = resultTable->item(row, 0)->text();
    detailView->setHtml(QStringLiteral(
        "<h3>%1</h3><p>Details: see certificate chain in table.</p>"
        "<p><i>Full chain inspection requires re-audit of the host.</i></p>"
    ).arg(host));
}

// ═══════════════════════════════════════════════════════════════════════════════
void AuditorTab::onBatchProgress(int current, int total)
{
    progressBar->setMaximum(total);
    progressBar->setValue(current);
}

// ═══════════════════════════════════════════════════════════════════════════════
void AuditorTab::addResultToTable(const IPView::Auditor::AuditResult &result)
{
    int const row = resultTable->rowCount();
    resultTable->insertRow(row);

    auto *hostItem = new QTableWidgetItem(result.host);
    hostItem->setToolTip(result.host);

    auto *portItem = new QTableWidgetItem(QString::number(result.port));

    QString const statusText = result.isSecure
        ? QStringLiteral("✅ Secure")
        : QStringLiteral("Insecure");
    auto *statusItem = new QTableWidgetItem(statusText);
    statusItem->setForeground(result.isSecure ? QColor(0x4c, 0xaf, 0x50)
                                              : QColor(0xf4, 0x43, 0x36));

    QString subject;
    if (!result.chain.empty()) {
        subject = result.chain.front().subject;
    }
    auto *subjectItem = new QTableWidgetItem(subject);

    QString issuer;
    if (!result.chain.empty()) {
        issuer = result.chain.front().issuer;
    }
    auto *issuerItem = new QTableWidgetItem(issuer);

    auto *daysItem = new QTableWidgetItem(
        result.daysRemaining > 0
            ? QStringLiteral("%1 days").arg(result.daysRemaining)
            : QStringLiteral("Expired"));
    daysItem->setForeground(result.daysRemaining < 30
                            ? QColor(0xff, 0x98, 0x00)
                            : QColor(0x4c, 0xaf, 0x50));

    resultTable->setItem(row, 0, hostItem);
    resultTable->setItem(row, 1, portItem);
    resultTable->setItem(row, 2, statusItem);
    resultTable->setItem(row, 3, subjectItem);
    resultTable->setItem(row, 4, issuerItem);
    resultTable->setItem(row, 5, daysItem);
}

// ═══════════════════════════════════════════════════════════════════════════════
void AuditorTab::showCertificateDetails(const IPView::Auditor::AuditResult &result)
{
    QString html;
    html += QStringLiteral("<h3>TLS Audit: %1:%2</h3>").arg(result.host).arg(result.port);
    html += QStringLiteral("<p><b>Latency:</b> %1 ms</p>").arg(result.latencyMs);
    html += QStringLiteral("<p><b>Overall:</b> %1</p>")
                .arg(result.isSecure ? QStringLiteral("✅ Secure") : QStringLiteral("Insecure"));

    if (result.daysRemaining > 0) {
        html += QStringLiteral("<p><b>Days until expiry:</b> %1</p>").arg(result.daysRemaining);
    } else {
        html += QStringLiteral("<p style='color:red'><b>⚠️ Certificate expired!</b></p>");
    }

    html += QStringLiteral("<hr><h4>Certificate Chain (%1 certs)</h4>").arg(result.chain.size());

    int idx = 1;
    for (auto const &ci : result.chain) {
        html += QStringLiteral("<hr><h5>Certificate #%1</h5>").arg(idx++);
        html += QStringLiteral("<p><b>Subject:</b> %1</p>").arg(ci.subject);
        html += QStringLiteral("<p><b>Issuer:</b> %1</p>").arg(ci.issuer);
        html += QStringLiteral("<p><b>Valid from:</b> %1</p>")
                    .arg(ci.validFrom.isValid() ? ci.validFrom.toString(Qt::ISODate) : QStringLiteral("N/A"));
        html += QStringLiteral("<p><b>Valid to:</b> %1</p>")
                    .arg(ci.validTo.isValid() ? ci.validTo.toString(Qt::ISODate) : QStringLiteral("N/A"));
        html += QStringLiteral("<p><b>Self-signed:</b> %1</p>")
                    .arg(ci.isSelfSigned ? QStringLiteral("Yes ⚠️") : QStringLiteral("No"));
        html += QStringLiteral("<p><b>Expired:</b> %1</p>")
                    .arg(ci.isExpired ? QStringLiteral("Yes ❌") : QStringLiteral("No"));
        html += QStringLiteral("<p><b>Valid:</b> %1</p>")
                    .arg(ci.isValid ? QStringLiteral("✅") : QStringLiteral("❌"));

        if (!ci.subjectAltNames.isEmpty()) {
            html += QStringLiteral("<p><b>SANs:</b> %1</p>")
                        .arg(ci.subjectAltNames.join(QStringLiteral(", ")));
        }

        if (!ci.errorMessage.isEmpty()) {
            html += QStringLiteral("<p style='color:red'><b>Error:</b> %1</p>").arg(ci.errorMessage);
        }
    }

    detailView->setHtml(html);
}
