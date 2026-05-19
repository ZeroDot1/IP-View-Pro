// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.4 — DatabaseWorker.h
//  C++26: std::jthread, std::stop_token, std::queue
//  Async DB worker: write operations are queued
//  and processed sequentially in a background thread (Item 14).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef DATABASEWORKER_H
#define DATABASEWORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QString>
#include <QJsonObject>
#include <QDateTime>

#include <queue>
#include <functional>
#include <atomic>
#include <optional>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Storage {

// ── Job types ───────────────────────────────────────────────────────────
enum class WriteOp {
    StoreResult,
    StoreTelemetry,
    StoreTelemetryAggregated,
    ClearHistory,
    ClearTelemetryAggregated,
    PruneHistory,
    PruneTelemetry
};

// ── A job in the write queue ─────────────────────────────────────────
struct WriteJob {
    WriteOp op;
    QJsonObject    data;        // für storeResult
    QString        iface;       // für Telemetry
    quint64        rxBytes{0};
    quint64        txBytes{0};
    double         rxSpeed{0.0};
    double         txSpeed{0.0};
    double         minRx{0.0}, minTx{0.0};
    double         maxRx{0.0}, maxTx{0.0};
    QDateTime      windowStart;
    QDateTime      windowEnd;
    int            keepDays{30};
};

// ═══════════════════════════════════════════════════════════════════════════════
class DatabaseWorker : public QThread
{
    Q_OBJECT

public:
    explicit DatabaseWorker(QObject *parent = nullptr);
    ~DatabaseWorker() override;

    /// Enqueue a write job (processed asynchronously).
    void enqueue(WriteJob job) noexcept;

    /// Clear the queue and stop the worker.
    void shutdown() noexcept;

    /// Anzahl wartender Jobs.
    [[nodiscard]] int pendingJobs() const noexcept { return mPending.load(); }

signals:
    void jobCompleted(WriteOp op, bool success);
    void allJobsCompleted();

protected:
    void run() override;

private:
    void processJob(const WriteJob &job) noexcept;

    // ── Synchronization ──────────────────────────────────────────────────
    mutable QMutex         mMutex;
    QWaitCondition         mCond;
    std::queue<WriteJob>   mQueue;
    std::atomic<bool>      mRunning{true};
    std::atomic<int>       mPending{0};
};

} // namespace IPView::Storage

#endif // DATABASEWORKER_H
