// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — ErrorDialog.h
//  C++26 — Zentrale Fehlerdialog-Komponente
//  Einheitliche QMessageBox-Fehleranzeige mit konsistentem Theme.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QMessageBox>
#include <QString>
#include <QWidget>

// ═══════════════════════════════════════════════════════════════════════════════
//  ErrorDialog — statische Helfer für einheitliche Fehlerdialoge
// ═══════════════════════════════════════════════════════════════════════════════

namespace IPView::UI {

struct ErrorDialog
{
    ErrorDialog() = delete; // Nur statische Nutzung

    /// Zeigt einen modalen Fehlerdialog mit Titel und Nachricht.
    static void
    showError(QWidget *parent, const QString &title, const QString &message) noexcept
    {
        QMessageBox::warning(parent, title, message);
    }

    /// Zeigt einen kritischen Fehler (QMessageBox::Critical statt Warning).
    static void
    showCritical(QWidget *parent, const QString &title, const QString &message) noexcept
    {
        QMessageBox::critical(parent, title, message);
    }

    /// Zeigt einen Informationsdialog (QMessageBox::Information).
    static void
    showInfo(QWidget *parent, const QString &title, const QString &message) noexcept
    {
        QMessageBox::information(parent, title, message);
    }

    /// Zeigt einen Bestätigungsdialog (QMessageBox::Question).
    [[nodiscard]] static QMessageBox::StandardButton
    showConfirm(QWidget *parent, const QString &title, const QString &message,
                QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No) noexcept
    {
        return QMessageBox::question(parent, title, message, buttons);
    }
};

} // namespace IPView::UI

#endif // ERRORDIALOG_H
