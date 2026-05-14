// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — WhoisManager.h
//  C++26: [[nodiscard]], const-correctness, default member init
//  Asynchronous Whois/RDAP queries with normalized JSON output.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef WHOISMANAGER_H
#define WHOISMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

class WhoisManager : public QObject
{
    Q_OBJECT

public:
    explicit WhoisManager(QObject *parent = nullptr);

    // ── Public API ─────────────────────────────────────────────────────────
    void lookup(const QString &ip, const QString &apiName) noexcept;

    [[nodiscard]] QStringList getAvailableApis() const noexcept;

signals:
    void lookupFinished(const QString &result);
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    // ── Normalization Helpers ──────────────────────────────────────────────
    [[nodiscard]] QJsonObject normalizeRdap(const QJsonObject &obj)  const noexcept;
    [[nodiscard]] QJsonObject normalizeIpApi(const QJsonObject &obj) const noexcept;

    // ── Network ──────────────────────────────────────────────────────────
    QNetworkAccessManager *manager{nullptr};
    QString currentApi;
};

#endif // WHOISMANAGER_H
