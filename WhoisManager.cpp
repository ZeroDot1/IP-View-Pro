// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — WhoisManager.cpp
//  C++26: structured bindings, [[nodiscard]], auto, const-correctness
//  Asynchronous Whois/RDAP queries with API-specific normalization.
// ═══════════════════════════════════════════════════════════════════════════════

#include "WhoisManager.h"
#include "SecurityUtil.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QJsonArray>
#include <QDebug>

// ═══════════════════════════════════════════════════════════════════════════════
WhoisManager::WhoisManager(QObject *parent)
    : QObject(parent)
    , manager(new QNetworkAccessManager(this))
{
}

[[nodiscard]]
QStringList WhoisManager::getAvailableApis() const noexcept
{
    return {
        QStringLiteral("RDAP (Standard)"),
        QStringLiteral("IP-API (Detailed)"),
        QStringLiteral("IP-Whois.io")
    };
}

// ═══════════════════════════════════════════════════════════════════════════════
void WhoisManager::lookup(const QString &ip, const QString &apiName) noexcept
{
    if (ip.isEmpty() || apiName.isEmpty()) return;

    currentApi = apiName;

    QUrl url;
    if (apiName == QStringLiteral("RDAP (Standard)")) {
        url = QUrl(QStringLiteral("https://rdap.db.ripe.net/ip/%1").arg(ip));
    } else if (apiName == QStringLiteral("IP-API (Detailed)")) {
        url = QUrl(QStringLiteral("http://ip-api.com/json/%1?fields=66846719").arg(ip));
    } else {
        url = QUrl(QStringLiteral("http://ipwho.is/%1").arg(ip));
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArrayLiteral("IPView/2.0"));
    request.setTransferTimeout(15000);

    QNetworkReply * const reply = manager->get(request);

    // ── Security: Strict SSL validation ──────────────────────────────┐
    enforceStrictSsl(reply);                                            //┘

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });

    // Safety timeout
    QTimer::singleShot(20000, reply, [reply]() {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
void WhoisManager::onReplyFinished(QNetworkReply *reply)
{

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray const rawData = reply->readAll();
    QJsonDocument const doc = QJsonDocument::fromJson(rawData);

    if (!doc.isNull() && doc.isObject()) {
        QJsonObject const obj = doc.object();
        QJsonObject normalized;

        // API-specific normalization
        if (currentApi == QStringLiteral("RDAP (Standard)")) {
            normalized = normalizeRdap(obj);
        } else if (currentApi == QStringLiteral("IP-API (Detailed)")) {
            normalized = normalizeIpApi(obj);
        } else {
            // IP-Whois.io — flat structure, just sanitize
            for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
                QString const& key = it.key();
                QJsonValue const value = it.value();
                QString val;
                if (value.isString()) {
                    val = value.toString();
                } else if (value.isDouble()) {
                    val = QString::number(value.toDouble());
                } else if (value.isBool()) {
                    val = value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
                } else if (value.isArray()) {
                    QStringList items;
                    QJsonArray const arr = value.toArray();
                    items.reserve(arr.size());
                    for (QJsonValue const& av : arr) {
                        items.append(av.toVariant().toString());
                    }
                    val = items.join(QStringLiteral(", "));
                } else {
                    val = value.toVariant().toString();
                }
                normalized[key] = val.trimmed();
            }
        }

        if (!normalized.isEmpty()) {
            emit lookupFinished(QString::fromUtf8(
                QJsonDocument(normalized).toJson(QJsonDocument::Indented)));
            reply->deleteLater();
            return;
        }
    }

    // Fallback: output raw data
    emit lookupFinished(QString::fromUtf8(rawData));
    reply->deleteLater();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  RDAP Normalization
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]]
QJsonObject WhoisManager::normalizeRdap(const QJsonObject &obj) const noexcept
{
    QJsonObject result;

    // Handle
    result[QStringLiteral("Handle")] = obj[QStringLiteral("handle")].toString();

    // IP range from CIDR
    if (obj.contains(QStringLiteral("cidr0_cidrs"))) {
        QJsonArray const cidrs = obj[QStringLiteral("cidr0_cidrs")].toArray();
        if (!cidrs.isEmpty()) {
            QJsonObject const first = cidrs[0].toObject();
            result[QStringLiteral("IP Range")] = QStringLiteral("%1/%2")
                .arg(first[QStringLiteral("v4prefix")].toString())
                .arg(first[QStringLiteral("length")].toInt());
        }
    } else if (obj.contains(QStringLiteral("arin_cidr"))) {
        result[QStringLiteral("IP Range")] = obj[QStringLiteral("arin_cidr")].toString();
    }

    // Name & Type
    result[QStringLiteral("Name")] = obj[QStringLiteral("name")].toString();
    result[QStringLiteral("Type")] = obj[QStringLiteral("type")].toString();

    // Entities (organizations, abuse contact)
    if (obj.contains(QStringLiteral("entities"))) {
        QJsonArray const entities = obj[QStringLiteral("entities")].toArray();
        QStringList orgs;
        QStringList abuseContacts;

        for (QJsonValue const& entityVal : entities) {
            QJsonObject const entity = entityVal.toObject();
            QString const handle = entity[QStringLiteral("handle")].toString();

            // Extract roles
            if (entity.contains(QStringLiteral("roles"))) {
                QJsonArray const roles = entity[QStringLiteral("roles")].toArray();
                for (QJsonValue const& roleVal : roles) {
                    QString const role = roleVal.toString();
                    if (role == QStringLiteral("abuse")) {
                        abuseContacts.append(handle);
                    }
                }
            }

            // Organization from vcardArrays
            if (entity.contains(QStringLiteral("vcardArrays"))) {
                QJsonArray const vcards = entity[QStringLiteral("vcardArrays")].toArray();
                for (QJsonValue const& vcardVal : vcards) {
                    QJsonArray const vcard = vcardVal.toArray();
                    if (vcard.size() >= 3 && vcard[0].toString() == QStringLiteral("fn")) {
                        QString const fn = vcard[3].toString();
                        if (!fn.isEmpty()) orgs.append(fn);
                    }
                }
            } else if (!handle.isEmpty()) {
                orgs.append(handle);
            }
        }

        if (!orgs.isEmpty())         result[QStringLiteral("Organizations")] = orgs.join(QStringLiteral("; "));
        if (!abuseContacts.isEmpty()) result[QStringLiteral("Abuse Contact")] = abuseContacts.join(QStringLiteral("; "));
    }

    // Events
    if (obj.contains(QStringLiteral("events"))) {
        QJsonArray const events = obj[QStringLiteral("events")].toArray();
        for (QJsonValue const& evVal : events) {
            QJsonObject const ev = evVal.toObject();
            QString const action = ev[QStringLiteral("eventAction")].toString();
            QString const date  = ev[QStringLiteral("eventDate")].toString();
            if (action == QStringLiteral("registration")) result[QStringLiteral("Registration Date")] = date;
            if (action == QStringLiteral("last changed")) result[QStringLiteral("Last Changed")]      = date;
        }
    }

    // Links
    if (obj.contains(QStringLiteral("links"))) {
        QJsonArray const links = obj[QStringLiteral("links")].toArray();
        QStringList urls;
        for (QJsonValue const& linkVal : links) {
            QJsonObject const link = linkVal.toObject();
            QString const href = link[QStringLiteral("href")].toString();
            if (!href.isEmpty()) urls.append(href);
        }
        if (!urls.isEmpty()) result[QStringLiteral("RDAP Links")] = urls.join(QChar::LineFeed);
    }

    if (obj.contains(QStringLiteral("port43"))) {
        result[QStringLiteral("Whois Server")] = obj[QStringLiteral("port43")].toString();
    }
    if (obj.contains(QStringLiteral("status"))) {
        QJsonArray const statuses = obj[QStringLiteral("status")].toArray();
        QStringList slist;
        slist.reserve(statuses.size());
        for (QJsonValue const& s : statuses) {
            slist.append(s.toString());
        }
        result[QStringLiteral("Status")] = slist.join(QStringLiteral(", "));
    }

    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  IP-API Normalization
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]]
QJsonObject WhoisManager::normalizeIpApi(const QJsonObject &obj) const noexcept
{
    QJsonObject result;

    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
        QString const& key = it.key();
        QJsonValue const value = it.value();
        QString val;
        if (value.isString()) {
            val = value.toString();
        } else if (value.isDouble()) {
            val = QString::number(value.toDouble());
        } else if (value.isBool()) {
            val = value.toBool() ? QStringLiteral("Yes") : QStringLiteral("No");
        } else if (value.isArray()) {
            QStringList items;
            QJsonArray const arr = value.toArray();
            items.reserve(arr.size());
            for (QJsonValue const& av : arr) {
                items.append(av.toVariant().toString());
            }
            val = items.join(QStringLiteral(", "));
        } else {
            val = value.toVariant().toString();
        }

        if (!val.trimmed().isEmpty()) {
            result[key] = val.trimmed();
        }
    }

    return result;
}
