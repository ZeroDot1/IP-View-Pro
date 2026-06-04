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

#include <chrono>
#include <utility>
#include <inplace_vector>   // C++26: stack-allocated fixed-capacity vector

#include "Timeouts.hpp"

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
    [[nodiscard]] int getSelectedApiIndex() const noexcept;

    [[nodiscard]] QStringList  getApiNames()  const noexcept;
    [[nodiscard]] QJsonObject  getLastData()  const noexcept;

    void setTimeout(int ms) noexcept;

signals:
    // C++26: Signal declarations remain Qt-specific
    void dataReceived(const QJsonObject &data);
    void errorOccurred(const QString &error);
    void requestFailed(const QUrl &url, const QString &reason);

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
    //  C++26: std::pair replaces QPair; first = display name, second = URL.
    //
    //  Storage is std::inplace_vector<..., N> with N chosen to
    //  cover every entry in the static IPv4_APIS / IPv6_APIS
    //  tables. The two arrays are bounded at compile time and
    //  never grow at runtime, so the heap allocation that a
    //  std::vector would do at construction is pure waste.
    //  inplace_vector keeps the storage inline (on the
    //  NetworkManager's stack frame / heap block, no extra
    //  allocation), and std::vector<>'s growth-doubling
    //  reallocation path simply cannot fire.
    using ApiEntry = std::pair<QString, QString>;
    static constexpr std::size_t kIPv4ApiCapacity = 16;
    static constexpr std::size_t kIPv6ApiCapacity = 8;
    std::inplace_vector<ApiEntry, kIPv4ApiCapacity> apiList;
    std::inplace_vector<ApiEntry, kIPv6ApiCapacity> ipv6ApiList;

    // ── State ─────────────────────────────────────────────────────────────
    int         currentApiIndex{0};
    int         currentIPv6ApiIndex{0};
    int         timeoutMs{static_cast<int>(IPView::Timeouts::HTTP_DEFAULT.count())};
    QJsonObject lastData;
    bool        isIPv6{false};

    // ── Network ──────────────────────────────────────────────────────────
    QNetworkAccessManager *manager{nullptr};
};

#endif // NETWORKMANAGER_H
