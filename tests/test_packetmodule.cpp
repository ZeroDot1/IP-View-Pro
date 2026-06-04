// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — test_packetmodule.cpp
//
//  Unit tests for PacketModule's /proc/net/tcp and /proc/net/udp
//  parsers. We synthesise tiny fake /proc files on disk (the real
//  /proc/net/tcp is read-only and varies between hosts) and point
//  parseProcNet() at them. parseProcNet is a static method so no
//  QObject setup is required.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "PacketModule.h"

#include <QFile>
#include <QString>
#include <QDir>
#include <QTemporaryFile>

using IPView::Packet::PacketModule;
using IPView::Packet::ConnectionState;

namespace {

// Write `content` to a fresh temp file and return the path.
[[nodiscard]] QString writeTempFile(const QString &content)
{
    auto *tmp = new QTemporaryFile;
    (void)tmp->open();
    tmp->write(content.toUtf8());
    tmp->close();
    return tmp->fileName();
}

} // namespace

IPVIEW_TEST_CASE(parseProcNet_skips_header_and_returns_empty_for_empty_body,
    auto const path = writeTempFile(
        QStringLiteral("  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"));
    auto const entries = PacketModule::parseProcNet(path, true);
    IPVIEW_CHECK_EQ(static_cast<int>(entries.size()), 0);
)

IPVIEW_TEST_CASE(parseProcNet_parses_single_tcp_listen_entry,
    auto const path = writeTempFile(QStringLiteral(
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 00000000:1F90 00000000:0000 0A 00000000:00000000 00:00000000 00000000     0        0 12345 1 0000000000000000 100 0 0 10 0\n"));
    auto const entries = PacketModule::parseProcNet(path, true);
    IPVIEW_CHECK_EQ(static_cast<int>(entries.size()), 1);
    if (entries.size() == 1) {
        IPVIEW_CHECK_EQ(entries[0].localPort,  8080);
        IPVIEW_CHECK_EQ(entries[0].remotePort, 0);
        IPVIEW_CHECK(entries[0].state == ConnectionState::Listen);
        IPVIEW_CHECK(entries[0].isTCP);
    }
)

IPVIEW_TEST_CASE(parseProcNet_decodes_ipv4_loopback_in_little_endian,
    // "0100007F" — bytes 01,00,00,7F — reversed → 127.0.0.1
    auto const path = writeTempFile(QStringLiteral(
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0035 00000000:0000 0A 00000000:00000000 00:00000000 00000000   975        0 99999 1 0000000000000000 100 0 0 10 5\n"));
    auto const entries = PacketModule::parseProcNet(path, true);
    IPVIEW_CHECK_EQ(static_cast<int>(entries.size()), 1);
    if (entries.size() == 1) {
        IPVIEW_CHECK_EQ(entries[0].localAddress, QStringLiteral("127.0.0.1"));
        IPVIEW_CHECK_EQ(static_cast<int>(entries[0].localPort),    53);
        IPVIEW_CHECK_EQ(static_cast<int>(entries[0].uid),          975);
    }
)

IPVIEW_TEST_CASE(parseProcNet_parses_established_state,
    // State code 0x01 → Established
    auto const path = writeTempFile(QStringLiteral(
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:C000 0200007F:01BB 01 00000000:00000000 00:00000000 00000000  1000        0 55555 1 0000000000000000 100 0 0 10 5\n"));
    auto const entries = PacketModule::parseProcNet(path, true);
    IPVIEW_CHECK_EQ(static_cast<int>(entries.size()), 1);
    if (entries.size() == 1) {
        IPVIEW_CHECK(entries[0].state == ConnectionState::Established);
        IPVIEW_CHECK_EQ(entries[0].localPort,  49152);
        IPVIEW_CHECK_EQ(entries[0].remotePort, 443);
    }
)

IPVIEW_TEST_CASE(parseProcNet_returns_empty_for_missing_file,
    auto const entries = PacketModule::parseProcNet(
        QStringLiteral("/nonexistent/proc/net/tcp"), true);
    IPVIEW_CHECK_EQ(static_cast<int>(entries.size()), 0);
)

IPVIEW_TEST_CASE(parseProcNet_skips_malformed_lines,
    // Line with too few columns must be rejected, not crash.
    auto const path = writeTempFile(QStringLiteral(
        "  sl  local_address rem_address   st tx_queue rx_queue tr tm->when retrnsmt   uid  timeout inode\n"
        "   0: 0100007F:0035 truncated\n"
        "   1: 0200007F:0036 00000000:0000 0A 00000000:00000000 00:00000000 00000000   500        0 22222 1 0000000000000000 100 0 0 10 0\n"));
    auto const entries = PacketModule::parseProcNet(path, true);
    IPVIEW_CHECK_EQ(static_cast<int>(entries.size()), 1);
    if (entries.size() == 1) {
        // Slot 1 line is well-formed and must survive.
        IPVIEW_CHECK_EQ(entries[0].slot, 1);
        IPVIEW_CHECK_EQ(entries[0].localAddress, QStringLiteral("127.0.0.2"));
    }
)
