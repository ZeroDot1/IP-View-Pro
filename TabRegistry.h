// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.3 — TabRegistry.h
//  C++26: std::unordered_map, std::string_view, constexpr
//  Zentrale Tab-Registrierung — Tabs werden als Key-Value-Paare verwaltet.
//  Entkoppelt MainWindow von konkreten Tab-Klassen (Registry Pattern).
//  Garantiert Insertion-Order für populateTabWidget() (Item: Tab-Reihenfolge).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef TABREGISTRY_H
#define TABREGISTRY_H

#include <QWidget>
#include <QTabWidget>
#include <QIcon>
#include <QString>
#include <QStringList>
#include <QList>

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
    /// Garantiert Insertion-Order für populateTabWidget().
    template <typename T, typename... Args>
    T* registerTab(const QString &id, const QString &title,
                   const QIcon &icon = QIcon(), Args&&... args)
    {
        static_assert(std::is_base_of_v<QWidget, T>,
                      "Tab type must inherit QWidget");

        // Bereits registriert? → existierenden Tab zurückgeben
        if (auto it = mIndex.find(id); it != mIndex.end()) {
            return static_cast<T*>(mOrder[static_cast<qsizetype>(it->second)].widget);
        }

        T *widget = new T(std::forward<Args>(args)...);
        widget->setObjectName(id);

        mOrder.append(TabEntry{id, title, icon, widget});
        mIndex[id] = static_cast<std::size_t>(mOrder.size()) - 1;

        return widget;
    }

    /// Alle registrierten Tabs in der Insertion-Reihenfolge einfügen.
    void populateTabWidget(QTabWidget *tabWidget) noexcept
    {
        if (!tabWidget) return;

        for (auto const &entry : mOrder) {
            if (entry.widget) {
                tabWidget->addTab(entry.widget, entry.icon, entry.title);
            }
        }
    }

    /// Tab anhand des Keys suchen.
    [[nodiscard]] QWidget* find(const QString &id) const noexcept
    {
        auto it = mIndex.find(id);
        if (it == mIndex.end()) return nullptr;
        return mOrder[static_cast<qsizetype>(it->second)].widget;
    }

    /// Typisierten Tab-Zugriff.
    template <typename T>
    [[nodiscard]] T* findAs(const QString &id) const noexcept
    {
        return qobject_cast<T*>(find(id));
    }

    /// Alle registrierten IDs in der Insertion-Reihenfolge abfragen.
    [[nodiscard]] QStringList ids() const noexcept
    {
        QStringList result;
        for (auto const &entry : mOrder) {
            result.append(entry.id);
        }
        return result;
    }

    /// Anzahl registrierter Tabs.
    [[nodiscard]] std::size_t count() const noexcept
    {
        return static_cast<std::size_t>(mOrder.size());
    }

    /// Prüfen, ob ein Tab registriert ist.
    [[nodiscard]] bool contains(const QString &id) const noexcept
    {
        return mIndex.contains(id);
    }

    /// Alle Tabs entfernen (aber nicht löschen — Qt übernimmt Parent).
    void clear() noexcept
    {
        mOrder.clear();
        mIndex.clear();
    }

private:
    QList<TabEntry>                         mOrder;   // Insertion-Order
    std::unordered_map<QString, std::size_t> mIndex;   // id → Index in mOrder
};

} // namespace IPView::UI

#endif // TABREGISTRY_H
