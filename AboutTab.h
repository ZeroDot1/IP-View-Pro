// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.7.0 — AboutTab.h
//  Shows build information: compiler, system, version, date.
//  No license — Public Domain.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef ABOUTTAB_H
#define ABOUTTAB_H

#include <QWidget>

class QLabel;

class AboutTab : public QWidget
{
    Q_OBJECT

public:
    explicit AboutTab(QWidget *parent = nullptr);

private:
    void setupUI() noexcept;
    [[nodiscard]] static QString buildInfo() noexcept;
    [[nodiscard]] static QString compilerInfo() noexcept;
    [[nodiscard]] static QString systemInfo() noexcept;
};

#endif // ABOUTTAB_H