// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — DatabaseModule.cpp
//  C++26: structured bindings, constexpr, [[nodiscard]]
//  SQLite persistence: IP history and telemetry data.
//  Thread-safe via QMutex. Singleton pattern.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "DatabaseModule.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QThread>

// ═══════════════════════════════════════════════════════════════════════════════
//  Statische Member
// ═══════════════════════════════════════════════════════════════════════════════

QSqlDatabase IPView::Storage::DatabaseModule::sDb;
QMutex       IPView::Storage::DatabaseModule::sMutex;
bool         IPView::Storage::DatabaseModule::sInitialized{false};
QString      IPView::Storage::DatabaseModule::sDbPath;

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Storage {

// ═══════════════════════════════════════════════════════════════════════════════
//  Singleton
// ═══════════════════════════════════════════════════════════════════════════════

DatabaseModule& DatabaseModule::instance() noexcept
{
    static DatabaseModule inst;
    return inst;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Init / Shutdown
// ═══════════════════════════════════════════════════════════════════════════════

bool DatabaseModule::init(const QString &dbPath) noexcept
{
    QMutexLocker lock(&sMutex);

    if (sInitialized) return true;

    QString const path = dbPath.isEmpty() ? defaultDbPath() : dbPath;
    sDbPath = path;

    // Ensure the directory exists
    QDir dir = QFileInfo(path).absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    sDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                    QStringLiteral("ipview_main"));
    sDb.setDatabaseName(path);

    if (!sDb.open()) {
        qWarning("DatabaseModule: Cannot open database: %s",
                 qPrintable(sDb.lastError().text()));
        return false;
    }

    // WAL mode for better performance on concurrent access
    QSqlQuery pragma(sDb);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));
    pragma.exec(QStringLiteral("PRAGMA foreign_keys=ON"));

    if (!createSchema()) {
        qWarning("DatabaseModule: Schema creation failed");
        sDb.close();
        return false;
    }

    sInitialized = true;
    qInfo("DatabaseModule: Initialized at %s", qPrintable(path));
    return true;
}

void DatabaseModule::shutdown() noexcept
{
    QMutexLocker lock(&sMutex);

    if (!sInitialized) return;

    sDb.close();
    sInitialized = false;
    qInfo("DatabaseModule: Shutdown complete");
}

bool DatabaseModule::isInitialized() noexcept
{
    return sInitialized;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Schema
// ═══════════════════════════════════════════════════════════════════════════════

bool DatabaseModule::createSchema() noexcept
{
    QSqlQuery query(sDb);

    // ── Tabelle: ip_history ────────────────────────────────────────────────
    bool const historyTable = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS ip_history ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  ip         TEXT NOT NULL,"
        "  country    TEXT,"
        "  country_code TEXT,"
        "  city       TEXT,"
        "  org        TEXT,"
        "  asn        TEXT,"
        "  json_data  TEXT,"
        "  timestamp  DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    ));

    if (!historyTable) {
        qWarning("DatabaseModule: Failed to create ip_history: %s",
                 qPrintable(query.lastError().text()));
        return false;
    }

    // ── Index on timestamp for fast queries ─────────────────────────
    query.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_history_timestamp "
        "ON ip_history(timestamp DESC)"
    ));

    // ── Tabelle: telemetry ─────────────────────────────────────────────────
    bool const telemetryTable = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS telemetry ("
        "  id         INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  interface  TEXT NOT NULL,"
        "  rx_bytes   INTEGER DEFAULT 0,"
        "  tx_bytes   INTEGER DEFAULT 0,"
        "  rx_speed   REAL DEFAULT 0.0,"
        "  tx_speed   REAL DEFAULT 0.0,"
        "  timestamp  DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    ));

    if (!telemetryTable) {
        qWarning("DatabaseModule: Failed to create telemetry table: %s",
                 qPrintable(query.lastError().text()));
        return false;
    }

    // ── Index auf interface + timestamp ────────────────────────────────────
    query.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_telemetry_iface_time "
        "ON telemetry(interface, timestamp DESC)"
    ));

    // ── telemetry_aggregated table (for TelemetryPersistenceModule) ────────
    bool const aggTable = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS telemetry_aggregated ("
        "  id           INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  interface    TEXT NOT NULL,"
        "  avg_rx_speed REAL DEFAULT 0.0,"
        "  avg_tx_speed REAL DEFAULT 0.0,"
        "  min_rx_speed REAL DEFAULT 0.0,"
        "  min_tx_speed REAL DEFAULT 0.0,"
        "  max_rx_speed REAL DEFAULT 0.0,"
        "  max_tx_speed REAL DEFAULT 0.0,"
        "  total_rx_bytes INTEGER DEFAULT 0,"
        "  total_tx_bytes INTEGER DEFAULT 0,"
        "  window_start DATETIME,"
        "  window_end   DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    ));

    if (!aggTable) {
        qWarning("DatabaseModule: Failed to create telemetry_aggregated table: %s",
                 qPrintable(query.lastError().text()));
        return false;
    }

    query.exec(QStringLiteral(
        "CREATE INDEX IF NOT EXISTS idx_agg_telemetry_iface_time "
        "ON telemetry_aggregated(interface, window_end DESC)"
    ));

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Write Operations
// ═══════════════════════════════════════════════════════════════════════════════

bool DatabaseModule::storeResult(const QJsonObject &data) noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return false;

    QSqlQuery query(sDb);
    query.prepare(QStringLiteral(
        "INSERT INTO ip_history (ip, country, country_code, city, org, asn, json_data) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"
    ));

    query.addBindValue(data[QStringLiteral("ip")].toString());
    query.addBindValue(data[QStringLiteral("country_name")].toString());
    query.addBindValue(data[QStringLiteral("country_code")].toString());
    query.addBindValue(data[QStringLiteral("city")].toString());
    query.addBindValue(data[QStringLiteral("org")].toString());
    query.addBindValue(data[QStringLiteral("asn")].toString());

    QJsonDocument const doc(data);
    query.addBindValue(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    if (!query.exec()) {
        qWarning("DatabaseModule: storeResult failed: %s",
                 qPrintable(query.lastError().text()));
        return false;
    }

    return true;
}

bool DatabaseModule::storeTelemetry(const QString &interfaceName,
                                    quint64 rxBytes, quint64 txBytes,
                                    double rxSpeed, double txSpeed) noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return false;

    QSqlQuery query(sDb);
    query.prepare(QStringLiteral(
        "INSERT INTO telemetry (interface, rx_bytes, tx_bytes, rx_speed, tx_speed) "
        "VALUES (?, ?, ?, ?, ?)"
    ));

    query.addBindValue(interfaceName);
    query.addBindValue(static_cast<qint64>(rxBytes));
    query.addBindValue(static_cast<qint64>(txBytes));
    query.addBindValue(rxSpeed);
    query.addBindValue(txSpeed);

    if (!query.exec()) {
        qWarning("DatabaseModule: storeTelemetry failed: %s",
                 qPrintable(query.lastError().text()));
        return false;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Read Operations
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<HistoryEntry> DatabaseModule::getHistory(int limit) noexcept
{
    QMutexLocker lock(&sMutex);
    std::vector<HistoryEntry> entries;

    if (!sInitialized) return entries;

    QSqlQuery query(sDb);
    query.prepare(QStringLiteral(
        "SELECT id, ip, country, country_code, city, org, asn, json_data, timestamp "
        "FROM ip_history ORDER BY timestamp DESC LIMIT ?"
    ));
    query.addBindValue(limit);

    if (!query.exec()) {
        qWarning("DatabaseModule: getHistory failed: %s",
                 qPrintable(query.lastError().text()));
        return entries;
    }

    while (query.next()) {
        HistoryEntry entry;
        entry.id          = query.value(0).toLongLong();
        entry.ip          = query.value(1).toString();
        entry.countryName = query.value(2).toString();
        entry.countryCode = query.value(3).toString();
        entry.city        = query.value(4).toString();
        entry.org         = query.value(5).toString();
        entry.asn         = query.value(6).toString();
        entry.jsonPayload = query.value(7).toString();
        entry.timestamp   = query.value(8).toDateTime();
        entries.push_back(std::move(entry));
    }

    return entries;
}

std::optional<HistoryEntry> DatabaseModule::getLatestEntry() noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return std::nullopt;

    QSqlQuery query(sDb);
    query.exec(QStringLiteral(
        "SELECT id, ip, country, country_code, city, org, asn, json_data, timestamp "
        "FROM ip_history ORDER BY timestamp DESC LIMIT 1"
    ));

    if (query.next()) {
        HistoryEntry entry;
        entry.id          = query.value(0).toLongLong();
        entry.ip          = query.value(1).toString();
        entry.countryName = query.value(2).toString();
        entry.countryCode = query.value(3).toString();
        entry.city        = query.value(4).toString();
        entry.org         = query.value(5).toString();
        entry.asn         = query.value(6).toString();
        entry.jsonPayload = query.value(7).toString();
        entry.timestamp   = query.value(8).toDateTime();
        return entry;
    }

    return std::nullopt;
}

int DatabaseModule::getHistoryCount() noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return 0;

    QSqlQuery query(sDb);
    if (query.exec(QStringLiteral("SELECT COUNT(*) FROM ip_history")) && query.next()) {
        return query.value(0).toInt();
    }

    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Maintenance
// ═══════════════════════════════════════════════════════════════════════════════

bool DatabaseModule::clearHistory() noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return false;

    QSqlQuery query(sDb);
    if (!query.exec(QStringLiteral("DELETE FROM ip_history"))) {
        qWarning("DatabaseModule: clearHistory failed: %s",
                 qPrintable(query.lastError().text()));
        return false;
    }

    return true;
}

bool DatabaseModule::vacuum() noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return false;

    QSqlQuery query(sDb);
    return query.exec(QStringLiteral("VACUUM"));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Aggregated Telemetry Write
// ═══════════════════════════════════════════════════════════════════════════════

bool DatabaseModule::storeTelemetryAggregated(
    const QString &interfaceName,
    double avgRxSpeed, double avgTxSpeed,
    double minRxSpeed, double minTxSpeed,
    double maxRxSpeed, double maxTxSpeed,
    quint64 totalRxBytes, quint64 totalTxBytes,
    const QDateTime &windowStart,
    const QDateTime &windowEnd) noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return false;

    QSqlQuery query(sDb);
    query.prepare(QStringLiteral(
        "INSERT INTO telemetry_aggregated "
        "(interface, avg_rx_speed, avg_tx_speed, "
        " min_rx_speed, min_tx_speed, max_rx_speed, max_tx_speed, "
        " total_rx_bytes, total_tx_bytes, window_start, window_end) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
    ));

    query.addBindValue(interfaceName);
    query.addBindValue(avgRxSpeed);
    query.addBindValue(avgTxSpeed);
    query.addBindValue(minRxSpeed);
    query.addBindValue(minTxSpeed);
    query.addBindValue(maxRxSpeed);
    query.addBindValue(maxTxSpeed);
    query.addBindValue(static_cast<qint64>(totalRxBytes));
    query.addBindValue(static_cast<qint64>(totalTxBytes));
    query.addBindValue(windowStart);
    query.addBindValue(windowEnd);

    if (!query.exec()) {
        qWarning("DatabaseModule: storeTelemetryAggregated failed: %s",
                 qPrintable(query.lastError().text()));
        return false;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Aggregated Telemetry Read
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<AggregatedTelemetryEntry>
DatabaseModule::getTelemetryAggregated(int limit) noexcept
{
    QMutexLocker lock(&sMutex);
    std::vector<AggregatedTelemetryEntry> entries;

    if (!sInitialized) return entries;

    QSqlQuery query(sDb);
    query.prepare(QStringLiteral(
        "SELECT id, interface, avg_rx_speed, avg_tx_speed, "
        "       min_rx_speed, min_tx_speed, max_rx_speed, max_tx_speed, "
        "       total_rx_bytes, total_tx_bytes, window_start, window_end "
        "FROM telemetry_aggregated "
        "ORDER BY window_end DESC LIMIT ?"
    ));
    query.addBindValue(limit);

    if (!query.exec()) {
        qWarning("DatabaseModule: getTelemetryAggregated failed: %s",
                 qPrintable(query.lastError().text()));
        return entries;
    }

    while (query.next()) {
        AggregatedTelemetryEntry e;
        e.id             = query.value(0).toLongLong();
        e.interfaceName  = query.value(1).toString();
        e.avgRxSpeed     = query.value(2).toDouble();
        e.avgTxSpeed     = query.value(3).toDouble();
        e.minRxSpeed     = query.value(4).toDouble();
        e.minTxSpeed     = query.value(5).toDouble();
        e.maxRxSpeed     = query.value(6).toDouble();
        e.maxTxSpeed     = query.value(7).toDouble();
        e.totalRxBytes   = static_cast<quint64>(query.value(8).toLongLong());
        e.totalTxBytes   = static_cast<quint64>(query.value(9).toLongLong());
        e.windowStart    = query.value(10).toDateTime();
        e.windowEnd      = query.value(11).toDateTime();
        entries.push_back(std::move(e));
    }

    return entries;
}

std::vector<AggregatedTelemetryEntry>
DatabaseModule::getTelemetryAggregatedForInterface(
    const QString &interfaceName, int limit) noexcept
{
    QMutexLocker lock(&sMutex);
    std::vector<AggregatedTelemetryEntry> entries;

    if (!sInitialized) return entries;

    QSqlQuery query(sDb);
    query.prepare(QStringLiteral(
        "SELECT id, interface, avg_rx_speed, avg_tx_speed, "
        "       min_rx_speed, min_tx_speed, max_rx_speed, max_tx_speed, "
        "       total_rx_bytes, total_tx_bytes, window_start, window_end "
        "FROM telemetry_aggregated "
        "WHERE interface = ? "
        "ORDER BY window_end DESC LIMIT ?"
    ));
    query.addBindValue(interfaceName);
    query.addBindValue(limit);

    if (!query.exec()) {
        qWarning("DatabaseModule: getTelemetryAggregatedForInterface failed: %s",
                 qPrintable(query.lastError().text()));
        return entries;
    }

    while (query.next()) {
        AggregatedTelemetryEntry e;
        e.id             = query.value(0).toLongLong();
        e.interfaceName  = query.value(1).toString();
        e.avgRxSpeed     = query.value(2).toDouble();
        e.avgTxSpeed     = query.value(3).toDouble();
        e.minRxSpeed     = query.value(4).toDouble();
        e.minTxSpeed     = query.value(5).toDouble();
        e.maxRxSpeed     = query.value(6).toDouble();
        e.maxTxSpeed     = query.value(7).toDouble();
        e.totalRxBytes   = static_cast<quint64>(query.value(8).toLongLong());
        e.totalTxBytes   = static_cast<quint64>(query.value(9).toLongLong());
        e.windowStart    = query.value(10).toDateTime();
        e.windowEnd      = query.value(11).toDateTime();
        entries.push_back(std::move(e));
    }

    return entries;
}

std::optional<AggregatedTelemetryEntry>
DatabaseModule::getLatestTelemetryAggregated() noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return std::nullopt;

    QSqlQuery query(sDb);
    query.prepare(QStringLiteral(
        "SELECT id, interface, avg_rx_speed, avg_tx_speed, "
        "       min_rx_speed, min_tx_speed, max_rx_speed, max_tx_speed, "
        "       total_rx_bytes, total_tx_bytes, window_start, window_end "
        "FROM telemetry_aggregated "
        "ORDER BY window_end DESC LIMIT 1"
    ));

    if (!query.exec()) return std::nullopt;

    if (query.next()) {
        AggregatedTelemetryEntry e;
        e.id             = query.value(0).toLongLong();
        e.interfaceName  = query.value(1).toString();
        e.avgRxSpeed     = query.value(2).toDouble();
        e.avgTxSpeed     = query.value(3).toDouble();
        e.minRxSpeed     = query.value(4).toDouble();
        e.minTxSpeed     = query.value(5).toDouble();
        e.maxRxSpeed     = query.value(6).toDouble();
        e.maxTxSpeed     = query.value(7).toDouble();
        e.totalRxBytes   = static_cast<quint64>(query.value(8).toLongLong());
        e.totalTxBytes   = static_cast<quint64>(query.value(9).toLongLong());
        e.windowStart    = query.value(10).toDateTime();
        e.windowEnd      = query.value(11).toDateTime();
        return e;
    }

    return std::nullopt;
}

std::optional<AggregatedTelemetryEntry>
DatabaseModule::getTelemetryStatsForWindow(
    const QDateTime &from, const QDateTime &to) noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return std::nullopt;

    QSqlQuery query(sDb);
    query.prepare(QStringLiteral(
        "SELECT "
        "  0 AS id, '' AS interface, "
        "  AVG(avg_rx_speed) AS avg_rx, AVG(avg_tx_speed) AS avg_tx, "
        "  MIN(min_rx_speed) AS min_rx, MIN(min_tx_speed) AS min_tx, "
        "  MAX(max_rx_speed) AS max_rx, MAX(max_tx_speed) AS max_tx, "
        "  SUM(total_rx_bytes) AS total_rx, SUM(total_tx_bytes) AS total_tx, "
        "  ? AS ws, ? AS we "
        "FROM telemetry_aggregated "
        "WHERE window_end >= ? AND window_end <= ?"
    ));
    query.addBindValue(from);
    query.addBindValue(to);
    query.addBindValue(from);
    query.addBindValue(to);

    if (!query.exec()) return std::nullopt;

    if (query.next()) {
        AggregatedTelemetryEntry e;
        e.id             = 0;
        e.interfaceName  = QStringLiteral("*ALL*");
        e.avgRxSpeed     = query.value(2).toDouble();
        e.avgTxSpeed     = query.value(3).toDouble();
        e.minRxSpeed     = query.value(4).toDouble();
        e.minTxSpeed     = query.value(5).toDouble();
        e.maxRxSpeed     = query.value(6).toDouble();
        e.maxTxSpeed     = query.value(7).toDouble();
        e.totalRxBytes   = static_cast<quint64>(query.value(8).toLongLong());
        e.totalTxBytes   = static_cast<quint64>(query.value(9).toLongLong());
        e.windowStart    = query.value(10).toDateTime();
        e.windowEnd      = query.value(11).toDateTime();
        return e;
    }

    return std::nullopt;
}

bool DatabaseModule::clearTelemetryAggregated() noexcept
{
    QMutexLocker lock(&sMutex);
    if (!sInitialized) return false;

    QSqlQuery query(sDb);
    if (!query.exec(QStringLiteral("DELETE FROM telemetry_aggregated"))) {
        qWarning("DatabaseModule: clearTelemetryAggregated failed: %s",
                 qPrintable(query.lastError().text()));
        return false;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Private Helpers
// ═══════════════════════════════════════════════════════════════════════════════

QString DatabaseModule::defaultDbPath() noexcept
{
    // Uses GenericConfigLocation (~/.config/ per XDG) + /IPView/,
    // so the DB resides in the same directory as the QSettings config:
    //   ~/.config/IPView/ipview_history.db
    //   ~/.config/IPView/IPView.conf  (QSettings, INI format)
    QString const configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return configDir + QStringLiteral("/IPView/ipview_history.db");
}

} // namespace IPView::Storage
