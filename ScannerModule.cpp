// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — ScannerModule.cpp
//  C++26: structured bindings, std::array, constexpr, [[nodiscard]]
//  Async port scanner using QTcpSocket (non-blocking).
//  Batch processing with max. 100 concurrent connections.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "ScannerModule.h"

#include <QElapsedTimer>
#include <QUrl>
#include <QNetworkInformation>

#include <algorithm>
#include <array>
#include <utility>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Scanner {

// ═══════════════════════════════════════════════════════════════════════════════
ScannerModule::ScannerModule(QObject *parent)
    : QObject(parent)
{
    mBatchTimer = new QTimer(this);
    mBatchTimer->setSingleShot(true);
    connect(mBatchTimer, &QTimer::timeout, this, &ScannerModule::onBatchComplete);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════════

void ScannerModule::runScan(const QString &ip, const QVector<int> &ports) noexcept
{
    if (mScanning) {
        emit scanError(QStringLiteral("Scan already in progress"));
        return;
    }

    // Validate input: SecurityUtil policy
    mTargetIp = ip.trimmed();
    if (mTargetIp.isEmpty()) {
        emit scanError(QStringLiteral("No target IP specified"));
        return;
    }

    // Remove duplicates and sort
    QSet<int> uniquePorts(ports.begin(), ports.end());
    mPendingPorts = QVector<int>(uniquePorts.begin(), uniquePorts.end());
    std::sort(mPendingPorts.begin(), mPendingPorts.end());

    if (mPendingPorts.isEmpty()) {
        emit scanError(QStringLiteral("No ports specified"));
        return;
    }

    mResults.clear();
    mOpenPorts.clear();
    mSocketContexts.clear();
    mActiveSockets    = 0;
    mTotalPorts       = static_cast<int>(mPendingPorts.size());
    mCompletedPorts   = 0;
    mScanning         = true;
    mCancelled        = false;

    emit scanProgress(0, mTotalPorts);

        // Start first batch
    startNextBatch();
}

void ScannerModule::runScanRange(const QString &ip, int startPort, int endPort) noexcept
{
    if (startPort < 1 || endPort > 65535 || startPort > endPort) {
        emit scanError(QStringLiteral("Invalid port range: must be 1-65535"));
        return;
    }

    QVector<int> ports;
    ports.reserve(endPort - startPort + 1);
    for (int p = startPort; p <= endPort; ++p) {
        ports.append(p);
    }

    runScan(ip, ports);
}

void ScannerModule::cancelScan() noexcept
{
    if (!mScanning) return;

    mCancelled = true;
    mBatchTimer->stop();

    // Close all active sockets
    for (auto &ctx : mSocketContexts) {
        if (ctx && ctx->socket && ctx->socket->state() != QAbstractSocket::UnconnectedState) {
            ctx->socket->abort();
        }
    }
    mSocketContexts.clear();
    mActiveSockets = 0;
    mScanning      = false;

    emit scanCancelled();
}

bool ScannerModule::isScanning() const noexcept
{
    return mScanning;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private Slots
// ═══════════════════════════════════════════════════════════════════════════════

void ScannerModule::onSocketConnected()
{
    // Identify which socket connected
    auto *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    for (auto &ctx : mSocketContexts) {
        if (ctx && ctx->socket.get() == socket && !ctx->emitted) {
            ctx->emitted = true;
            ctx->timer.invalidate(); // Timeout no longer needed

            int const ms = static_cast<int>(ctx->timer.elapsed());

            ScanResult result;
            result.port      = ctx->port;
            result.open      = true;
            result.service   = getServiceName(ctx->port);
            result.latencyMs = ms;

            mResults.append(result);
            mOpenPorts.append(result);
            emit portFound(result);

            // Close socket (port scan does not require data transfer)
            socket->disconnectFromHost();
            break;
        }
    }
}

void ScannerModule::onSocketError(QAbstractSocket::SocketError /*error*/)
{
    auto *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

        // Mark socket as "not open" (only if not already reported as open)
    for (auto &ctx : mSocketContexts) {
        if (ctx && ctx->socket.get() == socket && !ctx->emitted) {
            ctx->emitted = true;

            // Add closed port to results list (only if not cancelled)
            if (!mCancelled) {
                ScanResult result;
                result.port = ctx->port;
                result.open = false;
                result.service = getServiceName(ctx->port);
                mResults.append(result);
            }
            break;
        }
    }
}

void ScannerModule::onScanTimeout()
{
        // Timeout: mark all unresponsive sockets as closed
    for (auto &ctx : mSocketContexts) {
        if (ctx && !ctx->emitted) {
            ctx->emitted = true;
            if (ctx->socket) {
                ctx->socket->abort();
            }

            if (!mCancelled) {
                ScanResult result;
                result.port = ctx->port;
                result.open = false;
                result.service = getServiceName(ctx->port);
                mResults.append(result);
            }
        }
    }
}

void ScannerModule::onBatchComplete()
{
    if (mCancelled || !mScanning) return;

        // Consolidate results from the previous batch
    mCompletedPorts += static_cast<int>(mSocketContexts.size());
    emit scanProgress(mCompletedPorts, mTotalPorts);

    mSocketContexts.clear();
    mActiveSockets = 0;

    // Start next batch or finish
    if (mPendingPorts.isEmpty()) {
        finalizeResults();
    } else {
        startNextBatch();
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private Helpers
// ═══════════════════════════════════════════════════════════════════════════════

void ScannerModule::startNextBatch() noexcept
{
    int const batchSize = std::min(static_cast<int>(MAX_CONCURRENT_SCANS),
                                   static_cast<int>(mPendingPorts.size()));

    // Timeout timer for the entire batch
    mBatchTimer->start(CONNECTION_TIMEOUT_MS + SCAN_DELAY_MS * batchSize);

    for (int i = 0; i < batchSize; ++i) {
        if (mPendingPorts.isEmpty()) break;

        int const port = mPendingPorts.takeFirst();

        auto ctx = std::make_unique<SocketContext>();
        ctx->port    = port;
        ctx->socket  = std::make_unique<QTcpSocket>(this);
        ctx->emitted = false;

        // Connect to the socket
        connect(ctx->socket.get(), &QTcpSocket::connected,
                this, &ScannerModule::onSocketConnected);
        connect(ctx->socket.get(), &QTcpSocket::errorOccurred,
                this, &ScannerModule::onSocketError);

        ctx->timer.start();
        ctx->socket->connectToHost(mTargetIp, static_cast<quint16>(port));
        ctx->socket->setProperty("scanPort", port);

        mSocketContexts.push_back(std::move(ctx));
    }

    mActiveSockets = batchSize;
}

void ScannerModule::finalizeResults() noexcept
{
    mScanning = false;

        // Sort: open ports first, then by port number
    std::sort(mResults.begin(), mResults.end(),
        [](const ScanResult &a, const ScanResult &b) {
            if (a.open != b.open) return a.open > b.open;
            return a.port < b.port;
        });

    emit scanCompleted(mResults);
}

QString ScannerModule::getServiceName(int port) noexcept
{
    // Known port services (most common)
    struct PortService { int port; const char* name; };
    static constexpr std::array services = {
        PortService{21,   "ftp"},
        PortService{22,   "ssh"},
        PortService{23,   "telnet"},
        PortService{25,   "smtp"},
        PortService{53,   "dns"},
        PortService{80,   "http"},
        PortService{110,  "pop3"},
        PortService{111,  "rpcbind"},
        PortService{135,  "epmap"},
        PortService{139,  "netbios-ssn"},
        PortService{143,  "imap"},
        PortService{443,  "https"},
        PortService{445,  "microsoft-ds"},
        PortService{993,  "imaps"},
        PortService{995,  "pop3s"},
        PortService{1433, "ms-sql-s"},
        PortService{1521, "oracle"},
        PortService{2049, "nfs"},
        PortService{3306, "mysql"},
        PortService{3389, "ms-wbt-server"},
        PortService{5432, "postgresql"},
        PortService{5900, "vnc"},
        PortService{5984, "couchdb"},
        PortService{6379, "redis"},
        PortService{8080, "http-alt"},
        PortService{8443, "https-alt"},
        PortService{9090, "cockpit"},
        PortService{27017, "mongod"}
    };

    for (auto const &svc : services) {
        if (svc.port == port) {
            return QString::fromUtf8(svc.name);
        }
    }

    return QString();
}

} // namespace IPView::Scanner
