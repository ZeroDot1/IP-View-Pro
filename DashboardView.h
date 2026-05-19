// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — DashboardView.h
//  C++26: constexpr std::array, [[nodiscard]], structured bindings
//  Central overview of current IP data (extracted from MainWindow).
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
//  DashboardView — standalone widget class for the IP overview tab.
//  Contains API selection, IP display, data table and export buttons.
//  Communicates via signals with MainWindow / NetworkManager.
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
    void setIPv6Mode(bool enabled) noexcept;
    void setAutoRefresh(bool enabled) noexcept;
    void setApiIndex(int index) noexcept;
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
    // ── UI setup ───────────────────────────────────────────────────────────
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
