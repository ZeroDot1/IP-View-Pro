// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — NetworkDiscovery.h
//  C++26: [[nodiscard]], noexcept, structured bindings, std::optional
//  Discovers devices on a local IPv4 subnet via parallel ICMP ping + ARP
//  table cross-reference + reverse-DNS hostname resolution.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef NETWORKDISCOVERY_H
#define NETWORKDISCOVERY_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QProcess>
#include <QElapsedTimer>
#include <QHostInfo>

#include <array>
#include <atomic>
#include <chrono>
#include <optional>

class QTimer;  // forward decl — full def in NetworkDiscovery.cpp

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Scanner {

// ── A single device found on the network ───────────────────────────────────
struct DiscoveredDevice {
    QString ip;          // "192.168.1.42"
    QString hostname;    // "my-laptop" (empty if reverse-DNS failed)
    QString mac;         // "aa:bb:cc:dd:ee:ff" (empty if not in ARP cache)
    QString vendor;      // "Apple" (empty if OUI unknown)
    int     latencyMs{0}; // ICMP round-trip in ms (0 if not pinged)
};

// ═══════════════════════════════════════════════════════════════════════════════
//  NetworkDiscovery
//
//  Drives an asynchronous ping sweep over an IPv4 /24 subnet (or any
//  user-supplied prefix). A bounded pool of QProcess workers runs
//  `ping -c 1 -W 1` in parallel; each responding host triggers a
//  reverse-DNS lookup and an ARP-table cross-reference for the MAC
//  address. The class emits one `deviceFound()` signal per live host
//  and a final `completed()` when the sweep is done.
//
//  Subnet format accepted by startDiscovery():
//    * "192.168.1"        — /24 sweep over .1 .. .254
//    * "192.168.1.0/24"   — explicit CIDR
//    * ""                 — auto-detect first non-loopback IPv4 /24
//
//  Cancelled runs do not emit `completed()`.
class NetworkDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit NetworkDiscovery(QObject *parent = nullptr);
    ~NetworkDiscovery() override;

    // ── Public API ──────────────────────────────────────────────────────────
    void startDiscovery(const QString &subnet = QString()) noexcept;
    void cancel() noexcept;

    [[nodiscard]] bool isRunning() const noexcept { return mRunning.load(); }

    // ── Static helpers (exposed for unit testing) ──────────────────────────
    [[nodiscard]] static QStringList detectLocalSubnets() noexcept;

    struct SubnetRange {
        QString base;       // "192.168.1" — caller appends ".N"
        int     firstHost{1};
        int     lastHost{254};
    };
    [[nodiscard]] static std::optional<SubnetRange>
    parseSubnet(const QString &subnet) noexcept;

    // ── Constants ──────────────────────────────────────────────────────────
    static constexpr int   MAX_PARALLEL_PINGS = 20;
    static constexpr int   PING_TIMEOUT_MS    = 1000;
    static constexpr std::chrono::milliseconds
                             DISPATCH_INTERVAL{50};

signals:
    void deviceFound(const IPView::Scanner::DiscoveredDevice &device);
    void progress(int scanned, int total);
    void completed();
    void error(const QString &message);
    void cancelled();

private:
    // ── Per-process worker state ───────────────────────────────────────────
    struct PingWorker {
        QProcess     *process{nullptr};
        QString       ip;
        QElapsedTimer timer;
    };

    [[nodiscard]] static QStringList buildCandidateIps(
        const SubnetRange &range) noexcept;

    [[nodiscard]] static QString lookupMac(const QString &ip) noexcept;
    [[nodiscard]] static QString lookupVendor(const QString &mac) noexcept;

    // Start one new QProcess. Returns true if dispatched, false
    // if the queue is empty or the pool is full.
    bool dispatchNext() noexcept;

    // QProcess::finished slot. Looks up the worker that owns the
    // sender, processes the result (success → reverse-DNS, then
    // emit deviceFound), and updates progress.
    void onProcessFinished(int exitCode);

    // Dispatch timer tick: catch workers that finished without
    // emitting `finished` (rare, but possible on QProcess teardown
    // races) and refill the pool.
    void onDispatchTick() noexcept;

    // Reverse-DNS completion slot.
    void onHostInfoReady(const QHostInfo &host) noexcept;

    // ── State ──────────────────────────────────────────────────────────────
    QList<PingWorker>      mActiveWorkers;
    QStringList            mPendingIps;
    int                    mScannedCount{0};
    int                    mTotalCount{0};
    std::atomic<bool>      mRunning{false};
    std::atomic<bool>      mCancelRequested{false};

    // Pending reverse-DNS state. The lookupHost() callback is a
    // static function pointer, so we cannot pass the per-call
    // context through a lambda. We serialise the lookups: at most
    // one in flight at a time, queued in mPendingReverseLookups.
    struct PendingLookup {
        QString ip;
        int     latencyMs{0};
    };
    QList<PendingLookup>   mPendingReverseLookups;
    QString                mInFlightLookupIp;
    int                    mInFlightLookupLatency{0};

    QTimer                *mDispatchTimer{nullptr};
};

} // namespace IPView::Scanner

#endif // NETWORKDISCOVERY_H
