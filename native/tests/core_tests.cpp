#include "hijaz/block.hpp"
#include "hijaz/hash.hpp"
#include "hijaz/transaction.hpp"

#include <cassert>
#include <iostream>

int main() {
    using namespace hijaz;

    const std::vector<std::uint8_t> empty;
    assert(to_hex(sha3_256(empty)) ==
        "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a");

    Transaction tx;
    tx.sender[0] = 1;
    tx.receiver[0] = 2;
    tx.amount = 100;
    tx.nonce = 7;
    tx.signature = {0x01, 0x02, 0x03};
    assert(tx.structurally_valid());

    const auto first_id = tx.id();
    tx.signature.push_back(0x04);
    assert(tx.id() != first_id);
    assert(merkle_root({tx}) != merkle_root({}));

    BlockHeader header;
    header.height = 1;
    header.merkle_root = merkle_root({tx});
    const auto first_block_hash = header.hash();
    ++header.nonce;
    assert(header.hash() != first_block_hash);

    tx.amount = 0;
    assert(!tx.structurally_valid());

    std::cout << "HJZ native foundation tests: PASS\n";
}
