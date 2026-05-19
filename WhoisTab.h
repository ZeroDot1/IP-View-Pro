#ifndef WHOISTAB_H
#define WHOISTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include "WhoisManager.h"

class WhoisTab : public QWidget
{
    Q_OBJECT
public:
    explicit WhoisTab(QWidget *parent = nullptr);
    ~WhoisTab() override = default;

    void setIp(const QString &ip) noexcept;

private slots:
    void onLookupClicked();
    void onLookupFinished(const QString &result);

private:
    void setupUI() noexcept;

    QLineEdit    *ipEdit{nullptr};
    QComboBox    *apiCombo{nullptr};
    QPushButton  *lookupButton{nullptr};
    QTableWidget *resultArea{nullptr};
    WhoisManager *whoisManager{nullptr};
};

#endif
