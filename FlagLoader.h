// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — FlagLoader.h
//  C++26: [[nodiscard]], const-correctness, default member init
//  Loads country flag images from flagpedia.net with in-memory cache.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef FLAGLOADER_H
#define FLAGLOADER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QLabel>
#include <QMap>

class FlagLoader : public QObject
{
    Q_OBJECT

public:
    explicit FlagLoader(QObject *parent = nullptr);

    // ── Public API ─────────────────────────────────────────────────────────
    void loadFlag(const QString &cc, QLabel *label) noexcept;

    [[nodiscard]] QPixmap getFlagPixmap(const QString &cc) const noexcept;

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    // ── Cache ─────────────────────────────────────────────────────────────
    QMap<QString, QPixmap> flagCache;

    // ── Network ───────────────────────────────────────────────────────────
    QNetworkAccessManager *manager{nullptr};
};

#endif // FLAGLOADER_H
