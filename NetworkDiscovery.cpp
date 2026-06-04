// ═══════════════════════════════════════════════════════════════════════════════
//  IPView Pro v2.15.4 — NetworkDiscovery.cpp
//  C++26: QStringLiteral, std::optional, structured bindings
//  Public Domain — No License — No Restrictions.
// ═══════════════════════════════════════════════════════════════════════════════

#include "NetworkDiscovery.h"
#include "Logger.h"

#include <QFile>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QTextStream>
#include <QTimer>

#include <algorithm>
#include <array>
#include <ranges>
#include <string_view>

namespace IPView::Scanner {

// ═══════════════════════════════════════════════════════════════════════════════
//  Tiny OUI prefix table (first 24 bits of the MAC).
//
//  Curated for home / office LAN manufacturers we expect to find
//  in real life. A hit is a UX bonus; a miss is silent (the device
//  row just shows the raw MAC).
// ═══════════════════════════════════════════════════════════════════════════════
struct OuiEntry {
    std::array<std::uint8_t, 3> prefix;
    std::string_view           vendor;
};
constexpr std::array OUI_TABLE = {
    // Apple — common prefixes (full list is much longer; this
    // is a representative sample covering the last 10 years).
    OuiEntry{{0x00, 0x1B, 0x63}, "Apple"},
    OuiEntry{{0x00, 0x1F, 0xF3}, "Apple"},
    OuiEntry{{0x00, 0x25, 0x00}, "Apple"},
    OuiEntry{{0x00, 0x25, 0x4B}, "Apple"},
    OuiEntry{{0x00, 0x26, 0x08}, "Apple"},
    OuiEntry{{0x00, 0x26, 0xB0}, "Apple"},
    OuiEntry{{0x00, 0x26, 0xBB}, "Apple"},
    OuiEntry{{0x3C, 0x15, 0xC2}, "Apple"},
    OuiEntry{{0x40, 0x6C, 0x8F}, "Apple"},
    OuiEntry{{0x40, 0xA6, 0xD9}, "Apple"},
    OuiEntry{{0x44, 0x00, 0x10}, "Apple"},
    OuiEntry{{0x58, 0xB0, 0x35}, "Apple"},
    OuiEntry{{0x5C, 0x95, 0xAE}, "Apple"},
    OuiEntry{{0x60, 0x33, 0x4B}, "Apple"},
    OuiEntry{{0x60, 0xC5, 0x47}, "Apple"},
    OuiEntry{{0x64, 0xB9, 0xE8}, "Apple"},
    OuiEntry{{0x68, 0x5B, 0x35}, "Apple"},
    OuiEntry{{0x68, 0xA8, 0x6D}, "Apple"},
    OuiEntry{{0x6C, 0x40, 0x08}, "Apple"},
    OuiEntry{{0x70, 0x11, 0x24}, "Apple"},
    OuiEntry{{0x70, 0x73, 0xCB}, "Apple"},
    OuiEntry{{0x78, 0x31, 0xC1}, "Apple"},
    OuiEntry{{0x78, 0x7E, 0x61}, "Apple"},
    OuiEntry{{0x7C, 0x11, 0xBE}, "Apple"},
    OuiEntry{{0x7C, 0x6D, 0x62}, "Apple"},
    OuiEntry{{0x7C, 0xC5, 0x37}, "Apple"},
    OuiEntry{{0x80, 0xB0, 0x3D}, "Apple"},
    OuiEntry{{0x80, 0xE6, 0x50}, "Apple"},
    OuiEntry{{0x84, 0x38, 0x35}, "Apple"},
    OuiEntry{{0x88, 0x1F, 0xA1}, "Apple"},
    OuiEntry{{0x88, 0x53, 0x95}, "Apple"},
    OuiEntry{{0x88, 0xC6, 0x63}, "Apple"},
    OuiEntry{{0x8C, 0x29, 0x37}, "Apple"},
    OuiEntry{{0x8C, 0x7C, 0x92}, "Apple"},
    OuiEntry{{0x90, 0x27, 0xE4}, "Apple"},
    OuiEntry{{0x98, 0x01, 0xA7}, "Apple"},
    OuiEntry{{0x98, 0xB8, 0xE3}, "Apple"},
    OuiEntry{{0x98, 0xD6, 0xBB}, "Apple"},
    OuiEntry{{0x98, 0xE0, 0xD9}, "Apple"},
    OuiEntry{{0x9C, 0x04, 0xEB}, "Apple"},
    OuiEntry{{0x9C, 0x20, 0x7B}, "Apple"},
    OuiEntry{{0x9C, 0xF3, 0x87}, "Apple"},
    OuiEntry{{0xA4, 0x5E, 0x60}, "Apple"},
    OuiEntry{{0xA4, 0xB1, 0x97}, "Apple"},
    OuiEntry{{0xA4, 0xC3, 0x61}, "Apple"},
    OuiEntry{{0xA8, 0x20, 0x66}, "Apple"},
    OuiEntry{{0xA8, 0x5C, 0x2C}, "Apple"},
    OuiEntry{{0xA8, 0x88, 0x08}, "Apple"},
    OuiEntry{{0xA8, 0x96, 0x8A}, "Apple"},
    OuiEntry{{0xAC, 0x3C, 0x0B}, "Apple"},
    OuiEntry{{0xAC, 0x87, 0xA3}, "Apple"},
    OuiEntry{{0xAC, 0xCF, 0x5C}, "Apple"},
    OuiEntry{{0xB0, 0x17, 0xB8}, "Apple"},
    OuiEntry{{0xB0, 0x19, 0xC6}, "Apple"},
    OuiEntry{{0xB0, 0x34, 0x95}, "Apple"},
    OuiEntry{{0xB0, 0x65, 0xBD}, "Apple"},
    OuiEntry{{0xB0, 0x9F, 0xBA}, "Apple"},
    OuiEntry{{0xB4, 0x18, 0xD1}, "Apple"},
    OuiEntry{{0xB4, 0x4B, 0xD2}, "Apple"},
    OuiEntry{{0xB4, 0xF0, 0xAB}, "Apple"},
    OuiEntry{{0xB8, 0x09, 0x8A}, "Apple"},
    OuiEntry{{0xB8, 0x17, 0xC2}, "Apple"},
    OuiEntry{{0xB8, 0x53, 0xAC}, "Apple"},
    OuiEntry{{0xB8, 0x78, 0x2E}, "Apple"},
    OuiEntry{{0xB8, 0x9A, 0x2A}, "Apple"},
    OuiEntry{{0xB8, 0xE8, 0x56}, "Apple"},
    OuiEntry{{0xB8, 0xF6, 0xB1}, "Apple"},
    OuiEntry{{0xBC, 0x52, 0xB7}, "Apple"},
    OuiEntry{{0xBC, 0x67, 0x78}, "Apple"},
    OuiEntry{{0xBC, 0xA9, 0x20}, "Apple"},
    OuiEntry{{0xC0, 0x63, 0x94}, "Apple"},
    OuiEntry{{0xC0, 0x84, 0x7A}, "Apple"},
    OuiEntry{{0xC0, 0xCE, 0xCD}, "Apple"},
    OuiEntry{{0xC4, 0xB3, 0x01}, "Apple"},
    OuiEntry{{0xC8, 0x1E, 0xE7}, "Apple"},
    OuiEntry{{0xC8, 0x33, 0x4B}, "Apple"},
    OuiEntry{{0xC8, 0x69, 0xCD}, "Apple"},
    OuiEntry{{0xC8, 0x6F, 0x1D}, "Apple"},
    OuiEntry{{0xC8, 0xBC, 0xC8}, "Apple"},
    OuiEntry{{0xCC, 0x08, 0xE0}, "Apple"},
    OuiEntry{{0xCC, 0x25, 0xEF}, "Apple"},
    OuiEntry{{0xCC, 0x78, 0x5F}, "Apple"},
    OuiEntry{{0xD0, 0x23, 0xDB}, "Apple"},
    OuiEntry{{0xD0, 0x25, 0x98}, "Apple"},
    OuiEntry{{0xD0, 0x4F, 0x7E}, "Apple"},
    OuiEntry{{0xD0, 0x81, 0x7A}, "Apple"},
    OuiEntry{{0xD0, 0xE1, 0x40}, "Apple"},
    OuiEntry{{0xD4, 0x9A, 0x20}, "Apple"},
    OuiEntry{{0xD4, 0xF4, 0x6F}, "Apple"},
    OuiEntry{{0xD8, 0x00, 0x4D}, "Apple"},
    OuiEntry{{0xD8, 0x1D, 0x72}, "Apple"},
    OuiEntry{{0xD8, 0x30, 0x62}, "Apple"},
    OuiEntry{{0xD8, 0x96, 0x95}, "Apple"},
    OuiEntry{{0xD8, 0x9E, 0x3F}, "Apple"},
    OuiEntry{{0xD8, 0xA2, 0x5E}, "Apple"},
    OuiEntry{{0xD8, 0xCF, 0x9C}, "Apple"},
    OuiEntry{{0xDC, 0x2B, 0x61}, "Apple"},
    OuiEntry{{0xDC, 0x56, 0xE7}, "Apple"},
    OuiEntry{{0xDC, 0x9B, 0x9C}, "Apple"},
    OuiEntry{{0xDC, 0xA9, 0x04}, "Apple"},
    OuiEntry{{0xE0, 0x5F, 0x45}, "Apple"},
    OuiEntry{{0xE0, 0xB9, 0xBA}, "Apple"},
    OuiEntry{{0xE0, 0xC9, 0x7A}, "Apple"},
    OuiEntry{{0xE0, 0xF2, 0x11}, "Apple"},
    OuiEntry{{0xE0, 0xF8, 0x47}, "Apple"},
    OuiEntry{{0xE4, 0x8B, 0x7F}, "Apple"},
    OuiEntry{{0xE4, 0xC6, 0x3D}, "Apple"},
    OuiEntry{{0xE4, 0xCE, 0x8F}, "Apple"},
    OuiEntry{{0xE8, 0x04, 0x0B}, "Apple"},
    OuiEntry{{0xE8, 0x80, 0x2E}, "Apple"},
    OuiEntry{{0xE8, 0x8D, 0x28}, "Apple"},
    OuiEntry{{0xE8, 0xB2, 0xAC}, "Apple"},
    OuiEntry{{0xEC, 0x35, 0x86}, "Apple"},
    OuiEntry{{0xEC, 0x85, 0x2F}, "Apple"},
    OuiEntry{{0xF0, 0xB4, 0x79}, "Apple"},
    OuiEntry{{0xF0, 0xC1, 0xF1}, "Apple"},
    OuiEntry{{0xF0, 0xCB, 0xA1}, "Apple"},
    OuiEntry{{0xF0, 0xD1, 0xA9}, "Apple"},
    OuiEntry{{0xF0, 0xDB, 0xF8}, "Apple"},
    OuiEntry{{0xF0, 0xF6, 0x1C}, "Apple"},
    OuiEntry{{0xF4, 0x0F, 0x24}, "Apple"},
    OuiEntry{{0xF4, 0x1B, 0xA1}, "Apple"},
    OuiEntry{{0xF4, 0x5C, 0x89}, "Apple"},
    OuiEntry{{0xF4, 0xF1, 0x5A}, "Apple"},
    OuiEntry{{0xF4, 0xF9, 0x51}, "Apple"},
    OuiEntry{{0xF8, 0x1E, 0xDF}, "Apple"},
    OuiEntry{{0xF8, 0x27, 0x93}, "Apple"},
    OuiEntry{{0xFC, 0x25, 0x3F}, "Apple"},
    OuiEntry{{0xFC, 0xFC, 0x48}, "Apple"},
    // Samsung
    OuiEntry{{0x00, 0x00, 0xF0}, "Samsung"},
    OuiEntry{{0x00, 0x07, 0xAB}, "Samsung"},
    OuiEntry{{0x00, 0x0D, 0xAE}, "Samsung"},
    OuiEntry{{0x00, 0x12, 0x47}, "Samsung"},
    OuiEntry{{0x00, 0x12, 0xFB}, "Samsung"},
    OuiEntry{{0x00, 0x13, 0x77}, "Samsung"},
    OuiEntry{{0x00, 0x15, 0x99}, "Samsung"},
    OuiEntry{{0x00, 0x17, 0xC9}, "Samsung"},
    OuiEntry{{0x00, 0x17, 0xD5}, "Samsung"},
    OuiEntry{{0x00, 0x1A, 0x8A}, "Samsung"},
    OuiEntry{{0x00, 0x1B, 0x98}, "Samsung"},
    OuiEntry{{0x00, 0x1C, 0x43}, "Samsung"},
    OuiEntry{{0x00, 0x1D, 0x25}, "Samsung"},
    OuiEntry{{0x00, 0x1D, 0xF6}, "Samsung"},
    OuiEntry{{0x00, 0x1E, 0x7D}, "Samsung"},
    OuiEntry{{0x00, 0x21, 0x19}, "Samsung"},
    OuiEntry{{0x00, 0x21, 0x4C}, "Samsung"},
    OuiEntry{{0x00, 0x23, 0x39}, "Samsung"},
    OuiEntry{{0x00, 0x23, 0x3A}, "Samsung"},
    OuiEntry{{0x00, 0x23, 0x99}, "Samsung"},
    OuiEntry{{0x00, 0x23, 0xD6}, "Samsung"},
    OuiEntry{{0x00, 0x24, 0x54}, "Samsung"},
    OuiEntry{{0x00, 0x24, 0x90}, "Samsung"},
    OuiEntry{{0x00, 0x24, 0x91}, "Samsung"},
    OuiEntry{{0x00, 0x24, 0xE9}, "Samsung"},
    OuiEntry{{0x00, 0x25, 0x38}, "Samsung"},
    OuiEntry{{0x00, 0x25, 0x66}, "Samsung"},
    OuiEntry{{0x00, 0x25, 0x67}, "Samsung"},
    OuiEntry{{0x00, 0x26, 0x37}, "Samsung"},
    OuiEntry{{0x00, 0x26, 0x5D}, "Samsung"},
    OuiEntry{{0x08, 0x08, 0xC2}, "Samsung"},
    OuiEntry{{0x08, 0x37, 0x3D}, "Samsung"},
    OuiEntry{{0x08, 0x8C, 0x2C}, "Samsung"},
    OuiEntry{{0x08, 0xD4, 0x2B}, "Samsung"},
    OuiEntry{{0x08, 0xEC, 0xA9}, "Samsung"},
    OuiEntry{{0x08, 0xEE, 0x8B}, "Samsung"},
    OuiEntry{{0x0C, 0x14, 0x20}, "Samsung"},
    OuiEntry{{0x0C, 0x71, 0x5D}, "Samsung"},
    OuiEntry{{0x0C, 0x89, 0x10}, "Samsung"},
    OuiEntry{{0x0C, 0x8D, 0xDB}, "Samsung"},
    OuiEntry{{0x10, 0x1D, 0xC0}, "Samsung"},
    OuiEntry{{0x14, 0x1F, 0x78}, "Samsung"},
    OuiEntry{{0x18, 0x1E, 0xB0}, "Samsung"},
    OuiEntry{{0x18, 0x46, 0x17}, "Samsung"},
    OuiEntry{{0x18, 0x83, 0x31}, "Samsung"},
    OuiEntry{{0x18, 0xAF, 0x8F}, "Samsung"},
    OuiEntry{{0x1C, 0x5A, 0x3E}, "Samsung"},
    OuiEntry{{0x1C, 0x62, 0xB8}, "Samsung"},
    OuiEntry{{0x1C, 0xAF, 0xF7}, "Samsung"},
    OuiEntry{{0x20, 0x13, 0xE0}, "Samsung"},
    OuiEntry{{0x20, 0x64, 0x32}, "Samsung"},
    OuiEntry{{0x24, 0x4B, 0x81}, "Samsung"},
    OuiEntry{{0x24, 0xF5, 0xAA}, "Samsung"},
    OuiEntry{{0x28, 0x39, 0x5E}, "Samsung"},
    OuiEntry{{0x28, 0xCC, 0x01}, "Samsung"},
    OuiEntry{{0x28, 0xBA, 0xB5}, "Samsung"},
    OuiEntry{{0x2C, 0x0E, 0x3D}, "Samsung"},
    OuiEntry{{0x2C, 0xAE, 0x2B}, "Samsung"},
    OuiEntry{{0x30, 0x07, 0x4D}, "Samsung"},
    OuiEntry{{0x30, 0xC7, 0xAE}, "Samsung"},
    OuiEntry{{0x30, 0xCB, 0xF8}, "Samsung"},
    OuiEntry{{0x30, 0xD6, 0xC9}, "Samsung"},
    OuiEntry{{0x30, 0xE2, 0x83}, "Samsung"},
    OuiEntry{{0x34, 0x23, 0x87}, "Samsung"},
    OuiEntry{{0x34, 0x31, 0x11}, "Samsung"},
    OuiEntry{{0x34, 0xAF, 0x2C}, "Samsung"},
    OuiEntry{{0x34, 0xC3, 0xAC}, "Samsung"},
    OuiEntry{{0x38, 0x0A, 0x94}, "Samsung"},
    OuiEntry{{0x38, 0x0B, 0x40}, "Samsung"},
    OuiEntry{{0x38, 0xAA, 0x3C}, "Samsung"},
    OuiEntry{{0x38, 0xD4, 0x0B}, "Samsung"},
    OuiEntry{{0x3C, 0x5A, 0x37}, "Samsung"},
    OuiEntry{{0x3C, 0x8B, 0xFE}, "Samsung"},
    OuiEntry{{0x3C, 0xA1, 0x0D}, "Samsung"},
    OuiEntry{{0x40, 0x0E, 0x85}, "Samsung"},
    OuiEntry{{0x44, 0x8B, 0x32}, "Samsung"},
    OuiEntry{{0x48, 0x5A, 0x3F}, "Samsung"},
    OuiEntry{{0x48, 0x5A, 0xB6}, "Samsung"},
    OuiEntry{{0x4C, 0x3C, 0x16}, "Samsung"},
    OuiEntry{{0x4C, 0xBC, 0xA5}, "Samsung"},
    OuiEntry{{0x50, 0x01, 0xBB}, "Samsung"},
    OuiEntry{{0x50, 0x32, 0x75}, "Samsung"},
    OuiEntry{{0x50, 0xCC, 0xF8}, "Samsung"},
    OuiEntry{{0x50, 0xF0, 0xD3}, "Samsung"},
    OuiEntry{{0x54, 0x88, 0x0E}, "Samsung"},
    OuiEntry{{0x5C, 0x0A, 0x5B}, "Samsung"},
    OuiEntry{{0x5C, 0x51, 0x88}, "Samsung"},
    OuiEntry{{0x5C, 0xE8, 0xEB}, "Samsung"},
    OuiEntry{{0x5C, 0xE9, 0x31}, "Samsung"},
    OuiEntry{{0x5C, 0xF6, 0xDC}, "Samsung"},
    OuiEntry{{0x60, 0x6B, 0xBD}, "Samsung"},
    OuiEntry{{0x64, 0x1C, 0xB0}, "Samsung"},
    OuiEntry{{0x64, 0xB3, 0x10}, "Samsung"},
    OuiEntry{{0x68, 0xEB, 0xC5}, "Samsung"},
    OuiEntry{{0x6C, 0xB7, 0xF4}, "Samsung"},
    OuiEntry{{0x6C, 0xF3, 0x73}, "Samsung"},
    OuiEntry{{0x70, 0xF9, 0x27}, "Samsung"},
    OuiEntry{{0x78, 0x25, 0xAD}, "Samsung"},
    OuiEntry{{0x78, 0x2B, 0xCB}, "Samsung"},
    OuiEntry{{0x78, 0x52, 0x1A}, "Samsung"},
    OuiEntry{{0x78, 0x9E, 0xD0}, "Samsung"},
    OuiEntry{{0x78, 0xAB, 0xBB}, "Samsung"},
    OuiEntry{{0x78, 0xBD, 0xBC}, "Samsung"},
    OuiEntry{{0x78, 0xD2, 0x94}, "Samsung"},
    OuiEntry{{0x78, 0xF7, 0xBE}, "Samsung"},
    OuiEntry{{0x7C, 0x61, 0x66}, "Samsung"},
    OuiEntry{{0x80, 0x18, 0xA7}, "Samsung"},
    OuiEntry{{0x84, 0x11, 0x9E}, "Samsung"},
    OuiEntry{{0x84, 0x25, 0xDB}, "Samsung"},
    OuiEntry{{0x84, 0x38, 0x38}, "Samsung"},
    OuiEntry{{0x84, 0x55, 0xA5}, "Samsung"},
    OuiEntry{{0x88, 0x32, 0x9B}, "Samsung"},
    OuiEntry{{0x88, 0x9F, 0x6F}, "Samsung"},
    OuiEntry{{0x8C, 0x71, 0xF8}, "Samsung"},
    OuiEntry{{0x90, 0x18, 0x7C}, "Samsung"},
    OuiEntry{{0x94, 0x35, 0x0A}, "Samsung"},
    OuiEntry{{0x94, 0x51, 0x03}, "Samsung"},
    OuiEntry{{0x94, 0x76, 0xB7}, "Samsung"},
    OuiEntry{{0x94, 0xB1, 0x0A}, "Samsung"},
    OuiEntry{{0x98, 0x0C, 0xA5}, "Samsung"},
    OuiEntry{{0x9C, 0x65, 0xF9}, "Samsung"},
    OuiEntry{{0x9C, 0xE6, 0xE7}, "Samsung"},
    OuiEntry{{0xA0, 0x21, 0x95}, "Samsung"},
    OuiEntry{{0xA0, 0x75, 0x91}, "Samsung"},
    OuiEntry{{0xA0, 0xB1, 0x0A}, "Samsung"},
    OuiEntry{{0xA4, 0xEB, 0xD3}, "Samsung"},
    OuiEntry{{0xAC, 0x36, 0x13}, "Samsung"},
    OuiEntry{{0xAC, 0x5F, 0x3E}, "Samsung"},
    OuiEntry{{0xB0, 0xC4, 0xE7}, "Samsung"},
    OuiEntry{{0xB4, 0x62, 0x93}, "Samsung"},
    OuiEntry{{0xB8, 0xBB, 0xAF}, "Samsung"},
    OuiEntry{{0xB8, 0xC6, 0x8E}, "Samsung"},
    OuiEntry{{0xBC, 0x14, 0x85}, "Samsung"},
    OuiEntry{{0xBC, 0x20, 0xA4}, "Samsung"},
    OuiEntry{{0xBC, 0x79, 0xAD}, "Samsung"},
    OuiEntry{{0xC0, 0xBD, 0xD1}, "Samsung"},
    OuiEntry{{0xC4, 0x62, 0xEA}, "Samsung"},
    OuiEntry{{0xC4, 0x73, 0x1E}, "Samsung"},
    OuiEntry{{0xC8, 0x14, 0x79}, "Samsung"},
    OuiEntry{{0xC8, 0x7E, 0x40}, "Samsung"},
    OuiEntry{{0xCC, 0x07, 0xAB}, "Samsung"},
    OuiEntry{{0xCC, 0xF9, 0xE8}, "Samsung"},
    OuiEntry{{0xD0, 0x22, 0xBE}, "Samsung"},
    OuiEntry{{0xD0, 0x87, 0xE2}, "Samsung"},
    OuiEntry{{0xD4, 0x87, 0xD8}, "Samsung"},
    OuiEntry{{0xD4, 0xAE, 0x52}, "Samsung"},
    OuiEntry{{0xD8, 0xC4, 0xE9}, "Samsung"},
    OuiEntry{{0xD8, 0xE0, 0xE1}, "Samsung"},
    OuiEntry{{0xDC, 0x71, 0x96}, "Samsung"},
    OuiEntry{{0xE0, 0x99, 0x71}, "Samsung"},
    OuiEntry{{0xE4, 0x32, 0xCB}, "Samsung"},
    OuiEntry{{0xE4, 0xE0, 0xC5}, "Samsung"},
    OuiEntry{{0xE8, 0x50, 0x8B}, "Samsung"},
    OuiEntry{{0xEC, 0x1F, 0x72}, "Samsung"},
    OuiEntry{{0xEC, 0x9B, 0xF3}, "Samsung"},
    OuiEntry{{0xF0, 0x25, 0xB7}, "Samsung"},
    OuiEntry{{0xF0, 0xE7, 0x7E}, "Samsung"},
    OuiEntry{{0xF4, 0x09, 0xD8}, "Samsung"},
    OuiEntry{{0xF4, 0x7B, 0x5E}, "Samsung"},
    OuiEntry{{0xF4, 0xD9, 0xFB}, "Samsung"},
    OuiEntry{{0xF8, 0x04, 0x2E}, "Samsung"},
    OuiEntry{{0xFC, 0x00, 0x12}, "Samsung"},
    OuiEntry{{0xFC, 0xA1, 0x3E}, "Samsung"},
    OuiEntry{{0xFC, 0xC2, 0xDE}, "Samsung"},
    OuiEntry{{0x34, 0x23, 0xBA}, "Samsung"},
    OuiEntry{{0xC0, 0xCC, 0xF8}, "Apple"},
    OuiEntry{{0xF0, 0xF2, 0x49}, "Samsung"},
    OuiEntry{{0xB0, 0xC5, 0x54}, "Samsung"},
    // Raspberry Pi Foundation
    OuiEntry{{0xB8, 0x27, 0xEB}, "Raspberry Pi"},
    OuiEntry{{0xDC, 0xA6, 0x32}, "Raspberry Pi"},
    OuiEntry{{0xE4, 0x5F, 0x01}, "Raspberry Pi"},
    OuiEntry{{0xD8, 0x3A, 0xDD}, "Raspberry Pi"},
    OuiEntry{{0x2C, 0xCF, 0x67}, "Raspberry Pi"},
    // Google
    OuiEntry{{0x3C, 0x5A, 0xB4}, "Google"},
    OuiEntry{{0xF4, 0xF5, 0xD8}, "Google"},
    OuiEntry{{0xF8, 0x8F, 0xCA}, "Google"},
    OuiEntry{{0xA4, 0x77, 0x33}, "Google"},
    OuiEntry{{0x70, 0x3A, 0xCB}, "Google"},
    OuiEntry{{0xAC, 0x63, 0xBE}, "Google"},
    OuiEntry{{0x20, 0xDF, 0xB9}, "Google"},
    OuiEntry{{0xF0, 0xEF, 0x86}, "Google"},
    OuiEntry{{0xFC, 0xC2, 0xDE}, "Google"},
    // Microsoft (Xbox + Surface + Windows Phone)
    OuiEntry{{0x00, 0x50, 0xF2}, "Microsoft"},
    OuiEntry{{0x00, 0x15, 0x5D}, "Microsoft"},
    OuiEntry{{0x00, 0x17, 0xF2}, "Microsoft"},
    OuiEntry{{0x00, 0x1D, 0xD8}, "Microsoft"},
    OuiEntry{{0x00, 0x22, 0x48}, "Microsoft"},
    OuiEntry{{0x00, 0x25, 0xAE}, "Microsoft"},
    OuiEntry{{0x7C, 0x1E, 0x52}, "Microsoft"},
    OuiEntry{{0x7C, 0xED, 0x8D}, "Microsoft"},
    OuiEntry{{0x98, 0x5F, 0xD3}, "Microsoft"},
    // VMware
    OuiEntry{{0x00, 0x05, 0x69}, "VMware"},
    OuiEntry{{0x00, 0x0C, 0x29}, "VMware"},
    OuiEntry{{0x00, 0x1C, 0x14}, "VMware"},
    OuiEntry{{0x00, 0x50, 0x56}, "VMware"},
    // Dell
    OuiEntry{{0x00, 0x0F, 0x1F}, "Dell"},
    OuiEntry{{0x00, 0x13, 0x72}, "Dell"},
    OuiEntry{{0x00, 0x14, 0x22}, "Dell"},
    OuiEntry{{0x00, 0x18, 0x8B}, "Dell"},
    OuiEntry{{0x00, 0x19, 0xB9}, "Dell"},
    OuiEntry{{0x00, 0x1C, 0x23}, "Dell"},
    OuiEntry{{0x00, 0x1D, 0x09}, "Dell"},
    OuiEntry{{0x00, 0x1E, 0x4F}, "Dell"},
    OuiEntry{{0x00, 0x1E, 0xC9}, "Dell"},
    OuiEntry{{0x00, 0x21, 0x9B}, "Dell"},
    OuiEntry{{0x00, 0x22, 0x19}, "Dell"},
    OuiEntry{{0x00, 0x23, 0xAE}, "Dell"},
    OuiEntry{{0x00, 0x24, 0xE8}, "Dell"},
    OuiEntry{{0x00, 0x25, 0x64}, "Dell"},
    OuiEntry{{0x00, 0x26, 0xB9}, "Dell"},
    OuiEntry{{0x18, 0x03, 0x73}, "Dell"},
    OuiEntry{{0x18, 0xA9, 0x9B}, "Dell"},
    OuiEntry{{0x18, 0xDB, 0xF2}, "Dell"},
    OuiEntry{{0x20, 0x04, 0x0F}, "Dell"},
    OuiEntry{{0x24, 0xB6, 0xFD}, "Dell"},
    OuiEntry{{0x28, 0xF1, 0x0E}, "Dell"},
    OuiEntry{{0x34, 0x17, 0xEB}, "Dell"},
    OuiEntry{{0x34, 0xE6, 0xD7}, "Dell"},
    OuiEntry{{0x44, 0xA8, 0x42}, "Dell"},
    OuiEntry{{0x50, 0x9A, 0x4C}, "Dell"},
    OuiEntry{{0x54, 0x9F, 0x35}, "Dell"},
    OuiEntry{{0x78, 0x2B, 0xCB}, "Dell"},
    OuiEntry{{0x80, 0x18, 0x44}, "Dell"},
    OuiEntry{{0x84, 0x7B, 0xEB}, "Dell"},
    OuiEntry{{0xA4, 0x1F, 0x72}, "Dell"},
    OuiEntry{{0xB0, 0x83, 0xFE}, "Dell"},
    OuiEntry{{0xB4, 0xE1, 0x0F}, "Dell"},
    OuiEntry{{0xD0, 0x43, 0x1E}, "Dell"},
    OuiEntry{{0xD0, 0x67, 0xE5}, "Dell"},
    OuiEntry{{0xD4, 0xAE, 0x52}, "Dell"},
    OuiEntry{{0xD4, 0xBE, 0xD9}, "Dell"},
    OuiEntry{{0xE0, 0xDB, 0x55}, "Dell"},
    OuiEntry{{0xEC, 0xF4, 0xBB}, "Dell"},
    OuiEntry{{0xF0, 0x1F, 0xAF}, "Dell"},
    OuiEntry{{0xF4, 0x8E, 0x38}, "Dell"},
    OuiEntry{{0xF8, 0xBC, 0x12}, "Dell"},
    OuiEntry{{0xF8, 0xCA, 0xB8}, "Dell"},
    OuiEntry{{0xF8, 0xDB, 0x88}, "Dell"},
    // HP
    OuiEntry{{0x00, 0x08, 0x02}, "HP"},
    OuiEntry{{0x00, 0x08, 0x83}, "HP"},
    OuiEntry{{0x00, 0x0B, 0xCD}, "HP"},
    OuiEntry{{0x00, 0x0E, 0x7F}, "HP"},
    OuiEntry{{0x00, 0x0F, 0x20}, "HP"},
    OuiEntry{{0x00, 0x10, 0x83}, "HP"},
    OuiEntry{{0x00, 0x11, 0x0A}, "HP"},
    OuiEntry{{0x00, 0x11, 0x85}, "HP"},
    OuiEntry{{0x00, 0x12, 0x79}, "HP"},
    OuiEntry{{0x00, 0x13, 0x21}, "HP"},
    OuiEntry{{0x00, 0x14, 0x38}, "HP"},
    OuiEntry{{0x00, 0x14, 0xC2}, "HP"},
    OuiEntry{{0x00, 0x15, 0x60}, "HP"},
    OuiEntry{{0x00, 0x16, 0x35}, "HP"},
    OuiEntry{{0x00, 0x17, 0xA4}, "HP"},
    OuiEntry{{0x00, 0x18, 0x71}, "HP"},
    OuiEntry{{0x00, 0x19, 0xBB}, "HP"},
    OuiEntry{{0x00, 0x1A, 0x4B}, "HP"},
    OuiEntry{{0x00, 0x1B, 0x78}, "HP"},
    OuiEntry{{0x00, 0x1C, 0xC4}, "HP"},
    OuiEntry{{0x00, 0x1E, 0x0B}, "HP"},
    OuiEntry{{0x00, 0x1F, 0x29}, "HP"},
    OuiEntry{{0x00, 0x21, 0x5A}, "HP"},
    OuiEntry{{0x00, 0x22, 0x64}, "HP"},
    OuiEntry{{0x00, 0x23, 0x7D}, "HP"},
    OuiEntry{{0x00, 0x24, 0x81}, "HP"},
    OuiEntry{{0x00, 0x25, 0xB3}, "HP"},
    OuiEntry{{0x00, 0x26, 0x55}, "HP"},
    OuiEntry{{0x00, 0x26, 0xF1}, "HP"},
    OuiEntry{{0x00, 0x30, 0x6E}, "HP"},
    OuiEntry{{0x00, 0x30, 0xA1}, "HP"},
    OuiEntry{{0x28, 0x80, 0x23}, "HP"},
    OuiEntry{{0x28, 0x92, 0x4A}, "HP"},
    OuiEntry{{0x2C, 0x23, 0x3A}, "HP"},
    OuiEntry{{0x2C, 0x41, 0x38}, "HP"},
    OuiEntry{{0x2C, 0x44, 0xFD}, "HP"},
    OuiEntry{{0x2C, 0x59, 0xE5}, "HP"},
    OuiEntry{{0x2C, 0x76, 0x8A}, "HP"},
    OuiEntry{{0x30, 0x8D, 0x99}, "HP"},
    OuiEntry{{0x30, 0xE1, 0x71}, "HP"},
    OuiEntry{{0x34, 0x64, 0xA9}, "HP"},
    OuiEntry{{0x38, 0x63, 0xBB}, "HP"},
    OuiEntry{{0x3C, 0x4A, 0x92}, "HP"},
    OuiEntry{{0x3C, 0x52, 0x82}, "HP"},
    OuiEntry{{0x3C, 0xA8, 0x2A}, "HP"},
    OuiEntry{{0x3C, 0xD9, 0x2B}, "HP"},
    OuiEntry{{0x40, 0xA8, 0xF0}, "HP"},
    OuiEntry{{0x44, 0x1E, 0xA1}, "HP"},
    OuiEntry{{0x44, 0x31, 0x92}, "HP"},
    OuiEntry{{0x44, 0x48, 0xC1}, "HP"},
    OuiEntry{{0x4C, 0x39, 0x09}, "HP"},
    OuiEntry{{0x50, 0x65, 0xF3}, "HP"},
    OuiEntry{{0x5C, 0x8A, 0x38}, "HP"},
    OuiEntry{{0x5C, 0xB9, 0x01}, "HP"},
    OuiEntry{{0x60, 0xEB, 0x69}, "HP"},
    OuiEntry{{0x64, 0x31, 0x50}, "HP"},
    OuiEntry{{0x64, 0x51, 0x06}, "HP"},
    OuiEntry{{0x68, 0xB5, 0x99}, "HP"},
    OuiEntry{{0x6C, 0x3B, 0xE5}, "HP"},
    OuiEntry{{0x6C, 0xC2, 0x17}, "HP"},
    OuiEntry{{0x70, 0x5A, 0x0F}, "HP"},
    OuiEntry{{0x78, 0x48, 0x59}, "HP"},
    OuiEntry{{0x78, 0xAC, 0xC0}, "HP"},
    OuiEntry{{0x78, 0xE3, 0xB5}, "HP"},
    OuiEntry{{0x78, 0xE7, 0xD1}, "HP"},
    OuiEntry{{0x7C, 0x5C, 0xF8}, "HP"},
    OuiEntry{{0x80, 0xCE, 0x62}, "HP"},
    OuiEntry{{0x80, 0xE8, 0x2C}, "HP"},
    OuiEntry{{0x80, 0xF0, 0x1D}, "HP"},
    OuiEntry{{0x84, 0x34, 0x97}, "HP"},
    OuiEntry{{0x84, 0x79, 0x73}, "HP"},
    OuiEntry{{0x84, 0x8F, 0x69}, "HP"},
    OuiEntry{{0x8C, 0xDC, 0xD4}, "HP"},
    OuiEntry{{0x90, 0x21, 0x55}, "HP"},
    OuiEntry{{0x94, 0x18, 0x82}, "HP"},
    OuiEntry{{0x98, 0x4B, 0xE1}, "HP"},
    OuiEntry{{0x9C, 0x8E, 0x99}, "HP"},
    OuiEntry{{0x9C, 0xB6, 0xD0}, "HP"},
    OuiEntry{{0x9C, 0xDC, 0x71}, "HP"},
    OuiEntry{{0xA0, 0x1D, 0x48}, "HP"},
    OuiEntry{{0xA0, 0x48, 0x1C}, "HP"},
    OuiEntry{{0xA0, 0x8C, 0xFD}, "HP"},
    OuiEntry{{0xA0, 0xB3, 0xCC}, "HP"},
    OuiEntry{{0xA0, 0xD3, 0xC1}, "HP"},
    OuiEntry{{0xA4, 0x5D, 0x36}, "HP"},
    OuiEntry{{0xA8, 0x3B, 0x76}, "HP"},
    OuiEntry{{0xAC, 0x16, 0x2D}, "HP"},
    OuiEntry{{0xB0, 0x5A, 0xDA}, "HP"},
    OuiEntry{{0xB4, 0xB5, 0x2F}, "HP"},
    OuiEntry{{0xB4, 0xB5, 0xFE}, "HP"},
    OuiEntry{{0xB8, 0xAF, 0xF7}, "HP"},
    OuiEntry{{0xBC, 0xEA, 0xFA}, "HP"},
    OuiEntry{{0xC0, 0x91, 0x34}, "HP"},
    OuiEntry{{0xC4, 0x34, 0x6B}, "HP"},
    OuiEntry{{0xC4, 0x65, 0x16}, "HP"},
    OuiEntry{{0xC8, 0xB5, 0xAD}, "HP"},
    OuiEntry{{0xC8, 0xCB, 0xB8}, "HP"},
    OuiEntry{{0xC8, 0xD3, 0xFF}, "HP"},
    OuiEntry{{0xCC, 0x3E, 0x5F}, "HP"},
    OuiEntry{{0xD0, 0x7E, 0x28}, "HP"},
    OuiEntry{{0xD4, 0xC9, 0xEF}, "HP"},
    OuiEntry{{0xD8, 0x9D, 0x67}, "HP"},
    OuiEntry{{0xD8, 0x9E, 0x3F}, "HP"},
    OuiEntry{{0xDC, 0x4A, 0x3E}, "HP"},
    OuiEntry{{0xE0, 0xF8, 0x47}, "HP"},
    OuiEntry{{0xE4, 0x11, 0x5B}, "HP"},
    OuiEntry{{0xE8, 0x39, 0x35}, "HP"},
    OuiEntry{{0xE8, 0xF7, 0x24}, "HP"},
    OuiEntry{{0xEC, 0x9A, 0x74}, "HP"},
    OuiEntry{{0xEC, 0xB1, 0xD7}, "HP"},
    OuiEntry{{0xF0, 0x92, 0x1C}, "HP"},
    OuiEntry{{0xF4, 0xCE, 0x46}, "HP"},
    OuiEntry{{0xF8, 0x0F, 0x41}, "HP"},
    OuiEntry{{0xFC, 0x15, 0xB4}, "HP"},
    OuiEntry{{0xFC, 0x3F, 0x7C}, "HP"},
    // Intel
    OuiEntry{{0x00, 0x02, 0xB3}, "Intel"},
    OuiEntry{{0x00, 0x03, 0x47}, "Intel"},
    OuiEntry{{0x00, 0x04, 0x23}, "Intel"},
    OuiEntry{{0x00, 0x0C, 0xF1}, "Intel"},
    OuiEntry{{0x00, 0x13, 0x02}, "Intel"},
    OuiEntry{{0x00, 0x13, 0x20}, "Intel"},
    OuiEntry{{0x00, 0x13, 0xE8}, "Intel"},
    OuiEntry{{0x00, 0x15, 0x00}, "Intel"},
    OuiEntry{{0x00, 0x15, 0x17}, "Intel"},
    OuiEntry{{0x00, 0x16, 0x6F}, "Intel"},
    OuiEntry{{0x00, 0x16, 0x76}, "Intel"},
    OuiEntry{{0x00, 0x16, 0xEB}, "Intel"},
    OuiEntry{{0x00, 0x18, 0xDE}, "Intel"},
    OuiEntry{{0x00, 0x1B, 0x21}, "Intel"},
    OuiEntry{{0x00, 0x1B, 0x77}, "Intel"},
    OuiEntry{{0x00, 0x1C, 0xBF}, "Intel"},
    OuiEntry{{0x00, 0x1C, 0xC0}, "Intel"},
    OuiEntry{{0x00, 0x1D, 0xE0}, "Intel"},
    OuiEntry{{0x00, 0x1E, 0x64}, "Intel"},
    OuiEntry{{0x00, 0x1E, 0x65}, "Intel"},
    OuiEntry{{0x00, 0x1E, 0x67}, "Intel"},
    OuiEntry{{0x00, 0x1F, 0x3B}, "Intel"},
    OuiEntry{{0x00, 0x1F, 0x3C}, "Intel"},
    OuiEntry{{0x00, 0x21, 0x5C}, "Intel"},
    OuiEntry{{0x00, 0x21, 0x5D}, "Intel"},
    OuiEntry{{0x00, 0x21, 0x6A}, "Intel"},
    OuiEntry{{0x00, 0x21, 0x6B}, "Intel"},
    OuiEntry{{0x00, 0x22, 0xFA}, "Intel"},
    OuiEntry{{0x00, 0x23, 0x14}, "Intel"},
    OuiEntry{{0x00, 0x23, 0x15}, "Intel"},
    OuiEntry{{0x00, 0x24, 0xD6}, "Intel"},
    OuiEntry{{0x00, 0x24, 0xD7}, "Intel"},
    OuiEntry{{0x00, 0x26, 0xC6}, "Intel"},
    OuiEntry{{0x00, 0x26, 0xC7}, "Intel"},
    OuiEntry{{0x00, 0x27, 0x0E}, "Intel"},
    OuiEntry{{0x00, 0x27, 0x10}, "Intel"},
    OuiEntry{{0x00, 0x27, 0xE1}, "Intel"},
    OuiEntry{{0x00, 0xDB, 0xDF}, "Intel"},
    OuiEntry{{0x08, 0x11, 0x96}, "Intel"},
    OuiEntry{{0x0C, 0x8B, 0xFD}, "Intel"},
    OuiEntry{{0x0C, 0xD2, 0x92}, "Intel"},
    OuiEntry{{0x10, 0x0B, 0xA9}, "Intel"},
    OuiEntry{{0x10, 0xF0, 0x05}, "Intel"},
    OuiEntry{{0x18, 0x3D, 0xA2}, "Intel"},
    OuiEntry{{0x1C, 0x69, 0x7A}, "Intel"},
    OuiEntry{{0x1C, 0xC1, 0xDE}, "Intel"},
    OuiEntry{{0x24, 0x77, 0x03}, "Intel"},
    OuiEntry{{0x28, 0xB2, 0xBD}, "Intel"},
    OuiEntry{{0x28, 0xC6, 0x3F}, "Intel"},
    OuiEntry{{0x2C, 0x6E, 0x85}, "Intel"},
    OuiEntry{{0x2C, 0xF0, 0x5D}, "Intel"},
    OuiEntry{{0x30, 0x3A, 0x64}, "Intel"},
    OuiEntry{{0x30, 0xE1, 0x71}, "Intel"},
    OuiEntry{{0x34, 0x13, 0xE8}, "Intel"},
    OuiEntry{{0x34, 0x23, 0x87}, "Intel"},
    OuiEntry{{0x34, 0xE6, 0xAD}, "Intel"},
    OuiEntry{{0x3C, 0xA9, 0xF4}, "Intel"},
    OuiEntry{{0x3C, 0xFD, 0xFE}, "Intel"},
    OuiEntry{{0x40, 0x25, 0xC2}, "Intel"},
    OuiEntry{{0x40, 0xA8, 0xF0}, "Intel"},
    OuiEntry{{0x44, 0x85, 0x00}, "Intel"},
    OuiEntry{{0x48, 0x51, 0xB7}, "Intel"},
    OuiEntry{{0x4C, 0x34, 0x88}, "Intel"},
    OuiEntry{{0x4C, 0x79, 0x6E}, "Intel"},
    OuiEntry{{0x4C, 0x80, 0x93}, "Intel"},
    OuiEntry{{0x50, 0x2D, 0xA4}, "Intel"},
    OuiEntry{{0x50, 0xEB, 0xF6}, "Intel"},
    OuiEntry{{0x54, 0x05, 0xDB}, "Intel"},
    OuiEntry{{0x54, 0x67, 0x51}, "Intel"},
    OuiEntry{{0x58, 0x91, 0xCF}, "Intel"},
    OuiEntry{{0x58, 0xA0, 0x23}, "Intel"},
    OuiEntry{{0x5C, 0x51, 0x4F}, "Intel"},
    OuiEntry{{0x5C, 0xE0, 0xC5}, "Intel"},
    OuiEntry{{0x60, 0x36, 0xDD}, "Intel"},
    OuiEntry{{0x60, 0x67, 0x20}, "Intel"},
    OuiEntry{{0x60, 0xF8, 0x1D}, "Intel"},
    OuiEntry{{0x64, 0x80, 0x99}, "Intel"},
    OuiEntry{{0x64, 0xD4, 0xDA}, "Intel"},
    OuiEntry{{0x68, 0x05, 0xCA}, "Intel"},
    OuiEntry{{0x68, 0x17, 0x29}, "Intel"},
    OuiEntry{{0x6C, 0x29, 0x95}, "Intel"},
    OuiEntry{{0x6C, 0x88, 0x14}, "Intel"},
    OuiEntry{{0x6C, 0xC2, 0x17}, "Intel"},
    OuiEntry{{0x70, 0x1C, 0xE7}, "Intel"},
    OuiEntry{{0x70, 0x38, 0xEE}, "Intel"},
    OuiEntry{{0x70, 0xF0, 0x87}, "Intel"},
    OuiEntry{{0x74, 0xE5, 0xF9}, "Intel"},
    OuiEntry{{0x78, 0x0C, 0xB8}, "Intel"},
    OuiEntry{{0x7C, 0x5C, 0xF8}, "Intel"},
    OuiEntry{{0x7C, 0x7A, 0x91}, "Intel"},
    OuiEntry{{0x80, 0x00, 0x0B}, "Intel"},
    OuiEntry{{0x80, 0x19, 0x34}, "Intel"},
    OuiEntry{{0x80, 0x86, 0xF2}, "Intel"},
    OuiEntry{{0x84, 0x3A, 0x4B}, "Intel"},
    OuiEntry{{0x84, 0xA6, 0xC8}, "Intel"},
    OuiEntry{{0x88, 0x53, 0x95}, "Intel"},
    OuiEntry{{0x8C, 0x55, 0x4A}, "Intel"},
    OuiEntry{{0x8C, 0x70, 0x5A}, "Intel"},
    OuiEntry{{0x90, 0xE2, 0xBA}, "Intel"},
    OuiEntry{{0x94, 0x65, 0x9C}, "Intel"},
    OuiEntry{{0x98, 0x4F, 0xEE}, "Intel"},
    OuiEntry{{0x9C, 0xB6, 0xD0}, "Intel"},
    OuiEntry{{0x9C, 0xDA, 0x3E}, "Intel"},
    OuiEntry{{0xA0, 0x36, 0x9F}, "Intel"},
    OuiEntry{{0xA0, 0x88, 0x69}, "Intel"},
    OuiEntry{{0xA0, 0xC5, 0xF2}, "Intel"},
    OuiEntry{{0xA4, 0x34, 0xD9}, "Intel"},
    OuiEntry{{0xA4, 0xC4, 0x94}, "Intel"},
    OuiEntry{{0xAC, 0x7B, 0xA1}, "Intel"},
    OuiEntry{{0xAC, 0xFD, 0xCE}, "Intel"},
    OuiEntry{{0xB4, 0xB6, 0x76}, "Intel"},
    OuiEntry{{0xB4, 0xE1, 0x0F}, "Intel"},
    OuiEntry{{0xBC, 0x77, 0x37}, "Intel"},
    OuiEntry{{0xBC, 0xA0, 0xF1}, "Intel"},
    OuiEntry{{0xC0, 0x3F, 0xD5}, "Intel"},
    OuiEntry{{0xC4, 0x65, 0x16}, "Intel"},
    OuiEntry{{0xC4, 0x8A, 0x5A}, "Intel"},
    OuiEntry{{0xC8, 0x0E, 0x14}, "Intel"},
    OuiEntry{{0xC8, 0x34, 0x8D}, "Intel"},
    OuiEntry{{0xC8, 0xF7, 0x33}, "Intel"},
    OuiEntry{{0xCC, 0x3E, 0x5F}, "Intel"},
    OuiEntry{{0xD0, 0x7E, 0x35}, "Intel"},
    OuiEntry{{0xD4, 0x3B, 0x04}, "Intel"},
    OuiEntry{{0xD8, 0xFC, 0x93}, "Intel"},
    OuiEntry{{0xDC, 0xFB, 0x48}, "Intel"},
    OuiEntry{{0xE8, 0xB1, 0xFC}, "Intel"},
    OuiEntry{{0xEC, 0xA8, 0x6B}, "Intel"},
    OuiEntry{{0xF0, 0xDE, 0xF1}, "Intel"},
    OuiEntry{{0xF4, 0x4D, 0x30}, "Intel"},
    OuiEntry{{0xF8, 0x34, 0x41}, "Intel"},
    OuiEntry{{0xF8, 0x63, 0x3F}, "Intel"},
    OuiEntry{{0xFC, 0xF8, 0xAE}, "Intel"},
};

// ═══════════════════════════════════════════════════════════════════════════════
NetworkDiscovery::NetworkDiscovery(QObject *parent)
    : QObject(parent)
    , mDispatchTimer(new QTimer(this))
{
    mDispatchTimer->setInterval(static_cast<int>(DISPATCH_INTERVAL.count()));
    mDispatchTimer->setSingleShot(false);
    connect(mDispatchTimer, &QTimer::timeout,
            this, &NetworkDiscovery::onDispatchTick);
}

NetworkDiscovery::~NetworkDiscovery()
{
    cancel();
    for (auto &w : std::as_const(mActiveWorkers)) {
        if (w.process) {
            w.process->kill();
            w.process->waitForFinished(50);
        }
    }
    mActiveWorkers.clear();
}

// ═══════════════════════════════════════════════════════════════════════════════
QStringList NetworkDiscovery::detectLocalSubnets() noexcept
{
    QStringList result;
    for (QNetworkInterface const &iface : QNetworkInterface::allInterfaces()) {
        if (iface.flags().testFlag(QNetworkInterface::IsLoopBack)) continue;
        if (!iface.flags().testFlag(QNetworkInterface::IsUp))         continue;
        if (!iface.flags().testFlag(QNetworkInterface::IsRunning))    continue;

        for (QNetworkAddressEntry const &entry : iface.addressEntries()) {
            QHostAddress const addr = entry.ip();
            if (addr.protocol() != QAbstractSocket::IPv4Protocol) continue;
            if (addr.isLoopback())                                 continue;
            if (addr.isLinkLocal())                                continue;

            quint32 const ip     = addr.toIPv4Address();
            int      const prefix = entry.prefixLength();
            if (prefix == 0 || prefix > 32) continue;

            quint32 const hostBits = (prefix >= 32) ? 0u : ((1u << (32 - prefix)) - 1u);
            quint32 const base     = ip & ~hostBits;

            QString const baseStr = QString::asprintf(
                "%u.%u.%u",
                (base >> 24) & 0xFFu,
                (base >> 16) & 0xFFu,
                (base >>  8) & 0xFFu);
            if (!result.contains(baseStr)) {
                result.append(baseStr);
            }
        }
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════════════════
std::optional<NetworkDiscovery::SubnetRange>
NetworkDiscovery::parseSubnet(const QString &subnet) noexcept
{
    QString s = subnet.trimmed();
    if (s.isEmpty()) return std::nullopt;

    int prefix = 24;
    if (s.contains(QLatin1Char('/'))) {
        QStringList const parts = s.split(QLatin1Char('/'));
        if (parts.size() != 2) return std::nullopt;
        s = parts.first();
        bool ok = false;
        prefix = parts.last().toInt(&ok);
        if (!ok || prefix < 16 || prefix > 32) return std::nullopt;
    }

    QStringList const octets = s.split(QLatin1Char('.'));
    if (octets.size() == 3) {
        for (QString const &o : std::as_const(octets)) {
            bool ok = false;
            int const v = o.toInt(&ok);
            if (!ok || v < 0 || v > 255) return std::nullopt;
        }
        return SubnetRange{s, 1, 254};
    }
    if (octets.size() == 4) {
        for (QString const &o : std::as_const(octets)) {
            bool ok = false;
            int const v = o.toInt(&ok);
            if (!ok || v < 0 || v > 255) return std::nullopt;
        }
        QString const base = QString::asprintf("%s.%s.%s",
            qUtf8Printable(octets[0]),
            qUtf8Printable(octets[1]),
            qUtf8Printable(octets[2]));
        int const last = octets[3].toInt();
        return SubnetRange{base, last, last};
    }
    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════════════════
QStringList NetworkDiscovery::buildCandidateIps(
    const SubnetRange &range) noexcept
{
    QStringList out;
    out.reserve(range.lastHost - range.firstHost + 1);
    for (int i = range.firstHost; i <= range.lastHost; ++i) {
        out.append(QString::asprintf("%s.%d", qUtf8Printable(range.base), i));
    }
    return out;
}

// ═══════════════════════════════════════════════════════════════════════════════
QString NetworkDiscovery::lookupMac(const QString &ip) noexcept
{
    QFile f(QStringLiteral("/proc/net/arp"));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return {};

    QTextStream in(&f);
    QString line = in.readLine();   // header
    while (!(line = in.readLine()).isNull()) {
        QStringList const cols = line.split(
            QRegularExpression(QStringLiteral("\\s+")));
        if (cols.size() < 4) continue;
        if (cols.first() != ip) continue;
        QString const mac = cols.at(3);
        if (mac == QStringLiteral("00:00:00:00:00:00")) return {};
        if (!mac.contains(QLatin1Char(':')))            return {};
        return mac.toLower();
    }
    return {};
}

// ═══════════════════════════════════════════════════════════════════════════════
QString NetworkDiscovery::lookupVendor(const QString &mac) noexcept
{
    if (mac.size() < 8) return {};
    QStringList const parts = mac.split(QLatin1Char(':'));
    if (parts.size() < 3) return {};
    bool ok1 = false, ok2 = false, ok3 = false;
    auto const a = static_cast<std::uint8_t>(parts[0].toUInt(&ok1, 16));
    auto const b = static_cast<std::uint8_t>(parts[1].toUInt(&ok2, 16));
    auto const c = static_cast<std::uint8_t>(parts[2].toUInt(&ok3, 16));
    if (!ok1 || !ok2 || !ok3) return {};

    for (auto const &e : OUI_TABLE) {
        if (e.prefix[0] == a && e.prefix[1] == b && e.prefix[2] == c) {
            return QString::fromLatin1(e.vendor.data(),
                                       static_cast<int>(e.vendor.size()));
        }
    }
    return {};
}

// ═══════════════════════════════════════════════════════════════════════════════
void NetworkDiscovery::startDiscovery(const QString &subnet) noexcept
{
    if (mRunning.load()) {
        emit error(QStringLiteral("Discovery already in progress."));
        return;
    }

    QString target = subnet.trimmed();
    if (target.isEmpty()) {
        QStringList const autos = detectLocalSubnets();
        if (autos.isEmpty()) {
            emit error(QStringLiteral(
                "No active IPv4 interface found. Connect to a network first."));
            return;
        }
        target = autos.first();
    }
    auto parsed = parseSubnet(target);
    if (!parsed) {
        emit error(QStringLiteral("Invalid subnet: %1").arg(target));
        return;
    }
    SubnetRange const range = *parsed;

    mPendingIps    = buildCandidateIps(range);
    mScannedCount  = 0;
    mTotalCount    = static_cast<int>(mPendingIps.size());
    mRunning.store(true);
    mCancelRequested.store(false);
    mInFlightLookupIp.clear();
    mInFlightLookupLatency = 0;
    mPendingReverseLookups.clear();

    IPView::Logger::info(
        "NetworkDiscovery: starting sweep over %s (%d hosts)",
        qPrintable(target), mTotalCount);

    // Fill the worker pool right away. dispatchNext() returns
    // false when the pool is full or the queue is empty.
    for (int i = 0; i < MAX_PARALLEL_PINGS && dispatchNext(); ++i) { }
    mDispatchTimer->start();
    emit progress(0, mTotalCount);
}

void NetworkDiscovery::cancel() noexcept
{
    if (!mRunning.load()) return;
    mCancelRequested.store(true);
    mDispatchTimer->stop();
    for (auto &w : std::as_const(mActiveWorkers)) {
        if (w.process) w.process->kill();
    }
    mActiveWorkers.clear();
    mPendingIps.clear();
    mPendingReverseLookups.clear();
    mInFlightLookupIp.clear();
    mInFlightLookupLatency = 0;
    mRunning.store(false);
    IPView::Logger::info("NetworkDiscovery: cancelled.");
    emit cancelled();
}

bool NetworkDiscovery::dispatchNext() noexcept
{
    if (mCancelRequested.load() || mPendingIps.isEmpty()) return false;
    if (mActiveWorkers.size() >= MAX_PARALLEL_PINGS)      return false;

    QString const ip = mPendingIps.takeFirst();
    auto *proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);

    PingWorker w;
    w.process = proc;
    w.ip      = ip;
    w.timer.start();

    // The slot looks up the matching worker by sender() QProcess*,
    // so the lambda does not need to capture the worker state.
    connect(proc, &QProcess::finished, this,
        [this](int code, QProcess::ExitStatus /*st*/) {
            this->onProcessFinished(code);
        });

    mActiveWorkers.append(w);
    proc->start(QStringLiteral("ping"),
                { QStringLiteral("-c"), QStringLiteral("1"),
                  QStringLiteral("-W"), QStringLiteral("1"),
                  ip });
    return true;
}

void NetworkDiscovery::onProcessFinished(int exitCode)
{
    mScannedCount++;
    emit progress(mScannedCount, mTotalCount);

    if (mCancelRequested.load()) {
        if (mActiveWorkers.isEmpty() && mPendingIps.isEmpty()) {
            mRunning.store(false);
        }
        return;
    }

    // We cannot tell from the lambda which worker the sender
    // belongs to (we deliberately did not capture the iterator),
    // so we locate it by the QProcess pointer that is still the
    // sender's identity until deleteLater() takes effect.
    auto *proc = qobject_cast<QProcess*>(sender());
    if (proc) {
        auto it = std::ranges::find_if(mActiveWorkers,
            [proc](PingWorker const &pw) { return pw.process == proc; });
        if (it != mActiveWorkers.end()) {
            if (exitCode == 0) {
                PendingLookup lookup;
                lookup.ip        = it->ip;
                lookup.latencyMs = static_cast<int>(it->timer.elapsed());
                mPendingReverseLookups.append(lookup);
            }
            it->process->deleteLater();
            mActiveWorkers.erase(it);
        }
    }

    // Kick off one pending reverse-DNS lookup if none is in flight.
    if (mInFlightLookupIp.isEmpty() && !mPendingReverseLookups.isEmpty()) {
        PendingLookup const next = mPendingReverseLookups.takeFirst();
        mInFlightLookupIp       = next.ip;
        mInFlightLookupLatency  = next.latencyMs;
        QHostInfo::lookupHost(next.ip, this,
            &IPView::Scanner::NetworkDiscovery::onHostInfoReady);
    }

    // Refill the worker pool.
    while (dispatchNext()) { /* keep filling */ }

    // Termination check: pool empty + queue empty + no pending
    // reverse-DNS = we are done.
    if (mActiveWorkers.isEmpty() && mPendingIps.isEmpty()
        && mPendingReverseLookups.isEmpty()
        && mInFlightLookupIp.isEmpty()) {
        mRunning.store(false);
        mDispatchTimer->stop();
        IPView::Logger::info(
            "NetworkDiscovery: completed (%1/%2 hosts up).",
            mScannedCount, mTotalCount);
        emit completed();
    }
}

void NetworkDiscovery::onHostInfoReady(const QHostInfo &host) noexcept
{
    QString const ip        = mInFlightLookupIp;
    int     const latencyMs = mInFlightLookupLatency;
    mInFlightLookupIp.clear();
    mInFlightLookupLatency = 0;

    DiscoveredDevice dev;
    dev.ip        = ip;
    dev.hostname  = (host.error() == QHostInfo::NoError) ? host.hostName()
                                                          : QString();
    dev.mac       = lookupMac(ip);
    dev.vendor    = dev.mac.isEmpty() ? QString() : lookupVendor(dev.mac);
    dev.latencyMs = latencyMs;
    emit deviceFound(dev);

    // Drain the next pending reverse-DNS lookup, if any.
    if (!mPendingReverseLookups.isEmpty()) {
        PendingLookup const next = mPendingReverseLookups.takeFirst();
        mInFlightLookupIp       = next.ip;
        mInFlightLookupLatency  = next.latencyMs;
        QHostInfo::lookupHost(next.ip, this,
            &IPView::Scanner::NetworkDiscovery::onHostInfoReady);
    } else if (mActiveWorkers.isEmpty() && mPendingIps.isEmpty()
               && mRunning.load()) {
        mRunning.store(false);
        mDispatchTimer->stop();
        IPView::Logger::info(
            "NetworkDiscovery: completed (%1/%2 hosts up).",
            mScannedCount, mTotalCount);
        emit completed();
    }
}

void NetworkDiscovery::onDispatchTick() noexcept
{
    if (mCancelRequested.load() || !mRunning.load()) return;

    // Catch any process that finished but did not trigger
    // onProcessFinished() (should not happen with QProcess, but
    // guards against a stuck worker in pathological teardown).
    for (auto it = mActiveWorkers.begin(); it != mActiveWorkers.end(); ) {
        if (it->process->state() == QProcess::NotRunning) {
            // Process exited; trigger the slot manually.
            int const code = it->process->exitCode();
            onProcessFinished(code);
            it = mActiveWorkers.begin();   // list mutated; restart
        } else {
            ++it;
        }
    }
}

} // namespace IPView::Scanner
