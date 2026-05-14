// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — FlagLoader.cpp
//  C++26: auto, const-correctness, noexcept
//  Loads country flags asynchronously with in-memory caching.
// ═══════════════════════════════════════════════════════════════════════════════

#include "FlagLoader.h"
#include "SecurityUtil.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QDebug>
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
    currentLabel   = label;
    currentCountry = cc.toLower();

    QUrl const url(QStringLiteral("https://flagpedia.net/data/flags/w580/%1.png")
                       .arg(currentCountry));
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArrayLiteral("IPView/2.0"));
    request.setTransferTimeout(10000);

    QNetworkReply * const reply = manager->get(request);

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

    if (reply->error() != QNetworkReply::NoError || !currentLabel) {
        if (reply->error() != QNetworkReply::NoError) {
            qDebug().noquote() << QStringLiteral("Flag download failed: %1")
                                      .arg(reply->errorString());
        }
        reply->deleteLater();
        return;
    }

    QByteArray const raw = reply->readAll();
    QPixmap pixmap;

    if (pixmap.loadFromData(raw)) {
        // Store in cache
        flagCache.insert(currentCountry, pixmap);
        currentLabel->setPixmap(pixmap.scaled(currentLabel->size(),
                                Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    reply->deleteLater();
}

[[nodiscard]]
QPixmap FlagLoader::getFlagPixmap(const QString &cc) const noexcept
{
    return flagCache.value(cc.toLower());
}
