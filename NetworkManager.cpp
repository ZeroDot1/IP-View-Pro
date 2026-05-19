// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — NetworkManager.cpp
//  C++26: structured bindings, std::array, [[nodiscard]]
//  Asynchronous API failover with timeout and sparse-data detection
// ═══════════════════════════════════════════════════════════════════════════════

#include "NetworkManager.h"
#include "DataNormalizer.h"
#include "SecurityUtil.h"

#include <QUrl>
#include <QDebug>
#include <QTimer>
#include <QNetworkRequest>

#include <array>     // C++26: constexpr API lists
#include <utility>   // std::pair

// ═══════════════════════════════════════════════════════════════════════════════
//  API Configuration — constexpr-like via std::array
//  C++26: structured bindings, const-correctness
// ═══════════════════════════════════════════════════════════════════════════════

// ── IPv4 APIs (prioritized by detail depth) ─────────────────────────────────
static constexpr auto IPv4_APIS = std::to_array<std::pair<std::string_view, std::string_view>>({
    {"IPWhois.is",          "http://ipwho.is/"},
    {"FreeIPAPI",           "https://freeipapi.com/api/json/"},
    {"IP-API (Detailed)",   "http://ip-api.com/json/?fields=66846719"},
    {"IPAPI.co",            "https://ipapi.co/json/"},
    {"IPInfo",              "https://ipinfo.io/json"},
    {"IPWhois.app",         "https://ipwhois.app/json/"},
    {"IP.sb",               "https://api.ip.sb/geoip"},
    {"SeeIP",               "https://api.seeip.org/geoip"},
    {"MyIP",                "https://api.myip.com/"},
    {"HTTPBin",             "https://httpbin.org/ip"},
    {"Amazon CheckIP",      "https://checkip.amazonaws.com/"},
});

// ── IPv6 APIs ────────────────────────────────────────────────────────────────
static constexpr auto IPv6_APIS = std::to_array<std::pair<std::string_view, std::string_view>>({
    {"IPWhois.is IPv6",     "http://ipwho.is/"},
    {"IPify IPv6",          "https://api6.ipify.org?format=json"},
    {"IPify64",             "https://api64.ipify.org?format=json"},
    {"IP-API IPv6",         "http://ip-api.com/json/?fields=66846719"},
});

// ═══════════════════════════════════════════════════════════════════════════════
NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , manager(new QNetworkAccessManager(this))
{
    // C++26 structured bindings for constexpr array unpacking
    // Copy compile-time APIs into runtime containers
    for (auto const& [name, url] : IPv4_APIS) {
        apiList.emplace_back(
            QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())),
            QString::fromUtf8(url.data(),  static_cast<qsizetype>(url.size()))
        );
    }
    for (auto const& [name, url] : IPv6_APIS) {
        ipv6ApiList.emplace_back(
            QString::fromUtf8(name.data(), static_cast<qsizetype>(name.size())),
            QString::fromUtf8(url.data(),  static_cast<qsizetype>(url.size()))
        );
    }

    // Per-reply connections (no manager-level finished signal — Qt 5.15+)
}

// ═══════════════════════════════════════════════════════════════════════════════
void NetworkManager::setSelectedAPI(int index) noexcept
{
    if (index >= 0 && index < static_cast<int>(apiList.size())) {
        currentApiIndex = index;
    }
}

int NetworkManager::getSelectedApiIndex() const noexcept
{
    return currentApiIndex;
}

[[nodiscard]]
QStringList NetworkManager::getApiNames() const noexcept
{
    QStringList names;
    names.reserve(static_cast<qsizetype>(apiList.size() + ipv6ApiList.size() + 1));

    for (auto const& [name, _url] : apiList) {  // structured bindings
        names.append(name);
    }
    names.append(QStringLiteral("--- IPv6 ---"));
    for (auto const& [name, _url] : ipv6ApiList) {
        names.append(name);
    }
    return names;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Data Fetching
// ═══════════════════════════════════════════════════════════════════════════════

void NetworkManager::fetchIPData() noexcept
{
    isIPv6 = false;
    currentApiIndex = 0;
    tryNextAPI();
}

void NetworkManager::fetchIPv6Data() noexcept
{
    isIPv6 = true;
    currentIPv6ApiIndex = 0;
    tryNextIPv6API();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Internal API Failover
// ═══════════════════════════════════════════════════════════════════════════════

void NetworkManager::tryNextAPI() noexcept
{
    if (currentApiIndex >= static_cast<int>(apiList.size())) {
        QString const errorMsg = QStringLiteral("All IPv4 APIs failed. Try IPv6 or check your connection.");
        qDebug().noquote() << errorMsg;
        emit errorOccurred(errorMsg);
        return;
    }

    QUrl const url(getApiUrl(currentApiIndex));
    if (url.isEmpty()) {
        ++currentApiIndex;
        tryNextAPI();
        return;
    }

    doRequest(url);
}

void NetworkManager::tryNextIPv6API() noexcept
{
    if (currentIPv6ApiIndex >= static_cast<int>(ipv6ApiList.size())) {
        QString const errorMsg = QStringLiteral("All IPv6 APIs failed. Try IPv4 or check your connection.");
        qDebug().noquote() << errorMsg;
        emit errorOccurred(errorMsg);
        return;
    }

    QUrl const url(getIPv6ApiUrl(currentIPv6ApiIndex));
    if (url.isEmpty()) {
        ++currentIPv6ApiIndex;
        tryNextIPv6API();
        return;
    }

    doRequest(url);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Common Request Helper
// ═══════════════════════════════════════════════════════════════════════════════

void NetworkManager::doRequest(const QUrl &url) noexcept
{

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", QByteArrayLiteral("IPView/2.0"));
    request.setTransferTimeout(timeoutMs);

    QNetworkReply * const reply = manager->get(request);

    // ── Security: Strict SSL validation (prevents MITM) ──────────────
    enforceStrictSsl(reply);

    // Per-reply connection (modern, type-safe)
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });

    // Safety timeout (in case Transfer-Timeout does not trigger)
    QTimer::singleShot(static_cast<int>(timeoutMs * 1.5), reply, [reply]() {
        if (reply && !reply->isFinished()) {
            reply->abort();
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
//  URL Queries (const-correct)
// ═══════════════════════════════════════════════════════════════════════════════

[[nodiscard]]
QString NetworkManager::getApiUrl(int index) const noexcept
{
    if (index >= 0 && index < static_cast<int>(apiList.size())) {
        return apiList[static_cast<std::size_t>(index)].second;
    }
    return QString();
}

[[nodiscard]]
QString NetworkManager::getIPv6ApiUrl(int index) const noexcept
{
    if (index >= 0 && index < static_cast<int>(ipv6ApiList.size())) {
        return ipv6ApiList[static_cast<std::size_t>(index)].second;
    }
    return QString();
}

// ═══════════════════════════════════════════════════════════════════════════════
void NetworkManager::setTimeout(int ms) noexcept
{
    timeoutMs = ms;
}

[[nodiscard]]
QJsonObject NetworkManager::getLastData() const noexcept
{
    return lastData;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Reply Evaluation
// ═══════════════════════════════════════════════════════════════════════════════

void NetworkManager::onReplyFinished(QNetworkReply *reply)
{

    if (reply->error() != QNetworkReply::NoError) {
        QString const errorMsg = QStringLiteral("API failed: %1").arg(reply->errorString());
        qDebug().noquote() << errorMsg;

        advanceToNextApi();
        emit errorOccurred(errorMsg);
        reply->deleteLater();
        return;
    }

    QByteArray const rawData = reply->readAll();
    QJsonObject const normalized = DataNormalizer::normalize(rawData);

    if (normalized.isEmpty()) {
        advanceToNextApi();
        reply->deleteLater();
        return;
    }

    // ── Sparse-Data Check ─────────────────────────────────────────────────
    //  If City or Org are missing, try the next API for enrichment
    bool const isSparse = normalized[QStringLiteral("city")].toString().isEmpty()
                       || normalized[QStringLiteral("org")].toString().isEmpty();

    if (isSparse && !normalized[QStringLiteral("ip")].toString().isEmpty()) {
        int const nextIndex = isIPv6 ? currentIPv6ApiIndex + 1 : currentApiIndex + 1;
        int const listSize  = isIPv6 ? static_cast<int>(ipv6ApiList.size())
                                     : static_cast<int>(apiList.size());

        if (nextIndex < listSize) {
            qDebug().noquote() << "Data is sparse, trying next API for enrichment...";
            advanceToNextApi();
            reply->deleteLater();
            return;
        }
    }

    // ── Success ────────────────────────────────────────────────────────────
    lastData = normalized;
    emit dataReceived(normalized);
    reply->deleteLater();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Failover Helper
// ═══════════════════════════════════════════════════════════════════════════════

void NetworkManager::advanceToNextApi() noexcept
{
    if (!isIPv6) {
        ++currentApiIndex;
        tryNextAPI();
    } else {
        ++currentIPv6ApiIndex;
        tryNextIPv6API();
    }
}
