// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — TelemetryModule.cpp
//  C++26: std::expected, std::string_view, structured bindings
//  Reads /proc/net/dev and calculates real-time transfer rates.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TelemetryModule.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#include <algorithm>
#include <charconv>     // C++26: std::from_chars
#include <system_error>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Telemetry {

// ── Konstruktor ─────────────────────────────────────────────────────────────
TelemetryModule::TelemetryModule(QObject *parent)
    : QObject(parent)
{
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &TelemetryModule::onTick);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Public API
// ═══════════════════════════════════════════════════════════════════════════════

std::expected<Stats, std::string>
TelemetryModule::fetchStats(std::string_view interface) noexcept
{
    QFile file(QStringLiteral("/proc/net/dev"));
    if (!file.open(QIODevice::ReadOnly)) {
        return std::unexpected(
            std::string("Cannot open /proc/net/dev: ") + file.errorString().toStdString());
    }

    QByteArray const raw = file.readAll();
    file.close();

    std::string_view const buf(raw.constData(),
                               static_cast<std::size_t>(raw.size()));
    return parseProcNetDev(buf, interface);
}

QStringList TelemetryModule::availableInterfaces() const noexcept
{
    if (mCacheValid && !mCachedInterfaces.isEmpty()) {
        return mCachedInterfaces;
    }

    QStringList result;
    QFile file(QStringLiteral("/proc/net/dev"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result;
    }

    QTextStream in(&file);
    // Skip header lines (Inter-|   Receive-...)
    in.readLine(); // first header line
    in.readLine(); // second header line

    static QRegularExpression const re(QStringLiteral(R"(^\s*([^:]+):)"));

    while (!in.atEnd()) {
        QString const line = in.readLine();
        auto const match = re.match(line);
        if (match.hasMatch()) {
            QString const ifName = match.captured(1).trimmed();
            // Filter loopback and virtual interfaces
            if (isValidInterface(ifName.toStdString())) {
                result.append(ifName);
            }
        }
    }

    file.close();

    mCachedInterfaces = result;
    mCacheValid = true;
    return result;
}

void TelemetryModule::startMonitoring(int intervalMs) noexcept
{
    if (mMonitoring) return;

    // Capture initial measurement values
    onTick();
    mTimer->start(intervalMs);
    mMonitoring = true;
}

void TelemetryModule::stopMonitoring() noexcept
{
    if (!mMonitoring) return;
    mTimer->stop();
    mMonitoring = false;
}

bool TelemetryModule::isMonitoring() const noexcept
{
    return mMonitoring;
}

QList<InterfaceInfo> TelemetryModule::getAllInterfaces() const noexcept
{
    return mInterfaces;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private Slots
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryModule::onTick() noexcept
{
    QFile file(QStringLiteral("/proc/net/dev"));
    if (!file.open(QIODevice::ReadOnly)) {   // ← binary mode, kein QTextStream
        emit errorOccurred(QStringLiteral("Cannot open /proc/net/dev"));
        return;
    }

    QByteArray const raw = file.readAll();   // ← QByteArray statt QString
    file.close();

    // ── Zero-Copy: std::string_view in den Roh-Puffer (Item 40) ──────────
    std::string_view const buf(raw.constData(),
                               static_cast<std::size_t>(raw.size()));

    QStringList const ifaces = availableInterfaces();
    QList<InterfaceInfo> updated;
    updated.reserve(ifaces.size());

    for (QString const &iface : ifaces) {
        std::string const name = iface.toStdString();
        Stats const current = parseProcNetDev(buf, name);

        // Find previous entry for speed calculation
        auto it = std::find_if(mInterfaces.begin(), mInterfaces.end(),
            [&](const InterfaceInfo &info) { return info.name == iface; });

        double rxSpeed = 0.0;
        double txSpeed = 0.0;

        if (it != mInterfaces.end()) {
            // Calculate speed (bytes per second)
            // Timer-Intervall in ms -> Faktor 1000/interval
            double const factor = 1000.0 / static_cast<double>(mTimer->interval());

            rxSpeed = static_cast<double>(current.rxBytes - it->current.rxBytes) * factor;
            txSpeed = static_cast<double>(current.txBytes - it->current.txBytes) * factor;

            it->previous = it->current;
            it->current  = current;
            it->rxSpeedBps = rxSpeed;
            it->txSpeedBps = txSpeed;
            updated.append(*it);
        } else {
            // New entry (no speed available on first run)
            InterfaceInfo info;
            info.name    = iface;
            info.current = current;
            updated.append(info);
        }
    }

    mInterfaces = updated;
    mCacheValid = false; // Reload cache on next call

    // ── Dynamic interval adjustment (Item 41) ──────────────────────────
    if (mDynamicAdjustment) {
        double totalActivity = 0.0;
        for (auto const &info : mInterfaces) {
            totalActivity += info.rxSpeedBps + info.txSpeedBps;
        }
        adjustIntervalDynamically(totalActivity);
    }

    emit telemetryUpdated(mInterfaces);
}

void TelemetryModule::adjustIntervalDynamically(double totalActivity) noexcept
{
    // totalActivity in bytes/sec — skaliere Intervall invers dazu
    //   activity <   50 KB/s  →  max interval (10 s)
    //   activity >   10 MB/s  →  min interval (500 ms)
    //   dazwischen → linear interpoliert
    double constexpr lowWater  =     50.0 * 1024.0;  //   50 KB/s
    double constexpr highWater =     10.0 * 1024.0 * 1024.0;  // 10 MB/s
    int const targetMs = static_cast<int>(
        std::clamp(
            mMaxIntervalMs - (totalActivity - lowWater)
                          / (highWater - lowWater)
                          * static_cast<double>(mMaxIntervalMs - mMinIntervalMs),
            static_cast<double>(mMinIntervalMs),
            static_cast<double>(mMaxIntervalMs)
        ));

    if (totalActivity <= lowWater) {
        // Sehr geringe Aktivität → langsamstes Intervall
        mTimer->setInterval(mMaxIntervalMs);
    } else if (totalActivity >= highWater) {
        // Hohe Aktivität → schnellstes Intervall
        mTimer->setInterval(mMinIntervalMs);
    } else {
        mTimer->setInterval(targetMs);
    }
}


// ═══════════════════════════════════════════════════════════════════════════════
//  Private Helpers
// ═══════════════════════════════════════════════════════════════════════════════

Stats TelemetryModule::parseProcNetDev(std::string_view buf,
                                       std::string_view interface) noexcept
{
    Stats stats{};

    // /proc/net/dev Format:
    // Inter-|   Receive                              |  Transmit
    //  face |bytes    packets errs drop fifo ...     |bytes    packets errs ...
    //    lo: 12345    678    0    0    0 ...         98765    432    0 ...

    // ── Zero-Copy: Zeilen-weise aus std::string_view (Item 40) ───────────
    std::size_t pos = 0;
    while (pos < buf.size()) {
        std::size_t const nl = buf.find('\n', pos);
        std::size_t const end = (nl == std::string_view::npos) ? buf.size() : nl;
        std::string_view const line = buf.substr(pos, end - pos);
        pos = (nl == std::string_view::npos) ? buf.size() : nl + 1;

        if (line.empty()) continue;

        // Interface-Namen in der Zeile suchen
        if (line.find(interface) == std::string_view::npos) continue;

        // Format: "   eth0:  rx_bytes rx_packets ..."
        std::size_t const colon = line.find(':');
        if (colon == std::string_view::npos) break;

        std::string_view const values = line.substr(colon + 1);

        // Token parsen (whitespace-separiert) — C++26: direkter Zugriff
        auto parseToken = [&](int idx) -> std::uint64_t {
            std::size_t start = 0;
            int tokenIdx = 0;
            while (start < values.size()) {
                // Whitespace überspringen
                while (start < values.size() && (values[start] == ' ' || values[start] == '\t'))
                    ++start;
                if (start >= values.size()) break;

                std::size_t tokenEnd = start;
                while (tokenEnd < values.size() && values[tokenEnd] != ' ' && values[tokenEnd] != '\t')
                    ++tokenEnd;

                if (tokenIdx == idx) {
                    std::string_view const tok = values.substr(start, tokenEnd - start);
                    std::uint64_t val = 0;
                    auto [ptr, ec] = std::from_chars(tok.data(), tok.data() + tok.size(), val);
                    return (ec == std::errc{}) ? val : 0;
                }

                ++tokenIdx;
                start = tokenEnd;
            }
            return 0;
        };

        // Indices: 0=rx_bytes, 1=rx_packets, 2=rx_errs, ...
        //          8=tx_bytes, 9=tx_packets, 10=tx_errs
        stats.rxBytes   = parseToken(0);
        stats.rxPackets = parseToken(1);
        stats.rxErrors  = parseToken(2);
        stats.txBytes   = parseToken(8);
        stats.txPackets = parseToken(9);
        stats.txErrors  = parseToken(10);
        break;
    }

    return stats;
}

bool TelemetryModule::isValidInterface(std::string_view name) const noexcept
{
    // Skip loopback, Docker/bridge/VPN interfaces
    if (name == "lo" || name == "docker0") return false;

    // Only real Ethernet/WLAN interfaces (eth*, wlan*, wlp*, enp*)
    if (name.size() < 2) return false;

    // Virtual interfaces (veth*, br-, tun*, tap*, vbox*)
    if (name.starts_with("veth") || name.starts_with("br-") ||
        name.starts_with("tun") || name.starts_with("tap") ||
        name.starts_with("vbox") || name.starts_with("vmnet") ||
        name.starts_with("virbr") || name.starts_with("docker")) {
        return false;
    }

    return true;
}

} // namespace IPView::Telemetry
