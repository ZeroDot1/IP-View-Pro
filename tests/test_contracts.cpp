// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — tests/test_contracts.cpp
//
//  Validates that the IPVIEW_PRE / IPVIEW_POST / IPVIEW_ASSERT
//  macros throw std::logic_error when the underlying expression is
//  false, and do not throw when it is true.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "../Contracts.hpp"

#include <stdexcept>
#include <QString>

IPVIEW_TEST_CASE(contracts_pre_throws_on_false,
    IPVIEW_CHECK_THROWS_AS(IPVIEW_PRE(false), std::logic_error)
)

IPVIEW_TEST_CASE(contracts_pre_does_not_throw_on_true,
    IPVIEW_CHECK_NOTHROW(IPVIEW_PRE(true));
    IPVIEW_CHECK_NOTHROW(IPVIEW_PRE(1 + 1 == 2));
    IPVIEW_CHECK_NOTHROW(IPVIEW_PRE(42 > 0))
)

IPVIEW_TEST_CASE(contracts_post_throws_on_false,
    IPVIEW_CHECK_THROWS_AS(IPVIEW_POST(false), std::logic_error)
)

IPVIEW_TEST_CASE(contracts_post_does_not_throw_on_true,
    IPVIEW_CHECK_NOTHROW(IPVIEW_POST(true))
)

IPVIEW_TEST_CASE(contracts_assert_throws_on_false,
    IPVIEW_CHECK_THROWS_AS(IPVIEW_ASSERT(false), std::logic_error)
)

IPVIEW_TEST_CASE(contracts_assert_does_not_throw_on_true,
    IPVIEW_CHECK_NOTHROW(IPVIEW_ASSERT(true));
    IPVIEW_CHECK_NOTHROW(IPVIEW_ASSERT(0 == 0))
)

IPVIEW_TEST_CASE(contracts_throw_message_contains_expression_text,
    try {
        IPVIEW_PRE(1 == 2);
    } catch (std::logic_error const& e) {
        QString const what = QString::fromUtf8(e.what());
        IPVIEW_CHECK(what.contains(QString::fromUtf8("1 == 2")));
        IPVIEW_CHECK(what.contains(QString::fromUtf8("pre")));
    }
)
