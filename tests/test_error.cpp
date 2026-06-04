// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — tests/test_error.cpp
//
//  Validates the IPView::Error enum, ErrorInfo formatting, and
//  the IPVIEW_TRY / IPVIEW_UNWRAP macros (when present).
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "../Error.hpp"

#include <string>

IPVIEW_TEST_CASE(error_enum_is_default_zero,
    // The "no error" code must be zero so an uninitialised
    // ErrorInfo compares equal to the success state.
    static_assert(static_cast<std::uint8_t>(IPView::Error::None) == 0);
    IPVIEW_CHECK(true))

IPVIEW_TEST_CASE(error_info_format_includes_code_and_message,
    IPView::ErrorInfo info;
    info.code    = IPView::Error::InvalidHostname;
    info.message = "Empty hostname provided";
    std::string const text = info.format();
    // The format should mention the error kind name and the message.
    // The source-location field is optional; some compilers omit the
    // file name when ErrorInfo is default-constructed.
    IPVIEW_CHECK(text.find("InvalidHostname") != std::string::npos);
    IPVIEW_CHECK(text.find("Empty hostname provided") != std::string::npos))

IPVIEW_TEST_CASE(unexpected_helper_returns_unexpected,
    IPView::Result<int> r = IPView::unexpected(IPView::Error::FileReadError,
        std::string{"cannot open /etc/shadow"});
    IPVIEW_CHECK(!r.has_value());
    IPVIEW_CHECK_EQ(static_cast<int>(r.error().code),
                    static_cast<int>(IPView::Error::FileReadError));
    IPVIEW_CHECK_EQ(r.error().message, std::string{"cannot open /etc/shadow"}))

IPVIEW_TEST_CASE(result_with_value_has_value,
    IPView::Result<int> r{42};
    IPVIEW_CHECK(r.has_value());
    IPVIEW_CHECK_EQ(*r, 42))

IPVIEW_TEST_CASE(result_with_unexpected_does_not_have_value,
    IPView::Result<int> r = IPView::unexpected(IPView::Error::Unknown,
        std::string{"boom"});
    IPVIEW_CHECK(!r.has_value());
    IPVIEW_CHECK_EQ(static_cast<int>(r.error().code),
                    static_cast<int>(IPView::Error::Unknown)))
