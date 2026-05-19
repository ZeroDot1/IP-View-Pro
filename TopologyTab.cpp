// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.10.0 — TopologyTab.cpp
//  C++26: QStringLiteral, auto, structured bindings, [[maybe_unused]]
//  QGraphicsView network topology visualization (Item 46).
//  Parses traceroute output and renders hops as an interactive node graph.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TopologyTab.h"
#include "Theme.h"
#include "SecurityUtil.h"

#include <QGraphicsEllipseItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QDateTime>
#include <QHeaderView>
#include <QMessageBox>
#include <cmath>

// ═══════════════════════════════════════════════════════════════════════════════
//  Helpers
// ═══════════════════════════════════════════════════════════════════════════════

/// Parse a single traceroute line like:
///   " 1  192.168.1.1 [2.345 ms]   gateway.local"
///   " 2  * * *"
///   " 3  10.0.0.1  12.345 ms   router.example.com"
[[nodiscard]]
static std::optional<HopData> parseTraceLine(const QString &line, int lineIndex)
{
    QString const trimmed = line.trimmed();
    if (trimmed.isEmpty()) return std::nullopt;

    // Skip header lines
    if (trimmed.startsWith(QStringLiteral("traceroute"), Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("tracert"),    Qt::CaseInsensitive) ||
        trimmed.startsWith(QStringLiteral("ping"),       Qt::CaseInsensitive))
    {
        return std::nullopt;
    }

    // Skip separator lines
    if (trimmed.startsWith(QLatin1Char('*')) && trimmed.count(QLatin1Char('*')) > 1)
        return std::nullopt;

    HopData hop;
    hop.hopNumber = lineIndex;

    // Check for timeout (all asterisks)
    if (trimmed.count(QLatin1Char('*')) >= 3) {
        // Timed out hop
        return hop;
    }

    // Pattern: "N  IP  latency_ms  hostname"
    // Split by spaces
    QStringList const parts = trimmed.split(QRegularExpression(QStringLiteral("\\s+")),
                                            Qt::SkipEmptyParts);

    // First token should be the hop number (skip it)
    int idx = 0;
    if (idx < parts.size()) {
        bool ok = false;
        (void)parts[idx].toInt(&ok);
        if (ok) ++idx;  // skip hop number
    }

    // Next token is IP address or hostname
    QString ip;
    if (idx < parts.size()) {
        ip = parts[idx];
        // Check if it looks like an IP
        if (ip.contains(QLatin1Char('.')) || ip.contains(QLatin1Char(':'))) {
            hop.ipAddress = ip;
            ++idx;
        } else if (ip.startsWith(QLatin1Char('*'))) {
            // Timeout
            return hop;
        } else {
            // Might be a hostname without IP
            hop.hostname = ip;
            ++idx;
        }
    }

    // Look for latency value (contains "ms")
    for (; idx < parts.size(); ++idx) {
        QString const &p = parts[idx];
        if (p.contains(QStringLiteral("ms"), Qt::CaseInsensitive)) {
            QString numStr = p;
            numStr.remove(QStringLiteral("ms"), Qt::CaseInsensitive);
            numStr = numStr.trimmed();
            bool ok = false;
            double val = numStr.toDouble(&ok);
            if (ok) {
                hop.latencyMs = val;
            }
            ++idx;
            break;
        }
    }

    // Remaining tokens form the hostname
    if (idx < parts.size() && hop.hostname.isEmpty()) {
        hop.hostname = parts.mid(idx).join(QLatin1Char(' '));
    }

    // If we have an IP but no hostname, use IP as hostname
    if (hop.hostname.isEmpty() && !hop.ipAddress.isEmpty()) {
        hop.hostname = hop.ipAddress;
    }

    return hop;
}

// ═══════════════════════════════════════════════════════════════════════════════
TopologyTab::TopologyTab(QWidget *parent)
    : QWidget(parent)
{
    setupUI();

    mProcess = new QProcess(this);
    connect(mProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TopologyTab::onTraceFinished);
    connect(mProcess, &QProcess::readyReadStandardOutput,
            this, &TopologyTab::onTraceReadyRead);
    connect(mProcess, &QProcess::errorOccurred,
            this, &TopologyTab::onTraceErrorOccurred);
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // ── Top: Input row ────────────────────────────────────────────────────
    auto *inputRow = new QHBoxLayout();

    auto *hostLabel = new QLabel(QStringLiteral("Target:"));
    hostInput = new QLineEdit();
    hostInput->setPlaceholderText(QStringLiteral("Hostname or IP (e.g. google.com)"));
    hostInput->setStyleSheet(inputStyle());

    traceButton = new QPushButton(QStringLiteral(" Trace Route"));
    traceButton->setProperty("accent", true);
    traceButton->setStyleSheet(btnAccentStyle());

    clearButton = new QPushButton(QStringLiteral(" Clear"));
    clearButton->setStyleSheet(btnSecondaryStyle());

    inputRow->addWidget(hostLabel);
    inputRow->addWidget(hostInput, 1);
    inputRow->addWidget(traceButton);
    inputRow->addWidget(clearButton);

    mainLayout->addLayout(inputRow);

    // ── Status ────────────────────────────────────────────────────────────
    statusLabel = new QLabel(QStringLiteral("Enter a target host and click Trace Route"));
    statusLabel->setStyleSheet(statusLabelStyle());
    mainLayout->addWidget(statusLabel);

    // ── Graphics View ─────────────────────────────────────────────────────
    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing, true);
    view->setRenderHint(QPainter::SmoothPixmapTransform, true);
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    view->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    view->setStyleSheet(QStringLiteral(
        "QGraphicsView { background-color: %1; border: 1px solid %2;"
        "  border-radius: %3; }"
    ).arg(C_BG_SUNKEN, C_BORDER, RADIUS_LG));
    view->setMinimumHeight(300);

    // Interactive zoom via scroll wheel
    view->setInteractive(true);
    view->setOptimizationFlag(QGraphicsView::DontAdjustForAntialiasing, true);

    mainLayout->addWidget(view, 1);

    // ── Connections ───────────────────────────────────────────────────────
    connect(traceButton, &QPushButton::clicked, this, &TopologyTab::onTraceClicked);
    connect(clearButton, &QPushButton::clicked, this, &TopologyTab::onClearClicked);
    connect(hostInput, &QLineEdit::returnPressed, this, &TopologyTab::onTraceClicked);

    // ── Wheel zoom ────────────────────────────────────────────────────────
    view->viewport()->installEventFilter(this);

    // Set dark background
    scene->setBackgroundBrush(QBrush(QColor(C_BG_SUNKEN)));
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::onTraceClicked()
{
    QString const input = hostInput->text().trimmed();
    if (input.isEmpty()) return;

    if (!isValidNetworkTarget(input)) {
        QMessageBox::warning(this,
            QStringLiteral("Invalid Target"),
            QStringLiteral("Please enter a valid IP address or hostname.\n"
                           "Shell metacharacters are not allowed."));
        return;
    }

    mTargetHost = input;
    clearScene();
    mHops.clear();
    mBuffer.clear();

    statusLabel->setText(QStringLiteral("Tracing route to %1 ...").arg(mTargetHost));
    traceButton->setEnabled(false);

    // Find traceroute binary
    QString const traceroute = findSystemTool(QStringLiteral("traceroute"));
    if (traceroute.isEmpty()) {
        statusLabel->setText(QStringLiteral("Error: traceroute not found. Install with: sudo pacman -S traceroute"));
        traceButton->setEnabled(true);
        return;
    }

    mProcess->start(traceroute, {
        QStringLiteral("-n"),        // numeric output (no DNS lookups — faster)
        QStringLiteral("-q"), QStringLiteral("1"),  // single query per hop
        QStringLiteral("-w"), QStringLiteral("3"),  // 3 second timeout
        QStringLiteral("-m"), QStringLiteral("30"), // max 30 hops
        mTargetHost
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::onTraceReadyRead()
{
    mBuffer.append(mProcess->readAllStandardOutput());
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::onTraceFinished(int exitCode, [[maybe_unused]] QProcess::ExitStatus status)
{
    traceButton->setEnabled(true);

    // Also read any remaining stderr
    mBuffer.append(mProcess->readAllStandardError());

    QString const output = QString::fromUtf8(mBuffer);

    // Parse output line by line
    QStringList const lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    int hopIndex = 1;
    for (QString const &line : lines) {
        auto hop = parseTraceLine(line, hopIndex);
        if (hop.has_value()) {
            if (hop->hopNumber >= hopIndex) {
                // Set isTarget on the last hop
                mHops.append(*hop);
                ++hopIndex;
            }
        }
    }

    if (!mHops.isEmpty()) {
        mHops.last().isTarget = true;
    }

    // ── Build the visual topology ─────────────────────────────────────────
    buildTopology();

    statusLabel->setText(QStringLiteral("Trace complete: %1 hops to %2")
                         .arg(mHops.size())
                         .arg(mTargetHost));

    if (exitCode != 0 && mHops.isEmpty()) {
        statusLabel->setText(QStringLiteral("Trace failed (exit code %1). "
                                            "Try installing traceroute: sudo pacman -S traceroute")
                             .arg(exitCode));
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::onTraceErrorOccurred(QProcess::ProcessError error)
{
    traceButton->setEnabled(true);
    QString msg;
    switch (error) {
        case QProcess::FailedToStart:
            msg = QStringLiteral("traceroute not found. Install: sudo pacman -S traceroute");
            break;
        case QProcess::Timedout:
            msg = QStringLiteral("Traceroute timed out.");
            break;
        case QProcess::Crashed:
            msg = QStringLiteral("Traceroute process crashed.");
            break;
        default:
            msg = QStringLiteral("An unknown error occurred.");
            break;
    }
    statusLabel->setText(QStringLiteral("Error: %1").arg(msg));
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::onClearClicked()
{
    mProcess->kill();
    clearScene();
    mHops.clear();
    mBuffer.clear();
    hostInput->clear();
    statusLabel->setText(QStringLiteral("Ready. Enter a target host and click Trace Route"));
    traceButton->setEnabled(true);
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::clearScene()
{
    scene->clear();
    // Re-set background after clear (clear removes it)
    scene->setBackgroundBrush(QBrush(QColor(C_BG_SUNKEN)));
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::buildTopology()
{
    clearScene();
    if (mHops.isEmpty()) return;

    auto const total = static_cast<int>(mHops.size());
    for (int i = 0; i < total; ++i) {
        addNode(mHops[i], i, total);
    }

    // Fit the scene content
    view->fitInView(scene->itemsBoundingRect().adjusted(-40, -40, 40, 40),
                    Qt::KeepAspectRatio);
}

// ═══════════════════════════════════════════════════════════════════════════════
void TopologyTab::addNode(const HopData &hop, int index, int total)
{
    Q_UNUSED(total)

    // ── Layout: left-to-right chain ──────────────────────────────────────
    qreal const x = 120.0 + static_cast<qreal>(index) * 220.0;
    qreal const y = 250.0;

    // ── Determine node color ──────────────────────────────────────────────
    QColor nodeColor;
    qreal nodeRadius = 18.0;

    if (hop.isTarget) {
        nodeColor = QColor(C_SUCCESS);       // green = destination
        nodeRadius = 24.0;
    } else if (hop.latencyMs <= 0.0) {
        nodeColor = QColor(C_ERROR);         // red = timeout
        nodeRadius = 14.0;
    } else if (hop.latencyMs < 10.0) {
        nodeColor = QColor(C_SUCCESS);       // green = fast
    } else if (hop.latencyMs < 50.0) {
        nodeColor = QColor(C_PRIMARY);       // blue = moderate
    } else if (hop.latencyMs < 150.0) {
        nodeColor = QColor(C_WARNING);       // orange = slow
    } else {
        nodeColor = QColor(C_ERROR);         // red = very slow
    }

    // ── Node circle ───────────────────────────────────────────────────────
    auto *ellipse = scene->addEllipse(
        x - nodeRadius, y - nodeRadius,
        nodeRadius * 2.0, nodeRadius * 2.0,
        QPen(QColor(C_TEXT), 2.0),
        QBrush(nodeColor));

    // Tooltip with full details
    QString tip;
    tip += QStringLiteral("Hop %1").arg(hop.hopNumber);
    if (!hop.ipAddress.isEmpty())
        tip += QStringLiteral("\nIP: %1").arg(hop.ipAddress);
    if (!hop.hostname.isEmpty() && hop.hostname != hop.ipAddress)
        tip += QStringLiteral("\nHost: %1").arg(hop.hostname);
    if (hop.latencyMs > 0.0)
        tip += QStringLiteral("\nLatency: %1 ms").arg(hop.latencyMs, 0, 'f', 2);
    else
        tip += QStringLiteral("\nTimeout");
    if (hop.isTarget)
        tip += QStringLiteral("\n(Destination)");
    ellipse->setToolTip(tip);

    // ── Hop number label (above node) ─────────────────────────────────────
    auto *numLabel = scene->addText(QString::number(hop.hopNumber), QFont(QStringLiteral("monospace"), 10, QFont::Bold));
    numLabel->setDefaultTextColor(QColor(C_TEXT));
    numLabel->setPos(x - numLabel->boundingRect().width() / 2.0,
                     y - nodeRadius - 22.0);

    // ── IP / hostname label (below node) ──────────────────────────────────
    QString const displayName = hop.ipAddress.isEmpty() ? hop.hostname : hop.ipAddress;
    QString shortName = displayName;
    if (shortName.length() > 16)
        shortName = shortName.left(16) + QStringLiteral("...");
    auto *nameLabel = scene->addText(shortName, QFont(QStringLiteral("monospace"), 8));
    nameLabel->setDefaultTextColor(QColor(C_TEXT_SEC));
    nameLabel->setPos(x - nameLabel->boundingRect().width() / 2.0,
                      y + nodeRadius + 4.0);

    // ── Latency label (below IP) ─────────────────────────────────────────
    QString latencyStr;
    if (hop.latencyMs > 0.0) {
        latencyStr = QStringLiteral("%1 ms").arg(hop.latencyMs, 0, 'f', 1);
    } else {
        latencyStr = QStringLiteral("* * *");
    }
    auto *latLabel = scene->addText(latencyStr, QFont(QStringLiteral("monospace"), 8));
    latLabel->setDefaultTextColor(QColor(C_TEXT_DIM));
    latLabel->setPos(x - latLabel->boundingRect().width() / 2.0,
                     y + nodeRadius + 18.0);

    // ── Connecting line to previous node ──────────────────────────────────
    if (index > 0) {
        qreal const prevX = 120.0 + static_cast<qreal>(index - 1) * 220.0;
        scene->addLine(
            prevX + 18.0, y, x - nodeRadius, y,
            QPen(QColor(C_BORDER), 2.0, Qt::DashLine));

        // Arrow marker (small triangle)
        QPolygonF arrow;
        arrow << QPointF(x - nodeRadius - 8.0, y - 5.0)
              << QPointF(x - nodeRadius - 8.0, y + 5.0)
              << QPointF(x - nodeRadius, y);
        scene->addPolygon(arrow, QPen(Qt::NoPen), QBrush(QColor(C_TEXT_DIM)));
    }
}
