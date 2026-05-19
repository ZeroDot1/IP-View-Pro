// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.3 — TabRegistry.h
//  C++26: std::unordered_map, std::string_view, constexpr
//  Central tab registry — tabs managed as key-value pairs.
//  Decouples MainWindow from concrete tab classes (Registry Pattern).
//  Guarantees insertion order for populateTabWidget() (Item: Tab order).
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
    QString  id;        // Unique key (e.g. "dashboard", "whois")
    QString  title;     // Display title
    QIcon    icon;      // Optional icon
    QWidget *widget{nullptr}; // Pointer to the tab widget
};

// ═══════════════════════════════════════════════════════════════════════════════
class TabRegistry
{
public:
    TabRegistry() = default;
    ~TabRegistry() = default;

    // ── Registry-Operationen ─────────────────────────────────────────────
    /// Register a tab (key must be unique).
    /// Guarantees insertion order for populateTabWidget().
    template <typename T, typename... Args>
    T* registerTab(const QString &id, const QString &title,
                   const QIcon &icon = QIcon(), Args&&... args)
    {
        static_assert(std::is_base_of_v<QWidget, T>,
                      "Tab type must inherit QWidget");

        // Already registered? → return existing tab
        if (auto it = mIndex.find(id); it != mIndex.end()) {
            return static_cast<T*>(mOrder[static_cast<qsizetype>(it->second)].widget);
        }

        T *widget = new T(std::forward<Args>(args)...);
        widget->setObjectName(id);

        mOrder.append(TabEntry{id, title, icon, widget});
        mIndex[id] = static_cast<std::size_t>(mOrder.size()) - 1;

        return widget;
    }

    /// Insert all registered tabs into a tab widget in insertion order.
    void populateTabWidget(QTabWidget *tabWidget) noexcept
    {
        if (!tabWidget) return;

        for (auto const &entry : mOrder) {
            if (entry.widget) {
                tabWidget->addTab(entry.widget, entry.icon, entry.title);
            }
        }
    }

    /// Find a tab by key.
    [[nodiscard]] QWidget* find(const QString &id) const noexcept
    {
        auto it = mIndex.find(id);
        if (it == mIndex.end()) return nullptr;
        return mOrder[static_cast<qsizetype>(it->second)].widget;
    }

    /// Typed tab access.
    template <typename T>
    [[nodiscard]] T* findAs(const QString &id) const noexcept
    {
        return qobject_cast<T*>(find(id));
    }

    /// Query all registered IDs in insertion order.
    [[nodiscard]] QStringList ids() const noexcept
    {
        QStringList result;
        for (auto const &entry : mOrder) {
            result.append(entry.id);
        }
        return result;
    }

    /// Number of registered tabs.
    [[nodiscard]] std::size_t count() const noexcept
    {
        return static_cast<std::size_t>(mOrder.size());
    }

    /// Check if a tab is registered.
    [[nodiscard]] bool contains(const QString &id) const noexcept
    {
        return mIndex.contains(id);
    }

    /// Remove all tabs (but do not delete — Qt handles parent ownership).
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
