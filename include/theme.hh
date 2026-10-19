// Monokai, with its pink as the primary color.
#pragma once

#include <raydom/raydom.hh>

#include <cstdint>

namespace theme {

constexpr std::uint32_t kBg     = 0x272822;  // Monokai's background
constexpr std::uint32_t kPanel  = 0x1e1f1c;
constexpr std::uint32_t kRaised = 0x3e3d32;
constexpr std::uint32_t kHover  = 0x49483e;
constexpr std::uint32_t kLine   = 0x34352d;
constexpr std::uint32_t kSpace  = 0x0b0b09;  // the canvas
constexpr std::uint32_t kText   = 0xf8f8f2;
constexpr std::uint32_t kMuted  = 0x75715e;  // comments
constexpr std::uint32_t kSoft   = 0xa6a28c;
constexpr std::uint32_t kPink   = 0xf92672;  // keywords; the primary color
constexpr std::uint32_t kPinkHi = 0xff4f8b;
constexpr std::uint32_t kPinkLo = 0xd81b60;
constexpr std::uint32_t kPurple = 0xae81ff;  // numbers
constexpr std::uint32_t kGreen  = 0xa6e22e;
constexpr std::uint32_t kYellow = 0xe6db74;
constexpr std::uint32_t kCyan   = 0x66d9ef;

// Restyles raydom_extras' controls for the context.
void apply(const rd::Context& ctx);

}  // namespace theme
