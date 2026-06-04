// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.0 — Concepts.h
//  C++26: concepts, requires, type-traits
//  Type constraints used across the project (network targets, ports,
//  buffers, time points, hashable types, result types).
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>     // C++20/26: concepts
#include <string_view>
#include <type_traits>
#include <chrono>
#include <functional>
#include <expected>
#include <cstdint>

// ═══════════════════════════════════════════════════════════════════════════════
namespace IPView::Concepts {

// ── NetworkTarget ────────────────────────────────────────────────────────
//  Accepts std::string_view, const char*, QString (via conversion)
//  and any type convertible to std::string_view.
template <typename T>
concept NetworkTarget = std::convertible_to<T, std::string_view>
                     || requires (T t) {
                            { t.toStdString() } -> std::convertible_to<std::string_view>;
                        };

// ── NumericPort ──────────────────────────────────────────────────────────
//  Integer types small enough to hold a 0–65535 port number.
template <typename T>
concept NumericPort = std::integral<T> && (sizeof(T) <= 2);

// ── BufferData ───────────────────────────────────────────────────────────
//  Any container that exposes contiguous raw bytes (QByteArray, std::string,
//  std::vector<std::byte>, std::span<std::byte>).
template <typename T>
concept BufferData = requires (T t) {
    { t.data() } -> std::convertible_to<const void*>;
    { t.size() } -> std::convertible_to<std::size_t>;
};

// ── StringLike ───────────────────────────────────────────────────────────
//  Anything convertible to std::string_view or QString.
template <typename T>
concept StringLike = std::convertible_to<T, std::string_view>
                  || requires (T t) { t.toStdString(); };

// ── Hashable ─────────────────────────────────────────────────────────────
//  std::hash<T> must be specialized and usable.
template <typename T>
concept Hashable = requires (T t) { std::hash<T>{}(t); };

// ── TimePoint ────────────────────────────────────────────────────────────
//  Any of the standard clock time points the project passes around.
template <typename T>
concept TimePoint = std::is_same_v<T, std::chrono::system_clock::time_point>
                 || std::is_same_v<T, std::chrono::steady_clock::time_point>
                 || std::is_same_v<T, std::chrono::high_resolution_clock::time_point>;

// ── Duration ─────────────────────────────────────────────────────────────
//  Any std::chrono duration — used to gate functions that take
//  timeout parameters generically.
template <typename T>
concept Duration = requires {
    typename T::rep;
    typename T::period;
    requires std::chrono::__is_duration_v<T>;
};

// ── Result ───────────────────────────────────────────────────────────────
//  A type that follows the std::expected<T, E> interface, so the
//  ErrorBridge layer can talk to any of them uniformly.
template <typename T>
concept Result = requires (T t) {
    typename T::value_type;
    typename T::error_type;
    { t.has_value() } -> std::convertible_to<bool>;
    { t.value() };
    { t.error() };
};

// ── Callback ─────────────────────────────────────────────────────────────
//  Any std::move_only_function-compatible callable (added so the
//  AuditorModule batch driver can take either std::function or
//  std::move_only_function without overloading the API).
template <typename T>
concept Callback = std::is_invocable_v<T> || requires (T t) { std::invoke(std::move(t)); };

} // namespace IPView::Concepts

#endif // CONCEPTS_H
