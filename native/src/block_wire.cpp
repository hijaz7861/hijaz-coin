#include "hijaz/block_wire.hpp"

#include <algorithm>
#include <limits>

namespace hijaz {
namespace {
void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int shift=24; shift>=0; shift-=8)
        out.push_back(static_cast<std::uint8_t>((v >> shift) & 0xffU));
}
void put_u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (int shift=56; shift>=0; shift-=8)
        out.push_back(static_cast<std::uint8_t>((v >> shift) & 0xffU));
}
bool get_u32(std::span<const std::uint8_t> b, std::size_t& p, std::uint32_t& v) {
    if (p > b.size() || b.size()-p < 4U) return false;
    v=(static_cast<std::uint32_t>(b[p])<<24U) |
      (static_cast<std::uint32_t>(b[p+1U])<<16U) |
      (static_cast<std::uint32_t>(b[p+2U])<<8U) |
       static_cast<std::uint32_t>(b[p+3U]);
    p+=4U; return true;
}
bool get_u64(std::span<const std::uint8_t> b, std::size_t& p, std::uint64_t& v) {
    if (p > b.size() || b.size()-p < 8U) return false;
    v=0; for (std::size_t i=0;i<8U;++i) v=(v<<8U)|b[p+i];
    p+=8U; return true;
}
void put_hash(std::vector<std::uint8_t>& out, const Hash256& h) {
    out.insert(out.end(),h.begin(),h.end());
}
bool get_hash(std::span<const std::uint8_t> b, std::size_t& p, Hash256& h) {
    if (p > b.size() || b.size()-p < h.size()) return false;
    std::copy_n(b.begin()+static_cast<std::ptrdiff_t>(p),h.size(),h.begin());
    p+=h.size(); return true;
}
} // namespace

std::vector<std::uint8_t> encode_block(const Block& block) {
    if (block.header.version != 2U || block.transactions.size() > max_block_transactions)
        return {};
    std::vector<std::uint8_t> out;
    out.reserve(4U+1U+124U+4U+block.transactions.size()*128U);
    out.insert(out.end(),{'H','J','Z','B'});
    out.push_back(1U);
    put_u32(out,block.header.version);
    put_u64(out,block.header.height);
    put_u64(out,block.header.timestamp);
    put_hash(out,block.header.previous_hash);
    put_hash(out,block.header.merkle_root);
    put_hash(out,block.header.state_root);
    put_u64(out,block.header.nonce);
    put_u32(out,static_cast<std::uint32_t>(block.transactions.size()));
    for (const auto& tx:block.transactions) {
        if (!tx.structurally_valid()) return {};
        const auto encoded=tx.canonical_bytes();
        if (encoded.size()>std::numeric_limits<std::uint32_t>::max() ||
            out.size()>max_block_wire_bytes-4U ||
            encoded.size()>max_block_wire_bytes-out.size()-4U) return {};
        put_u32(out,static_cast<std::uint32_t>(encoded.size()));
        out.insert(out.end(),encoded.begin(),encoded.end());
    }
    if (out.size()>max_block_wire_bytes) return {};
    return out;
}

BlockDecodeError decode_block(std::span<const std::uint8_t> bytes, Block& out) {
    constexpr std::size_t fixed_size=5U+4U+8U+8U+32U+32U+32U+8U+4U;
    if (bytes.size()<fixed_size) return BlockDecodeError::TooShort;
    if (bytes.size()>max_block_wire_bytes) return BlockDecodeError::InvalidLength;
    constexpr std::uint8_t magic[]={'H','J','Z','B'};
    for (std::size_t i=0;i<4U;++i)
        if (bytes[i]!=magic[i]) return BlockDecodeError::InvalidDomain;
    if (bytes[4]!=1U) return BlockDecodeError::UnsupportedVersion;
    std::size_t p=5U;
    Block candidate;
    if (!get_u32(bytes,p,candidate.header.version) ||
        !get_u64(bytes,p,candidate.header.height) ||
        !get_u64(bytes,p,candidate.header.timestamp) ||
        !get_hash(bytes,p,candidate.header.previous_hash) ||
        !get_hash(bytes,p,candidate.header.merkle_root) ||
        !get_hash(bytes,p,candidate.header.state_root) ||
        !get_u64(bytes,p,candidate.header.nonce))
        return BlockDecodeError::TooShort;
    if (candidate.header.version!=2U) return BlockDecodeError::UnsupportedVersion;
    std::uint32_t count=0;
    if (!get_u32(bytes,p,count)) return BlockDecodeError::TooShort;
    if (count>max_block_transactions) return BlockDecodeError::TooManyTransactions;
    candidate.transactions.reserve(count);
    for (std::uint32_t i=0;i<count;++i) {
        std::uint32_t length=0;
        if (!get_u32(bytes,p,length)) return BlockDecodeError::TooShort;
        if (length==0U || p>bytes.size() || length>bytes.size()-p)
            return BlockDecodeError::InvalidLength;
        Transaction tx;
        const auto error=decode_transaction(bytes.subspan(p,length),tx);
        if (error!=TransactionDecodeError::None) return BlockDecodeError::InvalidTransaction;
        candidate.transactions.push_back(std::move(tx));
        p+=length;
    }
    if (p!=bytes.size()) return BlockDecodeError::NonCanonical;
    const auto canonical=encode_block(candidate);
    if (canonical.size()!=bytes.size() ||
        !std::equal(canonical.begin(),canonical.end(),bytes.begin()))
        return BlockDecodeError::NonCanonical;
    out=std::move(candidate);
    return BlockDecodeError::None;
}
} // namespace hijaz
