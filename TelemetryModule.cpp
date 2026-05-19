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
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::unexpected(
            std::string("Cannot open /proc/net/dev: ") + file.errorString().toStdString());
    }

    QTextStream in(&file);
    QString const content = in.readAll();
    file.close();

    return parseProcNetDev(content, interface);
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
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred(QStringLiteral("Cannot open /proc/net/dev"));
        return;
    }

    QTextStream in(&file);
    QString const content = in.readAll();
    file.close();

    QStringList const ifaces = availableInterfaces();
    QList<InterfaceInfo> updated;

    for (QString const &iface : ifaces) {
        std::string const name = iface.toStdString();
        Stats const current = parseProcNetDev(content, name);

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

Stats TelemetryModule::parseProcNetDev(const QString &content,
                                       std::string_view interface) noexcept
{
    Stats stats{};

    // /proc/net/dev Format:
    // Inter-|   Receive                              |  Transmit
    //  face |bytes    packets errs drop fifo ...     |bytes    packets errs ...
    //    lo: 12345    678    0    0    0 ...         98765    432    0 ...

    QString const searchStr = QString::fromUtf8(interface.data(),
                                                static_cast<qsizetype>(interface.size()));
    QStringList const lines = content.split(QLatin1Char('\n'));

    for (QString const &line : lines) {
        if (!line.contains(searchStr, Qt::CaseInsensitive)) continue;

        // Format: "   eth0:  rx_bytes rx_packets rx_errs ...  tx_bytes ..."
        // Split after the colon
        qsizetype const colonPos = line.indexOf(QLatin1Char(':'));
        if (colonPos < 0) break;

        QString const valuesPart = line.mid(colonPos + 1).trimmed();
        QStringList const tokens = valuesPart.split(QRegularExpression(QStringLiteral("\\s+")),
                                                     Qt::SkipEmptyParts);

        if (tokens.size() < 10) break;

        // C++26: std::from_chars for performant string-to-integer conversion
        auto parseU64 = [](const QString &s) -> std::uint64_t {
            std::uint64_t val = 0;
            QByteArray const ba = s.toUtf8();
            auto [ptr, ec] = std::from_chars(ba.data(), ba.data() + ba.size(), val);
            if (ec != std::errc{}) return 0;
            return val;
        };

        // Indices: 0=rx_bytes, 1=rx_packets, 2=rx_errs, 3=rx_drop,
        //          4=rx_fifo, 5=rx_frame, 6=rx_compressed, 7=rx_multicast,
        //          8=tx_bytes, 9=tx_packets, 10=tx_errs, 11=tx_drop, ...
        stats.rxBytes   = parseU64(tokens.value(0));
        stats.rxPackets = parseU64(tokens.value(1));
        stats.rxErrors  = parseU64(tokens.value(2));
        stats.txBytes   = parseU64(tokens.value(8));
        stats.txPackets = parseU64(tokens.value(9));
        stats.txErrors  = parseU64(tokens.value(10));
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
