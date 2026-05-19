// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.3 — Concepts.h
//  C++26: concepts, requires, type-traits
//  Type constraints for network operations (NetworkTarget, etc.)
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>     // C++20/26: concepts
#include <string_view>
#include <type_traits>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Concepts {

// ── NetworkTarget (Item 28) ──────────────────────────────────────────────
//  Accepts std::string_view, const char*, QString (via conversion)
//  and any type convertible to std::string_view.
template <typename T>
concept NetworkTarget = std::convertible_to<T, std::string_view>
                     || requires (T t) {
                            { t.toStdString() } -> std::convertible_to<std::string_view>;
                        };

// ── NumericPort ─────────────────────────────────────────────────────────
//  Valid port range (1..65535)
template <typename T>
concept NumericPort = std::integral<T> && (sizeof(T) <= 2);

// ── BufferData ──────────────────────────────────────────────────────────
//  Accepts types that provide contiguous byte access
template <typename T>
concept BufferData = requires (T t) {
    { t.data() } -> std::convertible_to<const void*>;
    { t.size() } -> std::convertible_to<std::size_t>;
};

} // namespace IPView::Concepts

#endif // CONCEPTS_H
