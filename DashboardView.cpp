// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — DashboardView.cpp
//  C++26: constexpr std::to_array, structured bindings, [[nodiscard]]
//  Central overview — API selection, IP card, data table, export.
//  Extracted from MainWindow for modular architecture.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "DashboardView.h"
#include "Theme.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QFileDialog>
#include <QFont>
#include <QHeaderView>
#include <QJsonDocument>
#include <QMessageBox>

// ── Compile-time field definitions ─────────────────────────────────────────
//  C++26 std::array: guaranteed stack-allocated, constexpr
static constexpr auto FIELD_NAMES = std::to_array<std::string_view>({
    "IP", "City", "Region", "Country", "Continent", "ISP / Organization",
    "ASN", "Latitude", "Longitude", "Postal Code", "Timezone", "Currency",
    "Calling Code", "Languages", "Hostname", "Network Type", "Security"
});

static constexpr auto FIELD_KEYS = std::to_array<std::string_view>({
    "ip", "city", "region", "country_name", "continent", "org",
    "asn", "latitude", "longitude", "postal", "timezone", "currency",
    "calling_code", "languages", "hostname", "type", "security"
});

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::UI {

// ═══════════════════════════════════════════════════════════════════════════════
DashboardView::DashboardView(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

// ═══════════════════════════════════════════════════════════════════════════════
void DashboardView::setupUI() noexcept
{
    auto *layout = new QVBoxLayout(this);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    // ── Top controls row ──────────────────────────────────────────────────
    auto *topRow = new QHBoxLayout();
    apiCombo = new QComboBox();
    apiCombo->setFixedWidth(200);
    apiCombo->setStyleSheet(comboStyle());

    ipv6CheckBox       = new QCheckBox(QStringLiteral("IPv6"));
    autoRefreshCheckBox = new QCheckBox(QStringLiteral("Auto (5m)"));

    refreshButton = new QPushButton(QStringLiteral("\u27F3 Refresh"));
    refreshButton->setProperty("accent", true);
    refreshButton->setStyleSheet(btnAccentStyle());

    topRow->addWidget(apiCombo);
    topRow->addWidget(ipv6CheckBox);
    topRow->addWidget(autoRefreshCheckBox);
    topRow->addStretch();
    topRow->addWidget(refreshButton);

    // ── IP Card ────────────────────────────────────────────────────────────
    auto *ipCard = new QFrame();
    ipCard->setProperty("card", true);
    ipCard->setStyleSheet(cardStyle());
    auto *ipCardLayout = new QVBoxLayout(ipCard);
    ipLabel = new QLabel(QStringLiteral("Loading IP..."));
    ipLabel->setFont(QFont(QStringLiteral("Segoe UI"), 26, QFont::Bold));
    ipLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_PRIMARY));
    ipLabel->setAlignment(Qt::AlignCenter);
    ipCardLayout->addWidget(ipLabel);

    // ── Data Table ─────────────────────────────────────────────────────────
    ipTable = new QTableWidget(static_cast<int>(FIELD_NAMES.size()), 2);
    ipTable->setObjectName(QStringLiteral("ipTable"));
    ipTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ipTable->verticalHeader()->setVisible(false);
    ipTable->setHorizontalHeaderLabels({QStringLiteral("Field"), QStringLiteral("Value")});
    ipTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ipTable->setAlternatingRowColors(true);

    for (auto i = 0; i < static_cast<int>(FIELD_NAMES.size()); ++i) {
        auto *item = new QTableWidgetItem(
            QString::fromUtf8(FIELD_NAMES[static_cast<std::size_t>(i)].data(),
                              static_cast<qsizetype>(FIELD_NAMES[static_cast<std::size_t>(i)].size())));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);
        item->setForeground(QBrush(QColor(QStringLiteral("#888888"))));
        ipTable->setItem(i, 0, item);
    }

    // ── Bottom info row ────────────────────────────────────────────────────
    auto *botRow = new QHBoxLayout();
    timestampLabel = new QLabel();
    onlineLabel    = new QLabel();
    flagLabel      = new QLabel();
    flagLabel->setFixedSize(64, 42);

    copyAllButton = new QPushButton(QIcon(QStringLiteral(":/svgs/clipboard.svg")),
                                    QStringLiteral(" Copy All"));
    copyAllButton->setStyleSheet(btnSmallStyle());

    exportJsonButton = new QPushButton(QIcon(QStringLiteral(":/svgs/floppy-disk.svg")),
                                       QStringLiteral(" Export JSON"));
    exportJsonButton->setStyleSheet(btnSmallStyle());

    botRow->addWidget(timestampLabel);
    botRow->addStretch();
    botRow->addWidget(copyAllButton);
    botRow->addWidget(exportJsonButton);
    botRow->addWidget(onlineLabel);
    botRow->addWidget(flagLabel);

    // ── Assembly ───────────────────────────────────────────────────────
    layout->addLayout(topRow);
    layout->addWidget(ipCard);
    layout->addWidget(ipTable);
    layout->addLayout(botRow);

    // ── Signal-slot connections (dashboard-internal) ───────────────────────
    connect(refreshButton,       &QPushButton::clicked, this, &DashboardView::refreshRequested);
    connect(apiCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &DashboardView::apiChanged);
    connect(ipv6CheckBox,        &QCheckBox::toggled,   this, &DashboardView::ipv6Toggled);
    connect(autoRefreshCheckBox, &QCheckBox::toggled,   this, &DashboardView::autoRefreshToggled);
    connect(copyAllButton,       &QPushButton::clicked, this, &DashboardView::copyAllRequested);
    connect(exportJsonButton,    &QPushButton::clicked, this, &DashboardView::exportJsonRequested);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════════

void DashboardView::updateDisplay(const QJsonObject &jsonData) noexcept
{
    currentData = jsonData;

    QString const ip = jsonData[QStringLiteral("ip")].toString();
    ipLabel->setText(ip.isEmpty() ? QStringLiteral("N/A") : ip);

    // C++26 structured bindings with constexpr arrays
    static_assert(FIELD_NAMES.size() == FIELD_KEYS.size(),
                  "FIELD_NAMES and FIELD_KEYS must have the same size");

    for (auto i = 0; i < static_cast<int>(FIELD_KEYS.size()) && i < ipTable->rowCount(); ++i) {
        QString const key = QString::fromUtf8(
            FIELD_KEYS[static_cast<std::size_t>(i)].data(),
            static_cast<qsizetype>(FIELD_KEYS[static_cast<std::size_t>(i)].size()));
        QString const val = jsonData[key].toString();

        QTableWidgetItem *existing = ipTable->item(i, 1);
        QString const displayVal = val.isEmpty() ? QStringLiteral("N/A") : val;

        if (existing) {
            existing->setText(displayVal);
        } else {
            auto *item = new QTableWidgetItem(displayVal);
            item->setFlags(item->flags() ^ Qt::ItemIsEditable);
            ipTable->setItem(i, 1, item);
        }
    }

    timestampLabel->setText(QStringLiteral("Updated: ")
                            + QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")));
    onlineLabel->setText(QStringLiteral("\u25CF Online"));
    onlineLabel->setStyleSheet(onlineLabelStyle());
}

void DashboardView::setFlagPixmap(const QPixmap &pixmap) noexcept
{
    flagLabel->setPixmap(pixmap);
}

void DashboardView::setStatusMessage(const QString &msg) noexcept
{
    // Is supplemented by MainWindow via status bar
    Q_UNUSED(msg)
}

void DashboardView::setIPv6Mode(bool enabled) noexcept
{
    ipv6CheckBox->setChecked(enabled);
}

void DashboardView::setAutoRefresh(bool enabled) noexcept
{
    autoRefreshCheckBox->setChecked(enabled);
}

void DashboardView::setApiIndex(int index) noexcept
{
    if (index >= 0 && index < apiCombo->count()) {
        apiCombo->setCurrentIndex(index);
    }
}

void DashboardView::setApiNames(const QStringList &names) noexcept
{
    apiCombo->clear();
    apiCombo->addItems(names);
    apiCombo->setCurrentIndex(0);
}

QLabel* DashboardView::flagLabelWidget() const noexcept
{
    return flagLabel;
}

QString DashboardView::selectedApiName() const noexcept
{
    return apiCombo->currentText();
}

bool DashboardView::isIPv6Mode() const noexcept
{
    return ipv6CheckBox->isChecked();
}

} // namespace IPView::UI
