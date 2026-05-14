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
    void setIp(const QString &ip);

private slots:
    void onLookupClicked();
    void onLookupFinished(const QString &result);

private:
    QLineEdit *ipEdit;
    QComboBox *apiCombo;
    QPushButton *lookupButton;
    QTableWidget *resultArea;
    WhoisManager *whoisManager;
};

#endif
