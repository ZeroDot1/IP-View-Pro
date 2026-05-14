// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — NetworkManager.h
//  C++26: [[nodiscard]], const-correctness, structured bindings
//  Manages asynchronous API requests to 12+ Geo-IP services.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>

#include <vector>

// ═══════════════════════════════════════════════════════════════════════════════
//  NetworkManager — Asynchronous Fetch and Failover for Geo-IP APIs
//
//  Architecture:
//    • 11 IPv4 APIs + 4 IPv6-capable APIs in prioritized order
//    • Automatic failover on failure or sparse data
//    • Per-request timeout via QNetworkRequest::setTransferTimeout
//    • Safety timeout via QTimer::singleShot (timeout × 1.5)
// ═══════════════════════════════════════════════════════════════════════════════
class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);

    // C++26: Destructors are implicitly noexcept
    ~NetworkManager() override = default;

    // ── Public API ─────────────────────────────────────────────────────────
    void fetchIPData() noexcept;
    void fetchIPv6Data() noexcept;

    void setSelectedAPI(int index) noexcept;

    [[nodiscard]] QStringList  getApiNames()  const noexcept;
    [[nodiscard]] QJsonObject  getLastData()  const noexcept;

    void setTimeout(int ms) noexcept;

signals:
    // C++26: Signal declarations remain Qt-specific
    void dataReceived(const QJsonObject &data);
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    // ── Internal Helper Methods ────────────────────────────────────────────
    void tryNextAPI()           noexcept;
    void tryNextIPv6API()       noexcept;
    void doRequest(const QUrl &url) noexcept;
    void advanceToNextApi()     noexcept;

    [[nodiscard]] QString getApiUrl(int index)      const noexcept;
    [[nodiscard]] QString getIPv6ApiUrl(int index)  const noexcept;

    // ── API Configuration ───────────────────────────────────────────────────
    //  QPair<DisplayName, URL>
    using ApiEntry = QPair<QString, QString>;
    std::vector<ApiEntry> apiList;
    std::vector<ApiEntry> ipv6ApiList;

    // ── State ─────────────────────────────────────────────────────────────
    int         currentApiIndex{0};
    int         currentIPv6ApiIndex{0};
    int         timeoutMs{10000};
    QJsonObject lastData;
    bool        isIPv6{false};

    // ── Network ──────────────────────────────────────────────────────────
    QNetworkAccessManager *manager{nullptr};
};

#endif // NETWORKMANAGER_H
