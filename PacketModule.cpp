// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.10.0 — PacketModule.cpp
//  C++26: QStringLiteral, std::from_chars, std::optional
//  Active connection parser — reads /proc/net/tcp and /proc/net/udp (Item 47).
//  Parses hex-encoded kernel connection tables into structured data.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "PacketModule.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

#include <array>
#include <charconv>
#include <system_error>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Packet {

// ── Proc paths ──────────────────────────────────────────────────────────────
inline constexpr auto PROC_TCP = "/proc/net/tcp";
inline constexpr auto PROC_TCP6 = "/proc/net/tcp6";
inline constexpr auto PROC_UDP = "/proc/net/udp";
inline constexpr auto PROC_UDP6 = "/proc/net/udp6";

// ═══════════════════════════════════════════════════════════════════════════════
PacketModule::PacketModule(QObject *parent)
    : QObject(parent)
{
    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout, this, [this]() {
        auto snap = pollNow();
        emit connectionsUpdated(snap);
    });
}

// ═══════════════════════════════════════════════════════════════════════════════
ConnectionSnapshot PacketModule::pollNow()
{
    ConnectionSnapshot snap;
    snap.timestamp = QDateTime::currentDateTime();

    snap.tcpConnections  = parseProcNet(QString::fromLatin1(PROC_TCP), true);
    snap.udpConnections  = parseProcNet(QString::fromLatin1(PROC_UDP), false);

    // Also try IPv6 variants silently (they may not exist on all kernels)
    auto tcp6 = parseProcNet(QString::fromLatin1(PROC_TCP6), true);
    auto udp6 = parseProcNet(QString::fromLatin1(PROC_UDP6), false);
    snap.tcpConnections.append(tcp6);
    snap.udpConnections.append(udp6);

    mLastTCP = snap.tcpConnections;
    mLastUDP = snap.udpConnections;

    return snap;
}

// ═══════════════════════════════════════════════════════════════════════════════
void PacketModule::startPolling(int intervalMs)
{
    if (intervalMs < 1000) intervalMs = 1000;  // minimum 1s
    mTimer->start(intervalMs);
}

void PacketModule::stopPolling()
{
    mTimer->stop();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  /proc/net/tcp and /proc/net/udp parser
//
//  Format (one line):
//    sl  local_address  rem_address   ... columns
//    0:  0100007F:0035  00000000:0000  ... data
//
//  IP addresses are in hex, little-endian byte order on the wire.
//  e.g. "0100007F" → bytes [0x01,0x00,0x00,0x7F] → reversed → 127.0.0.1
//  Ports are in hex, big-endian: "0035" → 53
// ═══════════════════════════════════════════════════════════════════════════════

QList<ConnectionEntry> PacketModule::parseProcNet(const QString &path, bool isTCP)
{
    QList<ConnectionEntry> result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return result;  // file may not exist (IPv6 variants)

    QTextStream stream(&file);
    int lineNo = 0;
    while (!stream.atEnd()) {
        QString const line = stream.readLine();
        ++lineNo;
        if (lineNo == 1) continue;  // skip header

        auto entry = parseLine(line.trimmed(), isTCP, lineNo - 2);
        if (entry.slot >= 0) {
            result.append(entry);
        }
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
ConnectionEntry PacketModule::parseLine(const QString &line, bool isTCP, int slot)
{
    ConnectionEntry entry;
    entry.slot  = slot;
    entry.isTCP = isTCP;

    if (line.isEmpty()) {
        entry.slot = -1;
        return entry;
    }

    // Split by whitespace
    QStringList const parts = line.split(QRegularExpression(QStringLiteral("\\s+")),
                                         Qt::SkipEmptyParts);
    // Minimum columns expected: 0:sl 1:local 2:rem 3:st 4:tx_queue:rx_queue 5:tr 6:tm->when 7:retrnsmt 8:uid ...
    if (parts.size() < 10) {
        entry.slot = -1;
        return entry;
    }

    // ── Local address:port (parts[1]) ────────────────────────────────────
    // Format: "0100007F:0035"
    QString const localPart = parts[1];
    int const colonPos = static_cast<int>(localPart.lastIndexOf(QLatin1Char(':')));
    if (colonPos > 0) {
        entry.localAddress = hexToIp(localPart.left(colonPos));
        entry.localPort    = hexToPort(localPart.mid(colonPos + 1));
    }

    // ── Remote address:port (parts[2]) ───────────────────────────────────
    QString const remPart = parts[2];
    int const remColon = static_cast<int>(remPart.lastIndexOf(QLatin1Char(':')));
    if (remColon > 0) {
        entry.remoteAddress = hexToIp(remPart.left(remColon));
        entry.remotePort    = hexToPort(remPart.mid(remColon + 1));
    }

    // ── State (parts[3]) ────────────────────────────────────────────────
    if (isTCP) {
        bool ok = false;
        uint8_t code = static_cast<uint8_t>(parts[3].toUInt(&ok, 16));
        if (ok) {
            entry.state = tcpStateFromCode(code);
        }
    } else {
        // UDP doesn't use the same state codes; set to N/A
        entry.state = ConnectionState::Established;
    }

    // ── TX/RX queue (parts[4]) ────────────────────────────────────────────
    // Format: "00000000:00000000"
    QString const queuePart = parts[4];
    int const queueColon = static_cast<int>(queuePart.indexOf(QLatin1Char(':')));
    if (queueColon > 0) {
        auto parseHex = [](const QString &s) -> uint64_t {
            QByteArray const ba = s.toUtf8();
            uint64_t val = 0;
            auto [ptr, ec] = std::from_chars(ba.data(), ba.data() + ba.size(), val, 16);
            if (ec == std::errc{}) return val;
            return 0;
        };
        entry.rxQueue = parseHex(queuePart.left(queueColon));
        entry.txQueue = parseHex(queuePart.mid(queueColon + 1));
    }

    // ── UID (parts[7]) ─────────────────────────────────────────────────────
    if (parts.size() > 7) {
        QByteArray const uidBa = parts[7].toUtf8();
        uint64_t uidVal = 0;
        auto [ptr, ec] = std::from_chars(uidBa.data(), uidBa.data() + uidBa.size(), uidVal);
        if (ec == std::errc{}) {
            entry.uid = uidVal;
        }
    }

    return entry;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  Hex IP conversion
//
//  /proc/net/tcp stores IPv4 as 8 hex chars in little-endian byte order.
//  For example: "0100007F" → bytes [0x01,0x00,0x00,0x7F]
//  Reading these as little-endian bytes gives: 127.0.0.1
//
//  For IPv6 (32 hex chars), the same byte-reversal applies per 4-byte group.
// ═══════════════════════════════════════════════════════════════════════════════

QString PacketModule::hexToIp(const QString &hex)
{
    if (hex.length() == 8) {
        // ── IPv4 ──────────────────────────────────────────────────────────
        // Parse hex string as 4 bytes, then reverse byte order
        std::array<uint8_t, 4> bytes{};
        for (auto i = std::size_t{0}; i < 4; ++i) {
            bool ok = false;
            bytes[i] = static_cast<uint8_t>(hex.mid(static_cast<int>(i * 2), 2).toUInt(&ok, 16));
            if (!ok) return hex;
        }
        // Reverse: [3],[2],[1],[0] → dotted decimal
        return QStringLiteral("%1.%2.%3.%4")
            .arg(bytes[3])
            .arg(bytes[2])
            .arg(bytes[1])
            .arg(bytes[0]);
    }
    else if (hex.length() == 32) {
        // ── IPv6 ──────────────────────────────────────────────────────────
        // /proc/net/tcp6 stores IPv6 as 32 hex chars (16 bytes), in 4 groups of 4 bytes,
        // each group is little-endian, and the groups themselves are in network order.
        // Simplified: parse 16 bytes in reverse group order
        QString result;
        for (int group = 0; group < 8; ++group) {
            if (group > 0 && group % 2 == 0) result += QLatin1Char(':');
            // Each group is 4 hex chars (2 bytes), little-endian
            int const off = group * 4;
            QString const groupHex = hex.mid(off, 4);
            result += groupHex.mid(2, 2) + groupHex.mid(0, 2);
        }
        return result.toLower();
    }

    // Unknown format — return as-is
    return hex;
}

// ═══════════════════════════════════════════════════════════════════════════════
uint16_t PacketModule::hexToPort(const QString &hex)
{
    if (hex.length() != 4 && hex.length() != 2) return 0;
    bool ok = false;
    uint16_t port = static_cast<uint16_t>(hex.toUInt(&ok, 16));
    return ok ? port : 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
ConnectionState PacketModule::tcpStateFromCode(uint8_t code)
{
    switch (code) {
        case 0x01: return ConnectionState::Established;
        case 0x02: return ConnectionState::SynSent;
        case 0x03: return ConnectionState::SynReceived;
        case 0x04: return ConnectionState::FinWait1;
        case 0x05: return ConnectionState::FinWait2;
        case 0x06: return ConnectionState::TimeWait;
        case 0x07: return ConnectionState::Close;
        case 0x08: return ConnectionState::CloseWait;
        case 0x09: return ConnectionState::LastAck;
        case 0x0A: return ConnectionState::Listen;
        case 0x0B: return ConnectionState::Closing;
        default:   return ConnectionState::Unknown;
    }
}

} // namespace IPView::Packet
