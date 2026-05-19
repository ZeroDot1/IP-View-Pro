// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — ScannerTab.cpp
//  C++26: structured bindings, constexpr, [[nodiscard]]
//  GUI for the asynchronous port scanner (ScannerModule).
//  Provides Quick Scan (28 known ports) and custom port range.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "ScannerTab.h"
#include "Theme.h"

#include <QHeaderView>
#include <QTimer>

// ═══════════════════════════════════════════════════════════════════════════════
ScannerTab::ScannerTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    mScanner = new IPView::Scanner::ScannerModule(this);

    connect(mScanner, &IPView::Scanner::ScannerModule::portFound,
            this, &ScannerTab::onPortFound);
    connect(mScanner, &IPView::Scanner::ScannerModule::scanCompleted,
            this, &ScannerTab::onScanCompleted);
    connect(mScanner, &IPView::Scanner::ScannerModule::scanProgress,
            this, &ScannerTab::onScanProgress);
    connect(mScanner, &IPView::Scanner::ScannerModule::scanError,
            this, &ScannerTab::onScanError);
    connect(mScanner, &IPView::Scanner::ScannerModule::scanCancelled,
            this, &ScannerTab::onScanCancelled);
}

// ═══════════════════════════════════════════════════════════════════════════════
void ScannerTab::setupUI() noexcept
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // ── Title ──────────────────────────────────────────────────────────────
    auto *title = new QLabel(QStringLiteral("Port Scanner"));
    title->setStyleSheet(QStringLiteral("color: %1; font-size: 16px; font-weight: bold;").arg(C_PRIMARY));
    mainLayout->addWidget(title);

    // ── Target row ─────────────────────────────────────────────────────────
    auto *targetRow = new QHBoxLayout();
    auto *targetLabel = new QLabel(QStringLiteral("Target IP / Hostname:"));
    targetEdit = new QLineEdit();
    targetEdit->setPlaceholderText(QStringLiteral("e.g. 192.168.1.1 or scanme.nmap.org"));
    targetEdit->setStyleSheet(inputStyle());

    QList<QPushButton*> quickPorts;
    auto *quick22 = new QPushButton(QStringLiteral("22 (SSH)"));
    auto *quick80 = new QPushButton(QStringLiteral("80 (HTTP)"));
    auto *quick443 = new QPushButton(QStringLiteral("443 (HTTPS)"));
    auto *quick3306 = new QPushButton(QStringLiteral("3306 (MySQL)"));
    for (auto *btn : {quick22, quick80, quick443, quick3306}) {
        btn->setStyleSheet(btnSmallStyle());
        btn->setFixedHeight(28);
    }

    targetRow->addWidget(targetLabel);
    targetRow->addWidget(targetEdit, 1);
    targetRow->addWidget(quick22);
    targetRow->addWidget(quick80);
    targetRow->addWidget(quick443);
    targetRow->addWidget(quick3306);
    mainLayout->addLayout(targetRow);

    // ── Options row ────────────────────────────────────────────────────────
    auto *optionsRow = new QHBoxLayout();

    quickScanCheck = new QCheckBox(QStringLiteral("Quick Scan (28 ports)"));
    quickScanCheck->setChecked(true);
    quickScanCheck->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT));

    auto *rangeLabel = new QLabel(QStringLiteral("Custom range:"));
    rangeLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT_SEC));

    portFromSpin = new QSpinBox();
    portFromSpin->setRange(1, 65535);
    portFromSpin->setValue(1);
    portFromSpin->setFixedWidth(80);
    portFromSpin->setStyleSheet(inputStyle());
    portFromSpin->setEnabled(false);

    auto *dashLabel = new QLabel(QStringLiteral("–"));
    dashLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT));

    portToSpin = new QSpinBox();
    portToSpin->setRange(1, 65535);
    portToSpin->setValue(1024);
    portToSpin->setFixedWidth(80);
    portToSpin->setStyleSheet(inputStyle());
    portToSpin->setEnabled(false);

    connect(quickScanCheck, &QCheckBox::toggled, this, [this](bool checked) {
        portFromSpin->setEnabled(!checked);
        portToSpin->setEnabled(!checked);
    });

    scanButton = new QPushButton(QStringLiteral("▶ Start Scan"));
    scanButton->setProperty("accent", true);
    scanButton->setStyleSheet(btnAccentStyle());

    cancelButton = new QPushButton(QStringLiteral("■ Cancel"));
    cancelButton->setStyleSheet(QStringLiteral(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %2; "
        "border-radius: %3; padding: %4; }"
    ).arg(C_BG_ELEVATED, C_ERROR, RADIUS_MD, PADDING_BTN));
    cancelButton->setEnabled(false);

    optionsRow->addWidget(quickScanCheck);
    optionsRow->addSpacing(10);
    optionsRow->addWidget(rangeLabel);
    optionsRow->addWidget(portFromSpin);
    optionsRow->addWidget(dashLabel);
    optionsRow->addWidget(portToSpin);
    optionsRow->addStretch();
    optionsRow->addWidget(scanButton);
    optionsRow->addWidget(cancelButton);
    mainLayout->addLayout(optionsRow);

    // ── Progress bar ───────────────────────────────────────────────────────
    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    mainLayout->addWidget(progressBar);

    // ── Status ─────────────────────────────────────────────────────────────
    statusLabel = new QLabel(QStringLiteral("Enter a target IP and click Start Scan."));
    statusLabel->setStyleSheet(statusLabelStyle());
    mainLayout->addWidget(statusLabel);

    // ── Results table ──────────────────────────────────────────────────────
    resultTable = new QTableWidget(0, 4);
    resultTable->setHorizontalHeaderLabels({
        QStringLiteral("Port"), QStringLiteral("State"),
        QStringLiteral("Service"), QStringLiteral("Latency")
    });
    resultTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    resultTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    resultTable->verticalHeader()->setVisible(false);
    resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultTable->setAlternatingRowColors(true);
    resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(resultTable, 1);

    // ── Connections ────────────────────────────────────────────────────────
    connect(scanButton,   &QPushButton::clicked, this, &ScannerTab::onStartScan);
    connect(cancelButton, &QPushButton::clicked, this, &ScannerTab::onCancelScan);

    // Quick buttons set target IP as placeholder
    connect(quick22,   &QPushButton::clicked, this, [this]() { targetEdit->setText(QStringLiteral("192.168.1.1")); });
    connect(quick80,   &QPushButton::clicked, this, [this]() { targetEdit->setText(QStringLiteral("192.168.1.1")); });
    connect(quick443,  &QPushButton::clicked, this, [this]() { targetEdit->setText(QStringLiteral("192.168.1.1")); });
    connect(quick3306, &QPushButton::clicked, this, [this]() { targetEdit->setText(QStringLiteral("192.168.1.1")); });

    connect(resultTable, &QTableWidget::itemSelectionChanged,
            this, &ScannerTab::onSelectionChanged);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Slots
// ═══════════════════════════════════════════════════════════════════════════════

void ScannerTab::onStartScan()
{
    QString const target = targetEdit->text().trimmed();
    if (target.isEmpty()) {
        statusLabel->setText(QStringLiteral("Please enter a target IP or hostname."));
        return;
    }

    QVector<int> ports = collectPorts();
    if (ports.isEmpty()) {
        statusLabel->setText(QStringLiteral("No ports selected for scanning."));
        return;
    }

    resultTable->setRowCount(0);
    progressBar->setValue(0);
    progressBar->setVisible(true);
    setScanningState(true);
    statusLabel->setText(QStringLiteral("Scanning %1 ...").arg(target));

    mTotalPorts = static_cast<int>(ports.size());
    mScanner->runScan(target, ports);
}

void ScannerTab::onCancelScan()
{
    statusLabel->setText(QStringLiteral("Cancelling scan..."));
    mScanner->cancelScan();
}

void ScannerTab::onPortFound(const IPView::Scanner::ScanResult &result)
{
    int const row = resultTable->rowCount();
    resultTable->insertRow(row);

    auto *portItem = new QTableWidgetItem(QString::number(result.port));
    portItem->setTextAlignment(Qt::AlignCenter);

    auto *stateItem = new QTableWidgetItem(QStringLiteral("OPEN"));
    stateItem->setForeground(QBrush(QColor(C_SUCCESS)));
    stateItem->setTextAlignment(Qt::AlignCenter);

    auto *serviceItem = new QTableWidgetItem(
        result.service.isEmpty() ? QStringLiteral("unknown") : result.service);

    auto *latencyItem = new QTableWidgetItem(
        QStringLiteral("%1 ms").arg(result.latencyMs));
    latencyItem->setTextAlignment(Qt::AlignCenter);

    resultTable->setItem(row, 0, portItem);
    resultTable->setItem(row, 1, stateItem);
    resultTable->setItem(row, 2, serviceItem);
    resultTable->setItem(row, 3, latencyItem);
}

void ScannerTab::onScanCompleted(const QVector<IPView::Scanner::ScanResult> &results)
{
    setScanningState(false);
    progressBar->setVisible(false);

    int openCount = 0;
    for (auto const &r : results) {
        if (r.open) ++openCount;
    }

    statusLabel->setText(QStringLiteral("Scan completed – %1 ports open, %2 closed")
                             .arg(openCount)
                             .arg(static_cast<int>(results.size()) - openCount));
}

void ScannerTab::onScanProgress(int current, int total)
{
    if (total > 0) {
        progressBar->setValue(current * 100 / total);
        statusLabel->setText(QStringLiteral("Scanning... %1 / %2 ports")
                                 .arg(current).arg(total));
    }
}

void ScannerTab::onScanError(const QString &message)
{
    setScanningState(false);
    progressBar->setVisible(false);
    statusLabel->setText(QStringLiteral("Error: %1").arg(message));
}

void ScannerTab::onScanCancelled()
{
    setScanningState(false);
    progressBar->setVisible(false);
    statusLabel->setText(QStringLiteral("Scan cancelled by user."));
}

void ScannerTab::onSelectionChanged()
{
    // Placeholder for future extensions (e.g., detail view)
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private Helpers
// ═══════════════════════════════════════════════════════════════════════════════

void ScannerTab::setScanningState(bool scanning) noexcept
{
    scanButton->setEnabled(!scanning);
    cancelButton->setEnabled(scanning);
    targetEdit->setEnabled(!scanning);
    quickScanCheck->setEnabled(!scanning);
    portFromSpin->setEnabled(!scanning && !quickScanCheck->isChecked());
    portToSpin->setEnabled(!scanning && !quickScanCheck->isChecked());
}

QVector<int> ScannerTab::collectPorts() const noexcept
{
    if (quickScanCheck->isChecked()) {
        return IPView::Scanner::ScannerModule::defaultPorts();
    }

    int const from = portFromSpin->value();
    int const to   = portToSpin->value();
    if (from > to) return {};

    QVector<int> ports;
    ports.reserve(to - from + 1);
    for (int p = from; p <= to; ++p) {
        ports.append(p);
    }
    return ports;
}
