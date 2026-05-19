#include "WhoisTab.h"
#include "Theme.h"
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

WhoisTab::WhoisTab(QWidget *parent)
    : QWidget(parent)
    , whoisManager(new WhoisManager(this))
{
    setupUI();
}

void WhoisTab::setupUI() noexcept
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    auto *topRow = new QHBoxLayout();
    ipEdit = new QLineEdit();
    ipEdit->setPlaceholderText(QStringLiteral("Enter IP address..."));
    ipEdit->setStyleSheet(inputStyle());

    apiCombo = new QComboBox();
    apiCombo->addItems(whoisManager->getAvailableApis());
    apiCombo->setStyleSheet(comboStyle());

    lookupButton = new QPushButton(QStringLiteral("Lookup"));
    lookupButton->setProperty("accent", true);
    lookupButton->setStyleSheet(btnAccentStyle());

    auto *ipLabel = new QLabel(QStringLiteral("IP:"));
    ipLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT));
    auto *apiLabel = new QLabel(QStringLiteral("API:"));
    apiLabel->setStyleSheet(QStringLiteral("color: %1;").arg(C_TEXT));

    topRow->addWidget(ipLabel);
    topRow->addWidget(ipEdit);
    topRow->addWidget(apiLabel);
    topRow->addWidget(apiCombo);
    topRow->addWidget(lookupButton);

    resultArea = new QTableWidget(0, 2);
    resultArea->setObjectName(QStringLiteral("whoisTable"));
    resultArea->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    resultArea->verticalHeader()->setVisible(false);
    resultArea->setHorizontalHeaderLabels({QStringLiteral("Key"), QStringLiteral("Value")});

    layout->addLayout(topRow);
    layout->addWidget(resultArea);

    connect(lookupButton, &QPushButton::clicked, this, &WhoisTab::onLookupClicked);
    connect(whoisManager, &WhoisManager::lookupFinished, this, &WhoisTab::onLookupFinished);
}

void WhoisTab::setIp(const QString &ip) noexcept
{
    ipEdit->setText(ip);
}

void WhoisTab::onLookupClicked()
{
    QString const ip = ipEdit->text().trimmed();
    if (ip.isEmpty()) return;
    resultArea->setRowCount(0);
    whoisManager->lookup(ip, apiCombo->currentText());
}

void WhoisTab::onLookupFinished(const QString &result)
{
    QJsonDocument const doc = QJsonDocument::fromJson(result.toUtf8());
    if (doc.isObject()) {
        QJsonObject const obj = doc.object();
        resultArea->setRowCount(static_cast<int>(obj.size()));
        int row = 0;
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            resultArea->setItem(row, 0, new QTableWidgetItem(it.key()));
            resultArea->setItem(row, 1, new QTableWidgetItem(it.value().toVariant().toString()));
            ++row;
        }
    } else {
        resultArea->setRowCount(1);
        resultArea->setItem(0, 0, new QTableWidgetItem(QStringLiteral("Raw Data")));
        resultArea->setItem(0, 1, new QTableWidgetItem(result));
    }
}
