# ADR-0004: Transaction V3 Canonical Wire Format

Status: implemented prototype format; production activation is not approved.
Date: 2026-10-30

## Purpose

Specify the current source-level Transaction V3 encoding so nodes do not interpret authorization data differently. This format is a prototype until production cryptographic suite IDs, test vectors, and network activation rules are reviewed.

## Fields

In order, the unsigned signing payload is:

| Field | Encoding |
|---|---|
| Domain marker | five bytes: ASCII `HJZT` followed by `0x03` |
| Format version | u8; must equal 3 |
| Authorization suite | u16 big-endian; zero is invalid |
| Chain ID | u32 big-endian |
| Public-key length | u32 big-endian |
| Canonical public key | exactly the preceding length; 1..16384 bytes |
| Sender address | exactly 32 bytes |
| Receiver address | exactly 32 bytes |
| Amount | u64 big-endian; nonzero |
| Fee | u64 big-endian |
| Nonce | u64 big-endian |
| Signature length | u64 big-endian in canonical full transaction |
| Signature | exactly the preceding length; 1..16384 bytes |

The sender address is derived as:

`domain_hash("HJZ/ADDRESS/V1", u16_be(authorization_suite) || canonical_public_key)`

The `domain_hash` helper encodes the domain as a u32 big-endian byte length followed by the domain bytes and then the input bytes, before SHA3-256.

The signing payload includes all fields above through nonce, but excludes signature length and signature bytes. The complete canonical transaction appends signature length and signature bytes to the signing payload.

The transaction ID is:

`domain_hash("HJZ/TXID/V3", canonical_transaction_bytes)`

## Validation requirements

- Reject any format version other than 3.
- Reject suite ID zero and any suite not explicitly supported by the configured verifier.
- Reject public keys or signatures outside the stated bounds before expensive processing.
- Recompute sender address and reject mismatch.
- Verify signature over the exact signing payload.
- Check chain ID, nonce, balance, fee, and state transition.
- Any rejection must leave ledger state unchanged.

## Security and compatibility notes

- The current test suite ID 1 is only a fake unit-test suite. It is not a production algorithm assignment.
- No production ML-DSA parameter set or suite ID is selected by this ADR.
- Do not silently downgrade unknown suite IDs.
- Do not reinterpret V2 serialized transactions as V3.
- The 16 KiB field bounds are prototype limits, not a claim that they are optimal for ML-DSA or future suites.
- A production protocol must publish known-answer vectors for address derivation, signing payload bytes, canonical transaction bytes, and transaction IDs.

## Activation gate

This ADR documents current prototype behavior only. A production network must separately approve the cryptographic suite, public-key canonical encoding, signature encoding, resource limits, transaction activation/version policy, and interoperability test vectors.


## Decoder implementation gate

The current prototype implements canonical encoding and hashing only. It does **not** yet provide a canonical byte-to-Transaction decoder. Therefore, the format must not be treated as a complete network/storage interchange implementation.

Before a durable store or P2P message handler accepts serialized transactions, an independently tested decoder must:

- Parse from a bounded byte span; never allocate based solely on an untrusted length field.
- Require the exact five-byte domain marker and format version.
- Read all fixed-width integers in big-endian order and reject truncated fields.
- Check public-key and signature lengths against the 1..16384 byte bounds before copying or allocating.
- Check every length conversion and offset addition for overflow and remaining-input bounds.
- Require the exact canonical field order and reject trailing bytes.
- Reject empty/invalid fields and run structural validation after parsing.
- Re-encode the parsed object and require byte-for-byte equality with the input as a canonicality invariant.
- Leave the caller's output object unchanged on every parse failure.
- Return typed parse errors that distinguish truncation, invalid domain/version, invalid length, overflow, non-canonical encoding, and structural invalidity.

Required tests include every truncation boundary, zero and maximum lengths, maximum-plus-one lengths, uint32/uint64 boundary values, trailing bytes, altered domain/version, malformed field combinations, round-trip vectors, and failure-with-output-unchanged behavior. Fuzz the decoder with sanitizers before exposing it to untrusted input.

A separate block wire-format decision is also required before block persistence or network decoding. The current `BlockHeader::canonical_bytes()` serializes only the header; it is not a complete block encoding and does not define transaction-count or per-transaction framing. Do not invent an implicit block container format in storage code.
