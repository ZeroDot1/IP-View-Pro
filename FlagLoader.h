// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.5 — FlagLoader.h
//  C++26: [[nodiscard]], const-correctness, default member init
//  Loads country flag images asynchronously with an in-memory cache.
//
//  v2.15.4 rewrite:
//    * Source switched from flagpedia.net to flagcdn.com — flagpedia
//      started rate-limiting and eventually dropping our User-Agent,
//      which made the downloads fail silently. flagcdn.com is a CDN
//      (Cloudflare) so it has stable latency and serves the same
//      ISO-3166 country code URLs.
//    * The QVariant<quintptr> + reinterpret_cast<QLabel*> dance was
//      replaced with a `reply → (label, cc)` map. The map entry
//      outlives the request — when the reply finishes, we look up
//      the map, do the work, and erase the entry. The reply is
//      deleted via deleteLater() as Qt expects.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef FLAGLOADER_H
#define FLAGLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QLabel>
#include <QMap>
#include <QString>
#include <QPointer>
#include <QHash>

class FlagLoader : public QObject
{
    Q_OBJECT

public:
    explicit FlagLoader(QObject *parent = nullptr);

    // ── Public API ─────────────────────────────────────────────────────────
    //  Asynchronously load the flag for the given ISO-3166 alpha-2
    //  country code and apply it to `label` (scaled to label->size()).
    //  Subsequent calls with the same cc hit the in-memory cache.
    //  Safe to call from the GUI thread; replies are marshalled back
    //  via QNetworkAccessManager.
    void loadFlag(const QString &cc, QLabel *label) noexcept;

    [[nodiscard]] QPixmap getFlagPixmap(const QString &cc) const noexcept;

private slots:
    void onReplyFinished();

private:
    // Per-request context. Keyed by QNetworkReply* (the raw pointer
    // is the unique handle; replies are deleted via deleteLater()
    // after the entry is erased, so no dangling key in the map).
    struct FlagRequest {
        QPointer<QLabel> label;     // QPointer → nullptr if label destroyed
        QString          cc;        // lower-case ISO-3166 alpha-2
    };

    // ── Cache (lower-case cc → QPixmap) ─────────────────────────────────────
    QMap<QString, QPixmap> flagCache;

    // ── In-flight requests ─────────────────────────────────────────────────
    QHash<QNetworkReply*, FlagRequest> mPending;

    // ── Network ───────────────────────────────────────────────────────────
    QNetworkAccessManager *manager{nullptr};

    // Template string for the flag URL. The only %1 placeholder is
    // the lower-case two-letter ISO-3166 code.
    static constexpr auto FLAG_URL_FMT =
        "https://flagcdn.com/w160/%1.png";

    // Compose a flag URL from a country code.
    [[nodiscard]] static QString flagUrl(const QString &lowerCc) noexcept;
};

#endif // FLAGLOADER_H
