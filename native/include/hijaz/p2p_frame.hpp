#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace hijaz::p2p {

inline constexpr std::uint32_t protocol_magic = 0x484A5A31U; // "HJZ1"
inline constexpr std::uint32_t protocol_version = 1;
inline constexpr std::size_t frame_header_size = 14;
inline constexpr std::size_t max_payload_size = 2U * 1024U * 1024U;

enum class MessageType : std::uint16_t {
    Version = 1,
    VerAck = 2,
    Ping = 3,
    Pong = 4,
    GetHeaders = 5,
    Headers = 6,
    GetBlock = 7,
    Block = 8,
    Transaction = 9,
    Reject = 10
};

enum class FrameError { None, TooShort, BadMagic, UnsupportedVersion, Oversized, LengthMismatch, UnknownType };

struct Frame {
    MessageType type;
    std::vector<std::uint8_t> payload;
};

[[nodiscard]] std::vector<std::uint8_t> encode_frame(MessageType type,
    std::span<const std::uint8_t> payload);
[[nodiscard]] FrameError decode_frame(std::span<const std::uint8_t> bytes, Frame& out);

} // namespace hijaz::p2p
