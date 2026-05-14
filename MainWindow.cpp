// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — MainWindow.cpp
//  C++26: std::array for compile-time constants, auto, [[maybe_unused]]
//  QStringLiteral, structured bindings
// ═══════════════════════════════════════════════════════════════════════════════

#include "MainWindow.h"
#include "Theme.h"
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextEdit>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QFile>
#include <QTextStream>

#include <array>    // C++26: std::to_array for constexpr field lists
#include <utility>  // std::pair

// ── Compile-time constants for UI field names ─────────────────────────────
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
MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setupTray();

    networkManager  = new NetworkManager(this);
    flagLoader      = new FlagLoader(this);
    autoRefreshTimer = new QTimer(this);

    connect(autoRefreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshClicked);

    apiCombo->addItems(networkManager->getApiNames());
    apiCombo->setCurrentIndex(0);

    // ── Signal-Slot Connections ──────────────────────────────────────────
    connect(refreshButton,  &QPushButton::clicked,        this, &MainWindow::onRefreshClicked);
    connect(networkManager, &NetworkManager::dataReceived, this, &MainWindow::onDataReceived);
    connect(networkManager, &NetworkManager::errorOccurred, this, [this](QString const& err) {
        statusLabel->setText(QStringLiteral("Error: ") + err);
    });
    connect(apiCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onApiChanged);
    connect(ipv6CheckBox,        &QCheckBox::toggled, this, &MainWindow::onIPv6Toggled);
    connect(autoRefreshCheckBox, &QCheckBox::toggled, this, &MainWindow::onAutoRefreshToggled);

    onRefreshClicked();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  System Tray
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::setupTray() noexcept
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(QStringLiteral(":/icon.svg")));
    trayIcon->setToolTip(QStringLiteral("IP View Pro"));

    trayMenu = new QMenu(this);
    restoreAction = trayMenu->addAction(QStringLiteral("Restore Window"),
                                        this, &MainWindow::showNormal);
    trayMenu->addSeparator();
    exitAction = trayMenu->addAction(QStringLiteral("Exit"),
                                     this, &MainWindow::onExitRequested);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    connect(trayIcon, &QSystemTrayIcon::activated,
            this, &MainWindow::onTrayIconActivated);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Tray & Event Handlers
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger) {
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            activateWindow();
        }
    }
}

void MainWindow::onExitRequested()
{
    reallyQuit = true;
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!reallyQuit && trayIcon->isVisible()) {
        hide();
        event->ignore();
        trayIcon->showMessage(QStringLiteral("IP View Pro"),
                              QStringLiteral("Application running in tray."),
                              QSystemTrayIcon::Information, 2000);
    } else {
        event->accept();
    }
}

// ── Country code (e.g. "DE") → Unicode flag (🇩🇪) ─────────────────────────
//  Uses Unicode Regional Indicator Symbols (U+1F1E6..U+1F1FF).
//  Fallback: if the code is invalid, returns a white flag "🏳".
[[nodiscard]]
static QString countryCodeToFlag(const QString &cc) noexcept
{
    if (cc.length() != 2) return QStringLiteral("\U0001F3F3"); // white flag

    QString const upper = cc.toUpper();
    char32_t const a = static_cast<char32_t>(upper[0].unicode());
    char32_t const b = static_cast<char32_t>(upper[1].unicode());

    if (a < 'A' || a > 'Z' || b < 'A' || b > 'Z')
        return QStringLiteral("\U0001F3F3");

    char32_t const chars[2] = {
        0x1F1E6U + (static_cast<char32_t>(a) - 'A'),
        0x1F1E6U + (static_cast<char32_t>(b) - 'A')
    };
    return QString::fromUcs4(chars, 2);
}

// ── Tray hover tooltip (dark-theme design with Unicode) ────────────────────
//  QSystemTrayIcon does not support HTML/CSS, so the tooltip
//  is formatted using Unicode box-drawing characters and regional indicator flags.
void MainWindow::updateTrayTooltip(const QJsonObject &jsonData) noexcept
{
    QString tip;
    QString const ip     = jsonData[QStringLiteral("ip")].toString();
    QString const city   = jsonData[QStringLiteral("city")].toString();
    QString const cc     = jsonData[QStringLiteral("country_code")].toString();
    QString const cn     = jsonData[QStringLiteral("country_name")].toString();
    QString const org    = jsonData[QStringLiteral("org")].toString();
    QString const asn    = jsonData[QStringLiteral("asn")].toString();

    // ── Header line with app name ──────────────────────────────────────────
    tip += QStringLiteral("\u250F\u2501\u2501\u2501 IP View Pro \u2501\u2501\u2501\u2513\n"); // ┏━━━ IP View Pro ━━━┓
    tip += QStringLiteral("\u2503  \u25CF Online                     \u2503\n");             // ┃  ● Online          ┃

    // ── IP + Flag ─────────────────────────────────────────────────────
    tip += QStringLiteral("\u2523\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u252B\n"); // ┣━━━━━━━━━━━━━━━━━━━━━━━┫
    tip += QStringLiteral("\u2503  %1 %2\n")                               // ┃  🇩🇪 1.2.3.4
              .arg(countryCodeToFlag(cc),
                   ip.isEmpty()  ? QStringLiteral("—") : ip);
    if (!cn.isEmpty()) {
        tip += QStringLiteral("\u2503     %1").arg(cn);                   // ┃     Germany
        if (!city.isEmpty()) tip += QStringLiteral("  ·  %1").arg(city);  //          ·  Berlin
        tip += QLatin1Char('\n');
    }

    // ── ISP / ASN ───────────────────────────────────────────────────────
    if (!org.isEmpty() || !asn.isEmpty()) {
        tip += QStringLiteral("\u2523\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u252B\n"); // ┣━━━━━━━━━━━━━━━━━━━━━━━┫
        if (!org.isEmpty())
            tip += QStringLiteral("\u2503  %1\n").arg(org);
        if (!asn.isEmpty())
            tip += QStringLiteral("\u2503  %1\n").arg(asn);
    }

    // ── Timestamp ─────────────────────────────────────────────────────
    tip += QStringLiteral("\u2523\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u252B\n"); // ┣━━━━━━━━━━━━━━━━━━━━━━━┫
    tip += QStringLiteral("\u2503  %1\n")                                   // ┃  14:32:01
              .arg(QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss")));

    // ── Footer ────────────────────────────────────────────────────────
    tip += QStringLiteral("\u2517\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u2501\u251B"); // ┗━━━━━━━━━━━━━━━━━━━━━━━┛

    trayIcon->setToolTip(tip);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  UI Setup
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::setupUI() noexcept
{
    setWindowTitle(QStringLiteral("IP View Pro v") + QApplication::applicationVersion());
    setMinimumSize(800, 700);
    resize(950, 850);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    tabWidget = new QTabWidget();

    // ── Overview Tab ───────────────────────────────────────────────────────
    overviewTab = new QWidget();
    auto *ovLayout = new QVBoxLayout(overviewTab);
    ovLayout->setSpacing(15);
    ovLayout->setContentsMargins(20, 20, 20, 20);

    // Top controls row
    auto *topRow = new QHBoxLayout();
    apiCombo = new QComboBox();
    apiCombo->setFixedWidth(200);
    apiCombo->setStyleSheet(comboStyle());

    ipv6CheckBox       = new QCheckBox(QStringLiteral("IPv6"));
    autoRefreshCheckBox = new QCheckBox(QStringLiteral("Auto (5m)"));

    refreshButton = new QPushButton(QStringLiteral("⟳ Refresh"));
    refreshButton->setProperty("accent", true);
    refreshButton->setStyleSheet(btnAccentStyle());

    topRow->addWidget(apiCombo);
    topRow->addWidget(ipv6CheckBox);
    topRow->addWidget(autoRefreshCheckBox);
    topRow->addStretch();
    topRow->addWidget(refreshButton);

    // IP Card
    auto *ipCard = new QFrame();
    ipCard->setProperty("card", true);
    ipCard->setStyleSheet(cardStyle());
    auto *ipCardLayout = new QVBoxLayout(ipCard);
    ipLabel = new QLabel(QStringLiteral("Loading IP..."));
    ipLabel->setFont(QFont(QStringLiteral("Segoe UI"), 26, QFont::Bold));
    ipLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_PRIMARY));
    ipLabel->setAlignment(Qt::AlignCenter);
    ipCardLayout->addWidget(ipLabel);

    // Data Table — uses the constexpr FIELD_NAMES
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

    // Bottom info row
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

    ovLayout->addLayout(topRow);
    ovLayout->addWidget(ipCard);
    ovLayout->addWidget(ipTable);
    ovLayout->addLayout(botRow);

    // ── Sub-Tabs ──────────────────────────────────────────────────────────
    whoisTab     = new WhoisTab();
    toolsTab     = new ToolsTab();
    historyTab   = new HistoryTab();
    speedtestTab = new SpeedtestTab();
    aboutTab     = new AboutTab();

    tabWidget->addTab(overviewTab,  QIcon(QStringLiteral(":/svgs/chart-bar.svg")),
                      QStringLiteral(" Overview"));
    tabWidget->addTab(whoisTab,     QIcon(QStringLiteral(":/svgs/search.svg")),
                      QStringLiteral(" Whois Lookup"));
    tabWidget->addTab(toolsTab,     QIcon(QStringLiteral(":/svgs/wrench.svg")),
                      QStringLiteral(" Network Tools"));
    tabWidget->addTab(historyTab,   QIcon(QStringLiteral(":/svgs/scroll.svg")),
                      QStringLiteral(" History"));
    tabWidget->addTab(speedtestTab, QIcon(QStringLiteral(":/svgs/lightning.svg")),
                      QStringLiteral(" Speedtest"));
    tabWidget->addTab(aboutTab,     QIcon(QStringLiteral(":/svgs/info.svg")),
                      QStringLiteral(" About"));

    mainLayout->addWidget(tabWidget);

    // Status bar
    auto *statusLayout = new QHBoxLayout();
    statusLabel = new QLabel(QStringLiteral("Ready"));
    statusLabel->setStyleSheet(statusLabelStyle());
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    mainLayout->addLayout(statusLayout);

    // Button-Connections
    connect(copyAllButton,   &QPushButton::clicked, this, &MainWindow::onCopyAllClicked);
    connect(exportJsonButton, &QPushButton::clicked, this, &MainWindow::onExportJsonClicked);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Data Processing
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onDataReceived(const QJsonObject &jsonData)
{
    // Only add to history on IP change
    if (history.isEmpty() || history.first()[QStringLiteral("ip")] != jsonData[QStringLiteral("ip")]) {
        history.prepend(jsonData);
        static constexpr int MAX_HISTORY = 50;
        while (history.size() > MAX_HISTORY) {
            history.removeLast();
        }
        historyTab->updateHistory(history);
    }

    updateDataDisplay(jsonData);
    whoisTab->setIp(jsonData[QStringLiteral("ip")].toString());
    toolsTab->setTargetIp(jsonData[QStringLiteral("ip")].toString());
}

void MainWindow::updateDataDisplay(const QJsonObject &jsonData) noexcept
{
    currentData = jsonData;

    QString const ip = jsonData[QStringLiteral("ip")].toString();
    ipLabel->setText(ip.isEmpty() ? QStringLiteral("N/A") : ip);

    // C++26 structured bindings with constexpr arrays
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
    onlineLabel->setText(QStringLiteral("● Online"));
    onlineLabel->setStyleSheet(onlineLabelStyle());

    QString const cc = jsonData[QStringLiteral("country_code")].toString();
    if (!cc.isEmpty()) {
        flagLoader->loadFlag(cc, flagLabel);
    } else {
        flagLabel->clear();
    }

    updateTrayTooltip(jsonData);
    statusLabel->setText(QStringLiteral("Last update successful"));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Buttons & Controls
// ═══════════════════════════════════════════════════════════════════════════════

void MainWindow::onRefreshClicked()
{
    statusLabel->setText(QStringLiteral("Refreshing data..."));
    if (ipv6CheckBox->isChecked()) {
        networkManager->fetchIPv6Data();
    } else {
        networkManager->fetchIPData();
    }
}

void MainWindow::onApiChanged(int index)
{
    networkManager->setSelectedAPI(index);
}

void MainWindow::onIPv6Toggled(bool checked)
{
    apiCombo->setEnabled(!checked);
}

void MainWindow::onAutoRefreshToggled(bool checked)
{
    if (checked) {
        autoRefreshTimer->start(300000);  // 5 minutes
    } else {
        autoRefreshTimer->stop();
    }
}

void MainWindow::onCopyAllClicked()
{
    if (currentData.isEmpty()) return;

    QString text;
    text += QStringLiteral("IP View Pro - Data Export\n");
    text += QStringLiteral("========================\n");
    text += QStringLiteral("Timestamp: %1\n\n")
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));

    // C++26: iterate through structured bindings and std::array in parallel
    static_assert(FIELD_NAMES.size() == FIELD_KEYS.size(),
                  "FIELD_NAMES and FIELD_KEYS must have the same size");

    for (std::size_t i = 0; i < FIELD_NAMES.size(); ++i) {
        QString const key = QString::fromUtf8(FIELD_KEYS[i].data(),
                                              static_cast<qsizetype>(FIELD_KEYS[i].size()));
        QString const val = currentData[key].toString();
        QString const fieldName = QString::fromUtf8(FIELD_NAMES[i].data(),
                                                     static_cast<qsizetype>(FIELD_NAMES[i].size()));
        text += QStringLiteral("%1: %2\n")
                    .arg(fieldName, -22)
                    .arg(val.isEmpty() ? QStringLiteral("N/A") : val);
    }

    QApplication::clipboard()->setText(text);
    statusLabel->setText(QStringLiteral("Copied to clipboard!"));
}

void MainWindow::onExportJsonClicked()
{
    if (currentData.isEmpty()) {
        statusLabel->setText(QStringLiteral("No data to export"));
        return;
    }

    QString const fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("Export IP Data as JSON"),
        QStringLiteral("ipview_%1.json")
            .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"))),
        QStringLiteral("JSON Files (*.json)")
    );

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
                             QStringLiteral("Export Error"),
                             QStringLiteral("Could not open file for writing:\n%1")
                                 .arg(file.errorString()));
        return;
    }

    QJsonDocument const doc(currentData);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    statusLabel->setText(QStringLiteral("Data exported to: %1").arg(fileName));
}
