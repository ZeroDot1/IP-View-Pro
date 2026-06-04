// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — tests/test_security.cpp
//
//  Validates the IPView::Security helpers used across the project:
//  shell-metachar rejection, IPv4 / IPv6 / hostname / port
//  validation, and the isValidCommand() that SafeProcess uses.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "../SecurityUtil.h"

#include <QString>
#include <QHostAddress>

IPVIEW_TEST_CASE(security_rejects_shell_metachars,
    IPVIEW_CHECK(!isValidNetworkTarget(QStringLiteral("foo;rm -rf /")));
    IPVIEW_CHECK(!isValidNetworkTarget(QStringLiteral("a|b")));
    IPVIEW_CHECK(!isValidNetworkTarget(QStringLiteral("`whoami`")));
    IPVIEW_CHECK(!isValidNetworkTarget(QStringLiteral("$(date)"))))

IPVIEW_TEST_CASE(security_accepts_valid_ipv4,
    IPVIEW_CHECK(isValidNetworkTarget(QStringLiteral("1.1.1.1")));
    IPVIEW_CHECK(isValidNetworkTarget(QStringLiteral("192.168.1.1")));
    IPVIEW_CHECK(isValidNetworkTarget(QStringLiteral("8.8.8.8")))
)

IPVIEW_TEST_CASE(security_rejects_oversized_ipv4_octets,
    IPVIEW_CHECK(!isValidIPv4(QStringLiteral("256.1.1.1")));
    IPVIEW_CHECK(!isValidIPv4(QStringLiteral("999.0.0.0")))
)

IPVIEW_TEST_CASE(security_accepts_valid_ipv6,
    IPVIEW_CHECK(isValidNetworkTarget(QStringLiteral("::1")));
    IPVIEW_CHECK(isValidNetworkTarget(QStringLiteral("fe80::1")))
)

IPVIEW_TEST_CASE(security_accepts_hostnames,
    IPVIEW_CHECK(isValidNetworkTarget(QStringLiteral("example.com")));
    IPVIEW_CHECK(isValidNetworkTarget(QStringLiteral("a.b.c.d.e.f")))
)

IPVIEW_TEST_CASE(security_rejects_empty_input,
    IPVIEW_CHECK(!isValidNetworkTarget(QString{}));
    IPVIEW_CHECK(!isValidIPv4(QString{}));
    IPVIEW_CHECK(!isValidPort(QString{}))
)

IPVIEW_TEST_CASE(security_isValidCommand_accepts_paths,
    IPVIEW_CHECK(isValidCommand(QStringLiteral("/usr/bin/ping")));
    IPVIEW_CHECK(isValidCommand(QStringLiteral("ping")));
    IPVIEW_CHECK(isValidCommand(QStringLiteral("./speedtest-cli")))
)

IPVIEW_TEST_CASE(security_isValidCommand_rejects_injection,
    IPVIEW_CHECK(!isValidCommand(QStringLiteral("ping;rm -rf /")));
    IPVIEW_CHECK(!isValidCommand(QStringLiteral("a|b")));
    IPVIEW_CHECK(!isValidCommand(QStringLiteral("`ls`")));
    IPVIEW_CHECK(!isValidCommand(QStringLiteral("a b")))  // space
)

IPVIEW_TEST_CASE(security_port_range_enforced,
    IPVIEW_CHECK(isValidPort(QStringLiteral("80")));
    IPVIEW_CHECK(isValidPort(QStringLiteral("443")));
    IPVIEW_CHECK(!isValidPort(QStringLiteral("0")));
    IPVIEW_CHECK(!isValidPort(QStringLiteral("65536")));
    IPVIEW_CHECK(!isValidPort(QStringLiteral("-1")));
    IPVIEW_CHECK(!isValidPort(QStringLiteral("abc"))))
