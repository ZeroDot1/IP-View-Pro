// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.10.0 — PacketModule.h
//  C++26: noexcept, [[nodiscard]], std::optional, default member init
//  Active connection parser — reads /proc/net/tcp and /proc/net/udp (Item 47).
//  Provides structured connection data for live network monitoring.
//  Does NOT require root (parses world-readable procfs entries on Linux).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef PACKETMODULE_H
#define PACKETMODULE_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QString>
#include <QDateTime>

#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Packet {

// ── Connection state enum ─────────────────────────────────────────────────
enum class ConnectionState : uint8_t {
    Established   = 0x01,
    SynSent       = 0x02,
    SynReceived   = 0x03,
    FinWait1      = 0x04,
    FinWait2      = 0x05,
    TimeWait      = 0x06,
    Close         = 0x07,
    CloseWait     = 0x08,
    LastAck       = 0x09,
    Listen        = 0x0A,
    Closing       = 0x0B,
    Unknown       = 0xFF
};

// ── Connection entry ──────────────────────────────────────────────────────
struct ConnectionEntry {
    int         slot{0};               // kernel slot index
    QString     localAddress;
    uint16_t    localPort{0};
    QString     remoteAddress;
    uint16_t    remotePort{0};
    ConnectionState state{ConnectionState::Unknown};
    uint64_t    txQueue{0};
    uint64_t    rxQueue{0};
    uint64_t    uid{0};
    bool        isTCP{true};           // false for UDP entries
};

// ── Parsed snapshot ───────────────────────────────────────────────────────
struct ConnectionSnapshot {
    QDateTime                     timestamp;
    QList<ConnectionEntry>        tcpConnections;
    QList<ConnectionEntry>        udpConnections;
};

// ═══════════════════════════════════════════════════════════════════════════════
class PacketModule : public QObject
{
    Q_OBJECT

public:
    explicit PacketModule(QObject *parent = nullptr);
    ~PacketModule() override = default;

    // ── Manual poll ──────────────────────────────────────────────────────
    [[nodiscard]] ConnectionSnapshot pollNow();

    // ── Polling control ──────────────────────────────────────────────────
    void startPolling(int intervalMs = 5000);
    void stopPolling();
    [[nodiscard]] bool isPolling() const noexcept { return mTimer && mTimer->isActive(); }

    // ── Access ───────────────────────────────────────────────────────────
    [[nodiscard]] QList<ConnectionEntry> getTCP() const noexcept { return mLastTCP; }
    [[nodiscard]] QList<ConnectionEntry> getUDP() const noexcept { return mLastUDP; }

signals:
    void connectionsUpdated(const IPView::Packet::ConnectionSnapshot &snapshot);

private:
    // ── Parsers ──────────────────────────────────────────────────────────
    [[nodiscard]] static QList<ConnectionEntry> parseProcNet(const QString &path, bool isTCP);
    [[nodiscard]] static ConnectionEntry        parseLine(const QString &line, bool isTCP, int slot);
    [[nodiscard]] static QString                hexToIp(const QString &hex);
    [[nodiscard]] static uint16_t               hexToPort(const QString &hex);
    [[nodiscard]] static ConnectionState        tcpStateFromCode(uint8_t code);

    QTimer      *mTimer{nullptr};
    QList<ConnectionEntry> mLastTCP;
    QList<ConnectionEntry> mLastUDP;
};

} // namespace IPView::Packet

#endif // PACKETMODULE_H
