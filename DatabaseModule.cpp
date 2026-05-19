// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.8.0 — DatabaseModule.cpp
//  C++26: structured bindings, constexpr, [[nodiscard]]
//  SQLite-Persistenz: IP-Historie und Telemetrie-Daten.
//  Thread-sicher via QMutex. Singleton-Pattern.
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

    // Sicherstellen, dass das Verzeichnis existiert
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

    // WAL-Modus für bessere Performance bei parallelen Zugriffen
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

    // ── Index auf timestamp für schnelle Abfragen ─────────────────────────
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
//  Private Helpers
// ═══════════════════════════════════════════════════════════════════════════════

QString DatabaseModule::defaultDbPath() noexcept
{
    // Nutzt GenericConfigLocation (~/.config/ laut XDG) + /IPView/,
    // sodass die DB im selben Verzeichnis wie die QSettings-Config liegt:
    //   ~/.config/IPView/ipview_history.db
    //   ~/.config/IPView/IPView.conf  (QSettings, INI-Format)
    QString const configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    return configDir + QStringLiteral("/IPView/ipview_history.db");
}

} // namespace IPView::Storage
