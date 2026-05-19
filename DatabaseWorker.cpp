// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.4 — DatabaseWorker.cpp
//  Async DB worker — background thread for database writes (Item 14).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "DatabaseWorker.h"
#include "DatabaseModule.h"
#include "Logger.h"

#include <QMutexLocker>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Storage {

// ═══════════════════════════════════════════════════════════════════════════════
DatabaseWorker::DatabaseWorker(QObject *parent)
    : QThread(parent)
{
}

DatabaseWorker::~DatabaseWorker()
{
    shutdown();
}

void DatabaseWorker::enqueue(WriteJob job) noexcept
{
    QMutexLocker lock(&mMutex);
    mQueue.push(std::move(job));
    mPending.fetch_add(1);
    mCond.wakeOne();
}

void DatabaseWorker::shutdown() noexcept
{
    mRunning = false;
    {
        QMutexLocker lock(&mMutex);
        mCond.wakeAll();
    }
    wait(3000);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Thread-Hauptschleife
// ═══════════════════════════════════════════════════════════════════════════════

void DatabaseWorker::run()
{
    IPView::Logger::info("DatabaseWorker: Thread started");

    while (mRunning) {
        WriteJob job;

        {
            QMutexLocker lock(&mMutex);
            if (mQueue.empty()) {
                emit allJobsCompleted();
                mCond.wait(&mMutex, 1000); // 1s wakeup interval
                if (mQueue.empty()) continue;
            }
            job = mQueue.front();
            mQueue.pop();
        }

        processJob(job);
        mPending.fetch_sub(1);
    }

    IPView::Logger::info("DatabaseWorker: Thread stopped");
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Job processing (delegates to DatabaseModule)
// ═══════════════════════════════════════════════════════════════════════════════

void DatabaseWorker::processJob(const WriteJob &job) noexcept
{
    bool success = false;

    switch (job.op) {
    case WriteOp::StoreResult:
        success = DatabaseModule::storeResult(job.data);
        break;

    case WriteOp::StoreTelemetry:
        success = DatabaseModule::storeTelemetry(
            job.iface, job.rxBytes, job.txBytes, job.rxSpeed, job.txSpeed);
        break;

    case WriteOp::StoreTelemetryAggregated:
        success = DatabaseModule::storeTelemetryAggregated(
            job.iface,
            job.rxSpeed, job.txSpeed,     // avg_rx, avg_tx
            job.minRx, job.minTx,           // min_rx, min_tx
            job.maxRx, job.maxTx,           // max_rx, max_tx
            job.rxBytes, job.txBytes,       // total_rx, total_tx
            job.windowStart, job.windowEnd);
        break;

    case WriteOp::ClearHistory:
        success = DatabaseModule::clearHistory();
        break;

    case WriteOp::ClearTelemetryAggregated:
        success = DatabaseModule::clearTelemetryAggregated();
        break;

    case WriteOp::PruneHistory:
        success = DatabaseModule::pruneHistory(job.keepDays);
        break;

    case WriteOp::PruneTelemetry:
        success = DatabaseModule::pruneTelemetry(job.keepDays);
        break;
    }

    emit jobCompleted(job.op, success);
}

} // namespace IPView::Storage
