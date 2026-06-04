// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — tests/test_timeouts.cpp
//
//  Validates the Timeouts.hpp constants: types, sign, and that the
//  values are within sensible bounds for a desktop app.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "../Timeouts.hpp"

#include <chrono>
#include <type_traits>

IPVIEW_TEST_CASE(timeouts_constants_have_chrono_types,
    // Note: inline constexpr values are const-qualified, so we
    // strip the const before comparing.
    static_assert(std::is_same_v<
        std::remove_cvref_t<decltype(IPView::Timeouts::HTTP_DEFAULT)>,
        std::chrono::milliseconds>);
    static_assert(std::is_same_v<
        std::remove_cvref_t<decltype(IPView::Timeouts::TCP_CONNECT)>,
        std::chrono::milliseconds>);
    static_assert(std::is_same_v<
        std::remove_cvref_t<decltype(IPView::Timeouts::COOLDOWN_DEFAULT)>,
        std::chrono::seconds>)
)

IPVIEW_TEST_CASE(timeouts_http_default_is_between_one_and_thirty_seconds,
    auto const ms = IPView::Timeouts::HTTP_DEFAULT.count();
    IPVIEW_CHECK(ms >= 1000);
    IPVIEW_CHECK(ms <= 30000)
)

IPVIEW_TEST_CASE(timeouts_cooldown_is_at_least_thirty_seconds,
    auto const s = IPView::Timeouts::COOLDOWN_DEFAULT.count();
    IPVIEW_CHECK(s >= 30);
    IPVIEW_CHECK(s <= 3600)
)

IPVIEW_TEST_CASE(timeouts_tcp_connect_is_at_most_ten_seconds,
    auto const ms = IPView::Timeouts::TCP_CONNECT.count();
    IPVIEW_CHECK(ms >= 100);
    IPVIEW_CHECK(ms <= 10000)
)

IPVIEW_TEST_CASE(timeouts_polling_intervals_are_at_least_500ms,
    IPVIEW_CHECK(IPView::Timeouts::POLL_PACKET_DEFAULT.count() >= 500);
    IPVIEW_CHECK(IPView::Timeouts::POLL_TELEMETRY_DEFAULT.count() >= 500)
)
