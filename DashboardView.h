// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — DashboardView.h
//  C++26: constexpr std::array, [[nodiscard]], structured bindings
//  Zentrale Übersicht der aktuellen IP-Daten (ausgelagert aus MainWindow).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef DASHBOARDVIEW_H
#define DASHBOARDVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QJsonObject>
#include <QFrame>

#include <array>     // C++26: constexpr std::array
#include <string_view>

// ═══════════════════════════════════════════════════════════════════════════════
//  DashboardView — eigenständige Widget-Klasse für den IP-Overview-Tab.
//  Enthält API-Auswahl, IP-Anzeige, Daten-Tabelle und Export-Buttons.
//  Kommuniziert via Signale mit MainWindow / NetworkManager.
// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::UI {

class DashboardView : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardView(QWidget *parent = nullptr);

    // ── Public API ──────────────────────────────────────────────────────────
    void updateDisplay(const QJsonObject &jsonData) noexcept;
    void setFlagPixmap(const QPixmap &pixmap) noexcept;
    void setApiNames(const QStringList &names) noexcept;
    void setStatusMessage(const QString &msg) noexcept;
    [[nodiscard]] QString selectedApiName() const noexcept;
    [[nodiscard]] bool     isIPv6Mode()      const noexcept;
    [[nodiscard]] QLabel*  flagLabelWidget() const noexcept;

signals:
    // ── Forwarded to MainWindow / NetworkManager ────────────────────────────
    void refreshRequested();
    void apiChanged(int index);
    void ipv6Toggled(bool enabled);
    void autoRefreshToggled(bool enabled);
    void copyAllRequested();
    void exportJsonRequested();

private:
    // ── UI-Aufbau ───────────────────────────────────────────────────────────
    void setupUI() noexcept;

    // ── Widgets (Overview Tab) ──────────────────────────────────────────────
    QComboBox   *apiCombo{nullptr};
    QCheckBox   *ipv6CheckBox{nullptr};
    QCheckBox   *autoRefreshCheckBox{nullptr};
    QPushButton *refreshButton{nullptr};

    QLabel      *ipLabel{nullptr};
    QTableWidget *ipTable{nullptr};

    QLabel      *timestampLabel{nullptr};
    QLabel      *onlineLabel{nullptr};
    QLabel      *flagLabel{nullptr};

    QPushButton *copyAllButton{nullptr};
    QPushButton *exportJsonButton{nullptr};

    // ── Daten ───────────────────────────────────────────────────────────────
    QJsonObject  currentData;
};

} // namespace IPView::UI

#endif // DASHBOARDVIEW_H
