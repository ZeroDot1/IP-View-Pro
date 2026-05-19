// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — FlagLoader.cpp
//  C++26: auto, const-correctness, noexcept
//  Loads country flags asynchronously with in-memory caching.
// ═══════════════════════════════════════════════════════════════════════════════

#include "FlagLoader.h"
#include "SecurityUtil.h"
#include "Logger.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QTimer>

FlagLoader::FlagLoader(QObject *parent)
    : QObject(parent)
    , manager(new QNetworkAccessManager(this))
{
}

void FlagLoader::loadFlag(const QString &cc, QLabel *label) noexcept
{
    if (cc.isEmpty() || !label) return;

    // ── Cache-Check ─────────────────────────────────────────────────────
    auto const cacheIt = flagCache.constFind(cc);
    if (cacheIt != flagCache.constEnd()) {
        label->setPixmap(cacheIt->scaled(label->size(),
                         Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return;
    }

    // ── Download ─────────────────────────────────────────────────────────
    // Store per-request context via QNetworkReply properties to avoid
    // race conditions when multiple loadFlag() calls are in flight.
    QString const lowerCc = cc.toLower();

    QUrl const url(QStringLiteral("https://flagpedia.net/data/flags/w580/%1.png")
                       .arg(lowerCc));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArrayLiteral("IPView/2.0"));
    request.setTransferTimeout(10000);

    QNetworkReply * const reply = manager->get(request);

    // Attach context to the reply (prevents async race conditions)
    reply->setProperty("flagCountry", lowerCc);
    reply->setProperty("flagLabel",
                       QVariant::fromValue(reinterpret_cast<quintptr>(label)));

    // ── Security: detect and abort on SSL error ────────────────┐
    enforceStrictSsl(reply);                                        //┘

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });

    // Security timeout (15 s)
    QTimer::singleShot(15000, reply, [reply]() {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    });
}

void FlagLoader::onReplyFinished(QNetworkReply *reply)
{
    if (!reply) return;

    // Retrieve per-request context from reply properties
    QString const country = reply->property("flagCountry").toString();
    auto const labelPtr = reply->property("flagLabel").value<quintptr>();
    auto *label = reinterpret_cast<QLabel*>(labelPtr);

    if (reply->error() != QNetworkReply::NoError || !label) {
        if (reply->error() != QNetworkReply::NoError) {
            IPView::Logger::debug("Flag download failed for {}: {}",
                                      country.toStdString(), reply->errorString().toStdString());
        }
        reply->deleteLater();
        return;
    }

    QByteArray const raw = reply->readAll();
    QPixmap pixmap;

    if (pixmap.loadFromData(raw)) {
        // Store in cache
        flagCache.insert(country, pixmap);
        label->setPixmap(pixmap.scaled(label->size(),
                          Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    reply->deleteLater();
}

[[nodiscard]]
QPixmap FlagLoader::getFlagPixmap(const QString &cc) const noexcept
{
    return flagCache.value(cc.toLower());
}
