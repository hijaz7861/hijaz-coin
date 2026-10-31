#include "hijaz/hash.hpp"

#include <openssl/evp.h>

#include <array>
#include <limits>
#include <memory>
#include <stdexcept>

namespace hijaz {
namespace {
using MdContext = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

Hash256 digest(std::span<const std::uint8_t> input) {
    MdContext ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
    if (EVP_DigestInit_ex(ctx.get(), EVP_sha3_256(), nullptr) != 1)
        throw std::runtime_error("SHA3-256 initialization failed");
    if (!input.empty() && EVP_DigestUpdate(ctx.get(), input.data(), input.size()) != 1)
        throw std::runtime_error("SHA3-256 update failed");
    Hash256 out{};
    unsigned int length = 0;
    if (EVP_DigestFinal_ex(ctx.get(), out.data(), &length) != 1 || length != out.size())
        throw std::runtime_error("SHA3-256 finalization failed");
    return out;
}
} // namespace

Hash256 sha3_256(std::span<const std::uint8_t> input) { return digest(input); }

Hash256 domain_hash(std::string_view domain, std::span<const std::uint8_t> input) {
    if (domain.size() > std::numeric_limits<std::uint32_t>::max())
        throw std::length_error("hash domain is too long");

    MdContext ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
    if (!ctx) throw std::runtime_error("EVP_MD_CTX_new failed");
    if (EVP_DigestInit_ex(ctx.get(), EVP_sha3_256(), nullptr) != 1)
        throw std::runtime_error("SHA3-256 initialization failed");

    const auto domain_size = static_cast<std::uint32_t>(domain.size());
    const std::array<std::uint8_t, 4> encoded_size{
        static_cast<std::uint8_t>((domain_size >> 24U) & 0xffU),
        static_cast<std::uint8_t>((domain_size >> 16U) & 0xffU),
        static_cast<std::uint8_t>((domain_size >> 8U) & 0xffU),
        static_cast<std::uint8_t>(domain_size & 0xffU)
    };
    if (EVP_DigestUpdate(ctx.get(), encoded_size.data(), encoded_size.size()) != 1)
        throw std::runtime_error("SHA3-256 domain length update failed");
    if (!domain.empty() && EVP_DigestUpdate(ctx.get(), domain.data(), domain.size()) != 1)
        throw std::runtime_error("SHA3-256 domain update failed");
    if (!input.empty() && EVP_DigestUpdate(ctx.get(), input.data(), input.size()) != 1)
        throw std::runtime_error("SHA3-256 payload update failed");

    Hash256 out{};
    unsigned int length = 0;
    if (EVP_DigestFinal_ex(ctx.get(), out.data(), &length) != 1 || length != out.size())
        throw std::runtime_error("SHA3-256 finalization failed");
    return out;
}

std::string to_hex(const Hash256& value) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(value.size() * 2);
    for (const auto byte : value) {
        out.push_back(digits[byte >> 4U]);
        out.push_back(digits[byte & 0x0fU]);
    }
    return out;
}
} // namespace hijaz
