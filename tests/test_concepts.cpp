// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — tests/test_concepts.cpp
//
//  Validates that the Concepts.h type constraints accept the expected
//  types and reject the rest at compile time.
// ═══════════════════════════════════════════════════════════════════════════════

#include "TestFramework.hpp"
#include "../Concepts.h"

#include <string>
#include <string_view>
#include <chrono>
#include <vector>
#include <map>
#include <expected>
#include <type_traits>

IPVIEW_TEST_CASE(concepts_namespace_exists,
    // The whole point of this file is to compile-check the
    // concepts. We can't easily test them at runtime, but we
    // can confirm the static-assertions below hold.
    static_assert(IPView::Concepts::NumericPort<short>);
    static_assert(IPView::Concepts::NumericPort<unsigned short>);
    static_assert(!IPView::Concepts::NumericPort<int>);
    static_assert(!IPView::Concepts::NumericPort<std::size_t>);

    static_assert(IPView::Concepts::StringLike<std::string_view>);
    static_assert(IPView::Concepts::StringLike<const char*>);
    static_assert(IPView::Concepts::Hashable<int>);
    static_assert(IPView::Concepts::Hashable<std::string>);

    static_assert(IPView::Concepts::TimePoint<std::chrono::system_clock::time_point>);
    static_assert(IPView::Concepts::TimePoint<std::chrono::steady_clock::time_point>);
    static_assert(!IPView::Concepts::TimePoint<int>);

    static_assert(IPView::Concepts::Duration<std::chrono::seconds>);
    static_assert(IPView::Concepts::Duration<std::chrono::milliseconds>);
    static_assert(!IPView::Concepts::Duration<int>);

    using Expected = std::expected<int, std::string>;
    static_assert(IPView::Concepts::Result<Expected>);

    static_assert(IPView::Concepts::BufferData<std::string>);
    static_assert(IPView::Concepts::BufferData<std::vector<char>>);
    IPVIEW_CHECK(true))

IPVIEW_TEST_CASE(concepts_buffer_data_with_byte_vector,
    static_assert(IPView::Concepts::BufferData<std::vector<std::byte>>);
    IPVIEW_CHECK(true))
