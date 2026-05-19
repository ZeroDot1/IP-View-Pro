// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.2 — TabRegistry.h
//  C++26: std::unordered_map, std::string_view, constexpr
//  Zentrale Tab-Registrierung — Tabs werden als Key-Value-Paare verwaltet.
//  Entkoppelt MainWindow von konkreten Tab-Klassen (Registry Pattern).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef TABREGISTRY_H
#define TABREGISTRY_H

#include <QWidget>
#include <QTabWidget>
#include <QIcon>
#include <QString>
#include <QStringList>

#include <unordered_map>
#include <string_view>
#include <utility>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::UI {

// ── Eintrag in der Tab-Registry ──────────────────────────────────────────
struct TabEntry {
    QString  id;        // Eindeutiger Schlüssel (z.B. "dashboard", "whois")
    QString  title;     // Anzeigetitel
    QIcon    icon;      // Optionales Icon
    QWidget *widget{nullptr}; // Zeiger auf das Tab-Widget
};

// ═══════════════════════════════════════════════════════════════════════════════
class TabRegistry
{
public:
    TabRegistry() = default;
    ~TabRegistry() = default;

    // ── Registry-Operationen ─────────────────────────────────────────────
    /// Einen Tab registrieren (Key muss eindeutig sein).
    template <typename T, typename... Args>
    T* registerTab(const QString &id, const QString &title,
                   const QIcon &icon = QIcon(), Args&&... args)
    {
        static_assert(std::is_base_of_v<QWidget, T>,
                      "Tab type must inherit QWidget");

        auto it = mTabs.find(id);
        if (it != mTabs.end()) {
            return static_cast<T*>(it->second.widget);
        }

        T *widget = new T(std::forward<Args>(args)...);
        mTabs[id] = TabEntry{id, title, icon, widget};
        widget->setObjectName(id);
        return widget;
    }

    /// Alle registrierten Tabs in einen QTabWidget einfügen.
    void populateTabWidget(QTabWidget *tabWidget) noexcept
    {
        if (!tabWidget) return;

        for (auto &[key, entry] : mTabs) {
            if (entry.widget) {
                tabWidget->addTab(entry.widget, entry.icon, entry.title);
            }
        }
    }

    /// Tab anhand des Keys suchen.
    [[nodiscard]] QWidget* find(const QString &id) const noexcept
    {
        auto it = mTabs.find(id);
        return (it != mTabs.end()) ? it->second.widget : nullptr;
    }

    /// Typisierten Tab-Zugriff.
    template <typename T>
    [[nodiscard]] T* findAs(const QString &id) const noexcept
    {
        return qobject_cast<T*>(find(id));
    }

    /// Alle registrierten IDs abfragen.
    [[nodiscard]] QStringList ids() const noexcept
    {
        QStringList result;
        for (const auto &[key, _] : mTabs) {
            result.append(key);
        }
        return result;
    }

    /// Anzahl registrierter Tabs.
    [[nodiscard]] std::size_t count() const noexcept
    {
        return mTabs.size();
    }

    /// Prüfen, ob ein Tab registriert ist.
    [[nodiscard]] bool contains(const QString &id) const noexcept
    {
        return mTabs.contains(id);
    }

    /// Alle Tabs entfernen (aber nicht löschen — Qt übernimmt Parent).
    void clear() noexcept
    {
        mTabs.clear();
    }

private:
    std::unordered_map<QString, TabEntry> mTabs;
};

} // namespace IPView::UI

#endif // TABREGISTRY_H
