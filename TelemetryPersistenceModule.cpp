// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.0 — TelemetryPersistenceModule.cpp
//  C++26: std::optional, [[nodiscard]], noexcept, structured bindings
//  Periodic telemetry aggregation and historical data persistence.
//  Stores aggregated stats in SQLite via DatabaseModule.
//  Auto-start configurable via QSettings (ConfigManager).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TelemetryPersistenceModule.h"
#include "ConfigManager.h"
#include "DatabaseModule.h"

#include <QFile>
#include <QTextStream>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutexLocker>

#include "Logger.h"

#include <algorithm>
#include <charconv>
#include <map>
#include <system_error>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Telemetry {

// ═══════════════════════════════════════════════════════════════════════════════
TelemetryPersistenceModule::TelemetryPersistenceModule(QObject *parent)
    : QObject(parent)
{
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, &TelemetryPersistenceModule::onAggregationTick);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Lifecycle
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryPersistenceModule::start(int intervalMs) noexcept
{
    if (mTimer->isActive()) return;

    // Load initial samples to enable delta calculation
    QStringList const interfaces = listPhysicalInterfaces();
    for (QString const &iface : interfaces) {
        auto const bytes = fetchInterfaceBytes(iface);
        if (bytes.has_value()) {
            mPreviousSamples[iface] = InterfaceSample{
                Sample{bytes->first, bytes->second},
                QDateTime::currentDateTime()
            };
        }
    }
    mSamplesLoaded = true;

    // Trigger first aggregation immediately
    aggregateNow();

    mTimer->start(intervalMs);
    IPView::Logger::info("TelemetryPersistenceModule: Started (interval={} ms)", intervalMs);
}

void TelemetryPersistenceModule::stop() noexcept
{
    if (!mTimer->isActive()) return;
    mTimer->stop();
    mPreviousSamples.clear();
    mSamplesLoaded = false;
    IPView::Logger::info("TelemetryPersistenceModule: Stopped");
}

bool TelemetryPersistenceModule::isRunning() const noexcept
{
    return mTimer->isActive();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Manual aggregation
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryPersistenceModule::aggregateNow() noexcept
{
    emit aggregationStarted();

    QDateTime const now = QDateTime::currentDateTime();
    QStringList const interfaces = listPhysicalInterfaces();

    for (QString const &iface : interfaces) {
        auto const currentBytes = fetchInterfaceBytes(iface);
        if (!currentBytes.has_value()) continue;

        quint64 const rxNow = currentBytes->first;
        quint64 const txNow = currentBytes->second;

        auto prevIt = mPreviousSamples.find(iface);
        if (prevIt == mPreviousSamples.end()) {
            // First sample — store and skip speed calculation
            mPreviousSamples[iface] = InterfaceSample{
                Sample{rxNow, txNow}, now
            };
            continue;
        }

        InterfaceSample &prev = prevIt->second;
        double const elapsedSec = static_cast<double>(prev.timestamp.msecsTo(now)) / 1000.0;
        if (elapsedSec <= 0.0) continue;

        // Calculate average speeds (bytes/s) over the window
        double const rxSpeed = static_cast<double>(rxNow - prev.sample.rxBytes) / elapsedSec;
        double const txSpeed = static_cast<double>(txNow - prev.sample.txBytes) / elapsedSec;

        // Build an aggregated record
        IPView::Storage::AggregatedTelemetryEntry rec;
        rec.interfaceName = iface;
        rec.avgRxSpeed    = std::max(0.0, rxSpeed);
        rec.avgTxSpeed    = std::max(0.0, txSpeed);
        rec.minRxSpeed    = rec.avgRxSpeed;  // Single sample → avg = min = max
        rec.minTxSpeed    = rec.avgTxSpeed;
        rec.maxRxSpeed    = rec.avgRxSpeed;
        rec.maxTxSpeed    = rec.avgTxSpeed;
        rec.totalRxBytes  = rxNow;
        rec.totalTxBytes  = txNow;
        rec.windowStart   = prev.timestamp;
        rec.windowEnd     = now;

        // Store via DatabaseModule
        bool const stored = Storage::DatabaseModule::storeTelemetryAggregated(
            iface, rec.avgRxSpeed, rec.avgTxSpeed,
            rec.minRxSpeed, rec.minTxSpeed,
            rec.maxRxSpeed, rec.maxTxSpeed,
            rxNow, txNow,
            rec.windowStart, rec.windowEnd
        );

        if (stored) {
            emit aggregationStored(rec);
        } else {
            emit aggregationError(QStringLiteral("Failed to store aggregation for %1").arg(iface));
        }

        // Update previous sample
        prev.sample.rxBytes = rxNow;
        prev.sample.txBytes = txNow;
        prev.timestamp      = now;
    }

    emit aggregationCompleted();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Historical queries
//  Delegates to DatabaseModule for the actual SQL.
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<IPView::Storage::AggregatedTelemetryEntry>
TelemetryPersistenceModule::getHistory(int limit) const noexcept
{
    return Storage::DatabaseModule::getTelemetryAggregated(limit);
}

std::vector<IPView::Storage::AggregatedTelemetryEntry>
TelemetryPersistenceModule::getHistoryForInterface(
    const QString &interfaceName, int limit) const noexcept
{
    return Storage::DatabaseModule::getTelemetryAggregatedForInterface(
        interfaceName, limit);
}

std::optional<IPView::Storage::AggregatedTelemetryEntry>
TelemetryPersistenceModule::getLatestAggregation() const noexcept
{
    return Storage::DatabaseModule::getLatestTelemetryAggregated();
}

std::optional<IPView::Storage::AggregatedTelemetryEntry>
TelemetryPersistenceModule::getStatsForWindow(
    const QDateTime &from, const QDateTime &to) const noexcept
{
    return Storage::DatabaseModule::getTelemetryStatsForWindow(from, to);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Auto-start config — delegates to ConfigManager
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryPersistenceModule::setAutoStartEnabled(bool enabled) noexcept
{
    IPView::Config::Manager::saveTelemetryAutoStart(enabled);
}

bool TelemetryPersistenceModule::isAutoStartEnabled() noexcept
{
    return IPView::Config::Manager::loadTelemetryAutoStart();
}

void TelemetryPersistenceModule::setAutoStartInterval(int intervalMs) noexcept
{
    IPView::Config::Manager::saveTelemetryInterval(intervalMs);
}

int TelemetryPersistenceModule::autoStartInterval() noexcept
{
    // Default 60000 ms = 1 minute for persistence aggregation
    return IPView::Config::Manager::loadTelemetryInterval(60000);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Maintenance
// ═══════════════════════════════════════════════════════════════════════════════

bool TelemetryPersistenceModule::clearAggregatedHistory() noexcept
{
    return Storage::DatabaseModule::clearTelemetryAggregated();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private timer slot
// ═══════════════════════════════════════════════════════════════════════════════

void TelemetryPersistenceModule::onAggregationTick() noexcept
{
    aggregateNow();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private helpers
// ═══════════════════════════════════════════════════════════════════════════════

QStringList TelemetryPersistenceModule::listPhysicalInterfaces() noexcept
{
    QStringList result;
    QFile file(QStringLiteral("/proc/net/dev"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return result;
    }

    QTextStream in(&file);
    in.readLine(); // header 1
    in.readLine(); // header 2

    static QRegularExpression const re(QStringLiteral(R"(^\s*([^:]+):)"));

    while (!in.atEnd()) {
        QString const line = in.readLine();
        auto const match = re.match(line);
        if (match.hasMatch()) {
            QString const ifName = match.captured(1).trimmed();
            // Skip loopback, virtual, docker
            if (ifName == QStringLiteral("lo")) continue;
            if (ifName.startsWith(QStringLiteral("veth")) ||
                ifName.startsWith(QStringLiteral("br-"))  ||
                ifName.startsWith(QStringLiteral("tun"))  ||
                ifName.startsWith(QStringLiteral("tap"))  ||
                ifName.startsWith(QStringLiteral("docker"))) {
                continue;
            }
            result.append(ifName);
        }
    }

    file.close();
    return result;
}

std::optional<std::pair<quint64, quint64>>
TelemetryPersistenceModule::fetchInterfaceBytes(const QString &interfaceName) noexcept
{
    QFile file(QStringLiteral("/proc/net/dev"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }

    QTextStream in(&file);
    QString const content = in.readAll();
    file.close();

    QStringList const lines = content.split(QLatin1Char('\n'));

    for (QString const &line : lines) {
        if (!line.contains(interfaceName, Qt::CaseInsensitive)) continue;

        qsizetype const colonPos = line.indexOf(QLatin1Char(':'));
        if (colonPos < 0) break;

        QString const valuesPart = line.mid(colonPos + 1).trimmed();
        QStringList const tokens = valuesPart.split(
            QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);

        if (tokens.size() < 10) break;

        // C++26: std::from_chars
        auto parseU64 = [](const QString &s) -> quint64 {
            quint64 val = 0;
            QByteArray const ba = s.toUtf8();
            auto [ptr, ec] = std::from_chars(ba.data(), ba.data() + ba.size(), val);
            if (ec != std::errc{}) return 0;
            return val;
        };

        quint64 const rx = parseU64(tokens.value(0));
        quint64 const tx = parseU64(tokens.value(8));
        return std::make_pair(rx, tx);
    }

    return std::nullopt;
}

} // namespace IPView::Telemetry
