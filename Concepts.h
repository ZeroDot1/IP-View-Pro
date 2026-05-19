// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.9.3 — Concepts.h
//  C++26: concepts, requires, type-traits
//  Typ-Constraints für Netzwerkoperationen (NetworkTarget, etc.)
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
//  Akzeptiert std::string_view, const char*, QString (über Umweg)
//  und alle Typen, die sich in string_view konvertieren lassen.
template <typename T>
concept NetworkTarget = std::convertible_to<T, std::string_view>
                     || requires (T t) {
                            { t.toStdString() } -> std::convertible_to<std::string_view>;
                        };

// ── NumericPort ─────────────────────────────────────────────────────────
//  Gültiger Port-Bereich (1..65535)
template <typename T>
concept NumericPort = std::integral<T> && (sizeof(T) <= 2);

// ── BufferData ──────────────────────────────────────────────────────────
//  Akzeptiert Typen, die kontinuierlichen Byte-Zugriff erlauben
template <typename T>
concept BufferData = requires (T t) {
    { t.data() } -> std::convertible_to<const void*>;
    { t.size() } -> std::convertible_to<std::size_t>;
};

} // namespace IPView::Concepts

#endif // CONCEPTS_H
