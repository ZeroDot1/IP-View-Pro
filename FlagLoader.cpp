// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.5 — FlagLoader.cpp
//  C++26: auto, const-correctness, noexcept, QPointer for label lifetime
//  Loads country flags asynchronously with in-memory caching.
//
//  v2.15.4 rewrite — see FlagLoader.h for the rationale.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "FlagLoader.h"
#include "SecurityUtil.h"
#include "Logger.h"
#include "Timeouts.hpp"

#include <QUrl>
#include <QNetworkRequest>
#include <QTimer>

// ═══════════════════════════════════════════════════════════════════════════════

FlagLoader::FlagLoader(QObject *parent)
    : QObject(parent)
    , manager(new QNetworkAccessManager(this))
{
}

// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]]
QString FlagLoader::flagUrl(const QString &lowerCc) noexcept
{
    return QString::fromLatin1(FLAG_URL_FMT).arg(lowerCc);
}

// ═══════════════════════════════════════════════════════════════════════════════

void FlagLoader::loadFlag(const QString &cc, QLabel *label) noexcept
{
    if (cc.isEmpty() || !label) return;

    QString const lowerCc = cc.toLower();

    // ── Cache hit ────────────────────────────────────────────────────────
    auto const cacheIt = flagCache.constFind(lowerCc);
    if (cacheIt != flagCache.constEnd()) {
        label->setPixmap(cacheIt->scaled(label->size(),
                         Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return;
    }

    // ── New request ──────────────────────────────────────────────────────
    QNetworkRequest request{ QUrl(flagUrl(lowerCc)) };
    request.setRawHeader("User-Agent", QByteArrayLiteral("IPView/2.15"));
    request.setTransferTimeout(IPView::Timeouts::HTTP_FALLBACK);

    QNetworkReply * const reply = manager->get(request);

    // Record the (reply → context) association BEFORE returning, so
    // the finished slot can always look it up.
    mPending.insert(reply, FlagRequest{ QPointer<QLabel>(label), lowerCc });

    // Strict SSL — abort on any certificate error.
    enforceStrictSsl(reply);

    // Safety timeout — slightly longer than the transfer timeout,
    // so the OS-level timeout fires first and the reply is already
    // finished when this timer hits.
    QTimer::singleShot(IPView::Timeouts::HTTP_FALLBACK + std::chrono::seconds{5},
                       reply, [reply]() {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished,
            this, &FlagLoader::onReplyFinished);
}

// ═══════════════════════════════════════════════════════════════════════════════

void FlagLoader::onReplyFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    // Pop the per-request context. The map entry is removed
    // exactly once, no matter how many times the finished signal
    // fires (Qt guarantees a single emit, but defence-in-depth).
    auto it = mPending.find(reply);
    if (it == mPending.end()) {
        // Unknown reply — should not happen, but be defensive.
        reply->deleteLater();
        return;
    }
    FlagRequest const ctx = it.value();
    mPending.erase(it);

    // The QLabel may have been destroyed while the request was
    // in flight (tab switched, window closed). QPointer turns
    // the dangling pointer into nullptr automatically.
    QLabel *label = ctx.label.data();
    if (!label || reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::NoError) {
            IPView::Logger::debug("Flag download failed for {}: {}",
                                  ctx.cc.toStdString(),
                                  reply->errorString().toStdString());
        }
        reply->deleteLater();
        return;
    }

    QByteArray const raw = reply->readAll();
    QPixmap        pixmap;

    if (pixmap.loadFromData(raw)) {
        flagCache.insert(ctx.cc, pixmap);
        label->setPixmap(pixmap.scaled(label->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation));
    }

    reply->deleteLater();
}

// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]]
QPixmap FlagLoader::getFlagPixmap(const QString &cc) const noexcept
{
    return flagCache.value(cc.toLower());
}
