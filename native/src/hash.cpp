#include "hijaz/hash.hpp"

#include <openssl/evp.h>

#include <memory>
#include <stdexcept>
#include <vector>

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
    std::vector<std::uint8_t> framed;
    framed.reserve(4 + domain.size() + input.size());
    const auto n = static_cast<std::uint32_t>(domain.size());
    for (int shift = 24; shift >= 0; shift -= 8)
        framed.push_back(static_cast<std::uint8_t>((n >> shift) & 0xffU));
    framed.insert(framed.end(), domain.begin(), domain.end());
    framed.insert(framed.end(), input.begin(), input.end());
    return digest(framed);
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
