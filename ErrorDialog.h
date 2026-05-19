// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — ErrorDialog.h
//  C++26 — Central error dialog component
//  Consistent QMessageBox error display with unified theme.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QMessageBox>
#include <QString>
#include <QWidget>

// ═══════════════════════════════════════════════════════════════════════════════
//  ErrorDialog — static helpers for consistent error dialogs
// ═══════════════════════════════════════════════════════════════════════════════

namespace IPView::UI {

struct ErrorDialog
{
    ErrorDialog() = delete; // Nur statische Nutzung

    /// Show a modal error dialog with title and message.
    static void
    showError(QWidget *parent, const QString &title, const QString &message) noexcept
    {
        QMessageBox::warning(parent, title, message);
    }

    /// Show a critical error (QMessageBox::Critical instead of Warning).
    static void
    showCritical(QWidget *parent, const QString &title, const QString &message) noexcept
    {
        QMessageBox::critical(parent, title, message);
    }

    /// Show an information dialog (QMessageBox::Information).
    static void
    showInfo(QWidget *parent, const QString &title, const QString &message) noexcept
    {
        QMessageBox::information(parent, title, message);
    }

    /// Show a confirmation dialog (QMessageBox::Question).
    [[nodiscard]] static QMessageBox::StandardButton
    showConfirm(QWidget *parent, const QString &title, const QString &message,
                QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No) noexcept
    {
        return QMessageBox::question(parent, title, message, buttons);
    }
};

} // namespace IPView::UI

#endif // ERRORDIALOG_H
