// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.10.0 — TopologyTab.h
//  C++26: noexcept, [[nodiscard]], default member init
//  QGraphicsView network topology visualization (Item 46).
//  Displays traceroute hops as an interactive node graph.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef TOPOLOGYTAB_H
#define TOPOLOGYTAB_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsTextItem>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QProcess>
#include <QTimer>
#include <QList>
#include <QString>

#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════════
//  Hop data structure — one network hop / node in the topology
// ═══════════════════════════════════════════════════════════════════════════════
struct HopData {
    int         hopNumber{0};
    QString     ipAddress;
    QString     hostname;
    double      latencyMs{0.0};       // 0.0 = timeout / no response
    bool        isTarget{false};
};

// ═══════════════════════════════════════════════════════════════════════════════
class TopologyTab : public QWidget
{
    Q_OBJECT

public:
    explicit TopologyTab(QWidget *parent = nullptr);
    ~TopologyTab() override = default;

private slots:
    void onTraceClicked();
    void onTraceFinished(int exitCode, QProcess::ExitStatus status);
    void onTraceReadyRead();
    void onTraceErrorOccurred(QProcess::ProcessError error);
    void onClearClicked();

private:
    void setupUI();
    void buildTopology();
    void addNode(const HopData &hop, int index, int total);
    void clearScene();

    // ── UI elements ───────────────────────────────────────────────────────
    QLineEdit       *hostInput{nullptr};
    QPushButton     *traceButton{nullptr};
    QPushButton     *clearButton{nullptr};
    QLabel          *statusLabel{nullptr};
    QGraphicsView   *view{nullptr};
    QGraphicsScene  *scene{nullptr};

    // ── Trace process ────────────────────────────────────────────────────
    QProcess        *mProcess{nullptr};
    QByteArray       mBuffer;

    // ── Data ──────────────────────────────────────────────────────────────
    QList<HopData>   mHops;
    QString          mTargetHost;
};

#endif // TOPOLOGYTAB_H
