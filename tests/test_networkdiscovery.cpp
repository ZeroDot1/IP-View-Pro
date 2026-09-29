// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.5 — test_networkdiscovery.cpp
//  Unit tests for the public static helpers of NetworkDiscovery:
//   • parseSubnet()       — subnet string → SubnetRange
//   • detectLocalSubnets()— QNetworkInterface-based local subnets
//  No Qt event loop is started; tests are pure-logic.
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "NetworkDiscovery.h"

using IPView::Scanner::NetworkDiscovery;

// ═══════════════════════════════════════════════════════════════════════════════

IPVIEW_TEST_CASE(parseSubnet_accepts_a_b_c,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("192.168.1"));
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_REQUIRE(r->base == QStringLiteral("192.168.1"));
    IPVIEW_REQUIRE(r->firstHost == 1);
    IPVIEW_REQUIRE(r->lastHost  == 254);
)

IPVIEW_TEST_CASE(parseSubnet_accepts_cidr,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("10.0.0.42/24"));
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_REQUIRE(r->base      == QStringLiteral("10.0.0"));
    IPVIEW_REQUIRE(r->firstHost == 1);
    IPVIEW_REQUIRE(r->lastHost  == 254);
)

IPVIEW_TEST_CASE(parseSubnet_cidr_uses_the_network_address_not_the_input_host,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("192.168.1.42/24"));
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_REQUIRE(r->base == QStringLiteral("192.168.1"));
    IPVIEW_REQUIRE(r->firstHost == 1);
    IPVIEW_REQUIRE(r->lastHost == 254);
)

IPVIEW_TEST_CASE(parseSubnet_rejects_network_and_broadcast_as_single_hosts,
    IPVIEW_REQUIRE(!NetworkDiscovery::parseSubnet(QStringLiteral("10.0.0.0")).has_value());
    IPVIEW_REQUIRE(!NetworkDiscovery::parseSubnet(QStringLiteral("10.0.0.255")).has_value());
)

IPVIEW_TEST_CASE(parseSubnet_cidr_network_ending_in_zero_is_not_a_single_host,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("10.0.0.42/24"));
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_REQUIRE(r->firstHost == 1);
    IPVIEW_REQUIRE(r->lastHost == 254);
)

IPVIEW_TEST_CASE(parseSubnet_accepts_narrow_cidr,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("192.168.1.5/30"));
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_REQUIRE(r->base == QStringLiteral("192.168.1"));
    IPVIEW_REQUIRE(r->firstHost == 5);
    IPVIEW_REQUIRE(r->lastHost == 6);
)

IPVIEW_TEST_CASE(parseSubnet_rejects_empty_octets,
    IPVIEW_REQUIRE(!NetworkDiscovery::parseSubnet(QStringLiteral("10..0.2")).has_value());
)

IPVIEW_TEST_CASE(parseSubnet_accepts_point_to_point_cidr,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("10.0.0.0/31"));
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_REQUIRE(r->firstHost == 0);
    IPVIEW_REQUIRE(r->lastHost == 1);
)

IPVIEW_TEST_CASE(parseSubnet_4octet_specific_host,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("10.0.0.42"));
    IPVIEW_REQUIRE(r.has_value());
    IPVIEW_REQUIRE(r->base      == QStringLiteral("10.0.0"));
    IPVIEW_REQUIRE(r->firstHost == 42);
    IPVIEW_REQUIRE(r->lastHost  == 42);
)

IPVIEW_TEST_CASE(parseSubnet_rejects_empty,
    auto r = NetworkDiscovery::parseSubnet(QString());
    IPVIEW_REQUIRE(!r.has_value());
)

IPVIEW_TEST_CASE(parseSubnet_rejects_garbage,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("not.an.ip"));
    IPVIEW_REQUIRE(!r.has_value());
)

IPVIEW_TEST_CASE(parseSubnet_rejects_out_of_range,
    auto r = NetworkDiscovery::parseSubnet(QStringLiteral("256.0.0"));
    IPVIEW_REQUIRE(!r.has_value());
)

IPVIEW_TEST_CASE(detectLocalSubnets_is_nonempty,
    // On any reasonably configured Linux CI runner the loopback
    // interface is present, so detectLocalSubnets() must list at
    // least one subnet ("127.0.0" on Linux).
    QStringList const subs = NetworkDiscovery::detectLocalSubnets();
    IPVIEW_REQUIRE(!subs.isEmpty());
)
