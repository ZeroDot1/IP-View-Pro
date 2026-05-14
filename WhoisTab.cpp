#include "WhoisTab.h"
#include "Theme.h"
#include <QHeaderView>
#include <QJsonArray>

WhoisTab::WhoisTab(QWidget *parent)
    : QWidget(parent), whoisManager(new WhoisManager(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    QHBoxLayout *topRow = new QHBoxLayout();
    ipEdit = new QLineEdit();
    ipEdit->setPlaceholderText("Enter IP address...");
    ipEdit->setStyleSheet(inputStyle());

    apiCombo = new QComboBox();
    apiCombo->addItems(whoisManager->getAvailableApis());
    apiCombo->setStyleSheet(comboStyle());

    lookupButton = new QPushButton("Lookup");
    lookupButton->setProperty("accent", true);
    lookupButton->setStyleSheet(btnAccentStyle());

    topRow->addWidget(new QLabel("IP:"));
    topRow->addWidget(ipEdit);
    topRow->addWidget(new QLabel("API:"));
    topRow->addWidget(apiCombo);
    topRow->addWidget(lookupButton);

    resultArea = new QTableWidget(0, 2);
    resultArea->setObjectName("whoisTable");
    resultArea->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    resultArea->verticalHeader()->setVisible(false);
    resultArea->setHorizontalHeaderLabels({"Key", "Value"});

    layout->addLayout(topRow);
    layout->addWidget(resultArea);

    connect(lookupButton, &QPushButton::clicked, this, &WhoisTab::onLookupClicked);
    connect(whoisManager, &WhoisManager::lookupFinished, this, &WhoisTab::onLookupFinished);
}

void WhoisTab::setIp(const QString &ip) { ipEdit->setText(ip); }

void WhoisTab::onLookupClicked()
{
    QString ip = ipEdit->text().trimmed();
    if (ip.isEmpty()) return;
    resultArea->setRowCount(0);
    whoisManager->lookup(ip, apiCombo->currentText());
}

void WhoisTab::onLookupFinished(const QString &result)
{
    QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        resultArea->setRowCount(static_cast<int>(obj.size()));
        int row = 0;
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            resultArea->setItem(row, 0, new QTableWidgetItem(it.key()));
            resultArea->setItem(row, 1, new QTableWidgetItem(it.value().toVariant().toString()));
            row++;
        }
    } else {
        resultArea->setRowCount(1);
        resultArea->setItem(0, 0, new QTableWidgetItem("Raw Data"));
        resultArea->setItem(0, 1, new QTableWidgetItem(result));
    }
}
