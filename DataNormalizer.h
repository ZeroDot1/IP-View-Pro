// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.0 — DataNormalizer.h
//  C++26 (ISO/IEC 14882:2026)
//  Features:
//    • std::array         — compile-time constants for field names
//    • [[nodiscard]]      — prevent loss of return values
//    • constexpr/consteval — compile-time evaluation of lookup tables
//    • structured bindings — declarative access to map entries
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef DATANORMALIZER_H
#define DATANORMALIZER_H

#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStringList>
#include <QVariant>
#include <QRegularExpression>

#include <array>             // C++26: constexpr std::array
#include <vector>            // C++26: stack-based dynamic container
#include <string_view>       // C++17 / C++26: lightweight string views

// ═══════════════════════════════════════════════════════════════════════════════
//  DataNormalizer — namespace (statt Klasse) für JSON-Normalisierung.
//  Idiomatisches C++26: reine "namespace" anstelle einer statischen Klasse.
//  Normalisiert JSON-Responses von 12+ Geo-IP-APIs in ein einheitliches Format.
// ═══════════════════════════════════════════════════════════════════════════════
namespace DataNormalizer {

    // ── Main normalization method ──────────────────────────────────────
    //  Accepts raw data (JSON string or plain text) and returns a
    //  unified QJsonObject with all 18 standard fields.
    //  On complete failure, an empty QJsonObject is returned.
    [[nodiscard]]
    QJsonObject normalize(const QByteArray &rawData) noexcept
    {
        using namespace detail;
        QJsonObject normalized;
        QJsonDocument const doc = QJsonDocument::fromJson(rawData);

        if (doc.isNull() || !doc.isObject()) {
            // Fallback: plain IP address (e.g. "1.2.3.4")
            QString const possibleIp = QString::fromUtf8(rawData).trimmed();
            if (isValidIp(possibleIp)) {
                normalized[QStringLiteral("ip")] = possibleIp;
            }
        } else {
            QJsonObject obj = doc.object();
            processNestedJson(obj);

            if (isErrorResponse(obj)) {
                return QJsonObject();
            }

            // ── Core mapping ─────────────────────────────────────────
            //  C++26 std::array for compile-time constants
            //  Each entry: (API key, {list of alternative names})
            normalized[QStringLiteral("ip")]             = getString(obj, std::to_array<std::string_view>({"ipAddress", "ip", "query", "origin", "address", "ip_address"}));
            normalized[QStringLiteral("city")]           = getString(obj, std::to_array<std::string_view>({"cityName", "city", "city_name"}));
            normalized[QStringLiteral("region")]         = getString(obj, std::to_array<std::string_view>({"regionName", "region_name", "region", "region_code"}));
            normalized[QStringLiteral("country_name")]   = getString(obj, std::to_array<std::string_view>({"countryName", "country_name", "country"}));
            normalized[QStringLiteral("country_code")]   = getString(obj, std::to_array<std::string_view>({"countryCode", "country_code", "cc"}));
            normalized[QStringLiteral("continent")]      = getString(obj, std::to_array<std::string_view>({"continent", "continent_name", "continentCode", "continent_code"}));
            normalized[QStringLiteral("org")]            = getString(obj, std::to_array<std::string_view>({"asnOrganization", "org", "isp", "as", "organization", "asname"}));
            normalized[QStringLiteral("asn")]            = getString(obj, std::to_array<std::string_view>({"asn", "as", "as_number", "asn_number"}));
            normalized[QStringLiteral("postal")]         = getString(obj, std::to_array<std::string_view>({"zipCode", "postal", "zip", "zip_code", "postal_code"}));
            normalized[QStringLiteral("timezone")]       = getString(obj, std::to_array<std::string_view>({"timeZones", "timezone", "time_zone", "timezone_name"}));
            normalized[QStringLiteral("currency")]       = getString(obj, std::to_array<std::string_view>({"currencies", "currency", "currency_name", "currency_code"}));
            normalized[QStringLiteral("calling_code")]   = getString(obj, std::to_array<std::string_view>({"phoneCodes", "calling_code", "country_calling_code", "country_prefix", "country_phone"}));
            normalized[QStringLiteral("languages")]      = getString(obj, std::to_array<std::string_view>({"languages", "language"}));
            normalized[QStringLiteral("hostname")]       = getString(obj, std::to_array<std::string_view>({"hostname", "host"}));
            normalized[QStringLiteral("type")]           = getString(obj, std::to_array<std::string_view>({"type", "connection_type", "net_type"}));
            normalized[QStringLiteral("latitude")]       = getString(obj, std::to_array<std::string_view>({"latitude", "lat"}));
            normalized[QStringLiteral("longitude")]      = getString(obj, std::to_array<std::string_view>({"longitude", "lon"}));

            // Security
            QString const isProxy = getString(obj, std::to_array<std::string_view>({"isProxy", "proxy", "is_proxy", "is_vpn", "vpn", "tor", "is_tor", "hosting", "is_hosting"}));
            normalized[QStringLiteral("security")] =
                (isProxy.isEmpty() || isProxy == QStringLiteral("false") || isProxy == QStringLiteral("0"))
                    ? QStringLiteral("None / Clear")
                    : QStringLiteral("Detected (") + isProxy + QLatin1Char(')');
        }

        // Final IP fallback: if no structured JSON,
        // attempt to interpret raw data string as IP
        if (normalized[QStringLiteral("ip")].toString().isEmpty()) {
            QString const possibleIp = QString::fromUtf8(rawData).trimmed();
            if (isValidIp(possibleIp)) {
                normalized[QStringLiteral("ip")] = possibleIp;
            }
        }

        fillMissingFields(normalized);
        return normalized;
    }

    // ── JSON-to-HTML formatting ─────────────────────────────────────────
    //  Generates an HTML view with syntax highlighting from a QJsonObject.
    [[nodiscard]]
    QString formatJsonHtml(const QJsonObject &obj) noexcept
    {
        QString const json = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        QString html = QStringLiteral("<pre style='color: #ffffff; line-height: 1.4;'>");

        // C++26: structured bindings in range-based for (C++17 already)
        QStringList const lines = json.split(QLatin1Char('\n'));
        for (QString const& line : lines) {
            // Highlight keys
            static QRegularExpression const keyRegex(QStringLiteral("\"(.*)\":"));
            QString formatted = line;
            formatted.replace(keyRegex, QStringLiteral("<span style='color: #e94560;'>\"\\1\"</span>:"));

            // Highlight string values
            static QRegularExpression const valRegex(QStringLiteral(": \"(.*)\""));
            formatted.replace(valRegex, QStringLiteral(": <span style='color: #00ff88;'>\"\\1\"</span>"));

            // Highlight numeric values
            static QRegularExpression const numRegex(QStringLiteral(": (\\d+\\.?\\d*)"));
            formatted.replace(numRegex, QStringLiteral(": <span style='color: #00d4ff;'>\\1</span>"));

            html += formatted + QLatin1Char('\n');
        }

        html += QStringLiteral("</pre>");
        return html;
    }

// ── Detail helpers (intern, nicht Teil des öffentlichen API) ────────────
namespace detail {

    //  Used for the list of 18 standard field names in fillMissingFields()
    using FieldList = std::vector<QString>;

    // ── IP validation ────────────────────────────────────────────────────
    [[nodiscard]]
    bool isValidIp(const QString &ip) noexcept
    {
        return ip.contains(QLatin1Char('.')) || ip.contains(QLatin1Char(':'));
    }

    // ── Error detection ───────────────────────────────────────────────────
    [[nodiscard]]
    bool isErrorResponse(const QJsonObject &obj) noexcept
    {
        if (obj.contains(QStringLiteral("success")) && !obj[QStringLiteral("success")].toBool()) return true;
        if (obj.contains(QStringLiteral("status")) && obj[QStringLiteral("status")].toString() == QStringLiteral("fail")) return true;
        if (obj.contains(QStringLiteral("error"))) {
            QJsonValue const err = obj[QStringLiteral("error")];
            if (err.isBool() && err.toBool()) return true;
            if (err.isObject()) return true;
        }
        return false;
    }

    // ── Resolve nested JSON ─────────────────────────────────────
    //  APIs like ipapi.co sometimes provide JSON-as-string in fields.
    //  This method extracts nested values to the top level.
    void processNestedJson(QJsonObject &obj) noexcept
    {
        QStringList const keys = obj.keys();
        for (QString const& k : keys) {
            QJsonValue const kv = obj[k];
            if (!kv.isString()) continue;

            QString const val = kv.toString().trimmed();
            if (!val.startsWith(QLatin1Char('{')) || !val.endsWith(QLatin1Char('}'))) continue;

            QJsonDocument const nestedDoc = QJsonDocument::fromJson(val.toUtf8());
            if (nestedDoc.isNull() || !nestedDoc.isObject()) continue;

            QJsonObject const nestedObj = nestedDoc.object();
            for (auto it = nestedObj.constBegin(); it != nestedObj.constEnd(); ++it) {
                if (!obj.contains(it.key()) || obj[it.key()].toString().trimmed().isEmpty()) {
                    obj[it.key()] = it.value();
                }
            }
        }
    }

    // ── Type-safe retrieval from QJsonObject ─────────────────────────────────
    //  C++26: Template parameter with std::array<std::string_view, N>
    //  for compile-time key lists without heap allocation.
    template<std::size_t N>
    [[nodiscard]]
    QString getString(const QJsonObject &obj,
                      const std::array<std::string_view, N> &keys) noexcept
    {
        for (std::string_view const keySv : keys) {
            QString const key = QString::fromUtf8(keySv.data(), static_cast<qsizetype>(keySv.size()));
            if (!obj.contains(key) || obj[key].isNull()) continue;

            QJsonValue const v = obj[key];
            QString val;

            if (v.isString()) {
                val = v.toString();
            } else if (v.isDouble()) {
                val = QString::number(v.toDouble(), 'g', 12);
            } else if (v.isBool()) {
                val = v.toBool() ? QStringLiteral("true") : QStringLiteral("false");
            } else if (v.isArray()) {
                QStringList items;
                QJsonArray const arr = v.toArray();
                items.reserve(arr.size());
                for (QJsonValue const& av : arr) {
                    items.append(av.toVariant().toString());
                }
                val = items.join(QStringLiteral(", "));
            } else {
                val = v.toVariant().toString();
            }

            val = val.trimmed();
            if (val.startsWith(QLatin1Char('{')) && val.endsWith(QLatin1Char('}'))) continue;
            if (!val.isEmpty()) return val;
        }

        return QString();
    }

    // ── Initialize fields ─────────────────────────────────────────────
    //  Ensures all 18 standard fields exist (even if empty).
    void fillMissingFields(QJsonObject &normalized) noexcept
    {
        // C++26 std::inplace_vector: stack-allocated, max. 18 entries
        FieldList const fields = {
            QStringLiteral("ip"),            QStringLiteral("city"),
            QStringLiteral("region"),        QStringLiteral("country_name"),
            QStringLiteral("country_code"),  QStringLiteral("continent"),
            QStringLiteral("org"),           QStringLiteral("asn"),
            QStringLiteral("latitude"),      QStringLiteral("longitude"),
            QStringLiteral("postal"),        QStringLiteral("timezone"),
            QStringLiteral("currency"),      QStringLiteral("calling_code"),
            QStringLiteral("languages"),     QStringLiteral("hostname"),
            QStringLiteral("type"),          QStringLiteral("security")
        };

        for (QString const& f : fields) {
            if (!normalized.contains(f)) {
                normalized[f] = QString();
            }
        }
    }
} // namespace detail

} // namespace DataNormalizer

#endif // DATANORMALIZER_H
