#include "hijaz/p2p_frame.hpp"

#include <limits>

namespace hijaz::p2p {
namespace {
void put_u16(std::vector<std::uint8_t>& out, std::uint16_t v) {
    out.push_back(static_cast<std::uint8_t>(v >> 8U));
    out.push_back(static_cast<std::uint8_t>(v));
}
void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int s = 24; s >= 0; s -= 8) out.push_back(static_cast<std::uint8_t>(v >> s));
}
std::uint16_t get_u16(std::span<const std::uint8_t> b, std::size_t p) {
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(b[p]) << 8U) | b[p + 1]);
}
std::uint32_t get_u32(std::span<const std::uint8_t> b, std::size_t p) {
    return (static_cast<std::uint32_t>(b[p]) << 24U) |
           (static_cast<std::uint32_t>(b[p + 1]) << 16U) |
           (static_cast<std::uint32_t>(b[p + 2]) << 8U) |
           static_cast<std::uint32_t>(b[p + 3]);
}
bool known_type(std::uint16_t v) { return v >= 1 && v <= 10; }
}

std::vector<std::uint8_t> encode_frame(MessageType type, std::span<const std::uint8_t> payload) {
    if (payload.size() > max_payload_size) return {};
    std::vector<std::uint8_t> out;
    out.reserve(frame_header_size + payload.size());
    put_u32(out, protocol_magic);
    put_u32(out, protocol_version);
    put_u16(out, static_cast<std::uint16_t>(type));
    put_u32(out, static_cast<std::uint32_t>(payload.size()));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

FrameError decode_frame(std::span<const std::uint8_t> bytes, Frame& out) {
    if (bytes.size() < frame_header_size + 4U) return FrameError::TooShort;
    if (get_u32(bytes, 0) != protocol_magic) return FrameError::BadMagic;
    if (get_u32(bytes, 4) != protocol_version) return FrameError::UnsupportedVersion;
    const auto raw_type = get_u16(bytes, 8);
    if (!known_type(raw_type)) return FrameError::UnknownType;
    const auto length = get_u32(bytes, 10);
    if (length > max_payload_size) return FrameError::Oversized;
    if (bytes.size() != frame_header_size + 4U + length) return FrameError::LengthMismatch;
    out.type = static_cast<MessageType>(raw_type);
    out.payload.assign(bytes.begin() + static_cast<std::ptrdiff_t>(frame_header_size + 4U), bytes.end());
    return FrameError::None;
}
} // namespace hijaz::p2p
