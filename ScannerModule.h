// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — ScannerModule.h
//  C++26: [[nodiscard]], noexcept, const-correctness
//  Asynchroner Port-Scanner mit QTcpSocket (non-blocking).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef SCANNERMODULE_H
#define SCANNERMODULE_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QVector>
#include <QSet>
#include <QString>

#include <memory>   // std::unique_ptr

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Scanner {

// ── Ergebnis eines einzelnen Port-Scans ──────────────────────────────────────
struct ScanResult {
    int      port{0};
    bool     open{false};
    QString  service;     // Bekannter Service-Name (z. B. "http", "ssh")
    int      latencyMs{0};
};

// ═══════════════════════════════════════════════════════════════════════════════
class ScannerModule : public QObject
{
    Q_OBJECT

public:
    explicit ScannerModule(QObject *parent = nullptr);
    ~ScannerModule() override = default;

    // ── Public API ──────────────────────────────────────────────────────────
    void      runScan(const QString &ip, const QVector<int> &ports) noexcept;
    void      runScanRange(const QString &ip, int startPort, int endPort) noexcept;
    void      cancelScan() noexcept;
    [[nodiscard]] bool isScanning() const noexcept;

    // ── Konstanten ─────────────────────────────────────────────────────────
    static constexpr int CONNECTION_TIMEOUT_MS = 500;   // Timeout pro Port
    static constexpr int MAX_CONCURRENT_SCANS   = 100;  // Gleichzeitige Scans
    static constexpr int SCAN_DELAY_MS          = 10;   // Verzögerung zwischen Batches

    // Bekannte Ports (Auswahl)
    static constexpr std::array KNOWN_PORTS = {
        21, 22, 23, 25, 53, 80, 110, 111, 135, 139,
        143, 443, 445, 993, 995, 1433, 1521, 2049, 3306, 3389,
        5432, 5900, 5984, 6379, 8080, 8443, 9090, 27017
    };

    [[nodiscard]] static QVector<int> defaultPorts() noexcept
    {
        return QVector<int>(KNOWN_PORTS.begin(), KNOWN_PORTS.end());
    }

signals:
    // Wird emittiert, sobald ein Port gefunden wurde
    void portFound(const IPView::Scanner::ScanResult &result);
    // Wird emittiert, wenn ein Scan-Durchlauf abgeschlossen ist
    void scanCompleted(const QVector<IPView::Scanner::ScanResult> &results);
    // Wird emittiert bei Fehlern
    void scanError(const QString &message);
    // Fortschritt (currentIndex, totalCount)
    void scanProgress(int current, int total);
    // Wird emittiert, wenn der Scan abgebrochen wurde
    void scanCancelled();

private slots:
    void onSocketConnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onScanTimeout();
    void onBatchComplete();

private:
    void startNextBatch() noexcept;
    void finalizeResults() noexcept;
    [[nodiscard]] static QString getServiceName(int port) noexcept;

    // ── State ────────────────────────────────────────────────────────────────
    QString              mTargetIp;
    QVector<int>         mPendingPorts;
    QVector<ScanResult>  mResults;
    QVector<ScanResult>  mOpenPorts;       // Nur offene Ports (für portFound-Signal)

    // ── Concurrency-Management ──────────────────────────────────────────────
    int                   mActiveSockets{0};
    int                   mTotalPorts{0};
    int                   mCompletedPorts{0};
    bool                  mScanning{false};
    bool                  mCancelled{false};
    QTimer               *mBatchTimer{nullptr};

    // ── Socket-Pool ─────────────────────────────────────────────────────────
    struct SocketContext {
        std::unique_ptr<QTcpSocket> socket;
        int                         port;
        QElapsedTimer               timer;
        bool                        emitted{false};
    };
    std::vector<std::unique_ptr<SocketContext>> mSocketContexts;
};

} // namespace IPView::Scanner

#endif // SCANNERMODULE_H
