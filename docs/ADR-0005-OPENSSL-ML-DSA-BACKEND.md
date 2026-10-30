# ADR-0005: OpenSSL Provider-Backed ML-DSA Verification

Status: implemented backend; production suite assignment and activation remain pending.
Date: 2026-10-30

## Decision

Use OpenSSL's standardized provider-backed ML-DSA implementation as the first native signature-verification backend when OpenSSL 3.5 or later is available. Do not implement ML-DSA arithmetic in project code.

OpenSSL 3.5 documentation exposes ML-DSA-44, ML-DSA-65, and ML-DSA-87 through EVP key management and one-shot signature verification. The implementation supports key generation and sign/verify, and the raw public-key sizes are 1312, 1952, and 2592 bytes respectively. See the official OpenSSL 3.5 documentation and NIST FIPS 204.

## Implemented behavior

- `OpenSslMlDsaVerifier` is configured with an explicit suite ID and one of the exact algorithm names `ML-DSA-44`, `ML-DSA-65`, or `ML-DSA-87`.
- It imports the raw public key using OpenSSL EVP provider APIs.
- It verifies the exact transaction signing payload using one-shot ML-DSA verification.
- It rejects unsupported suite IDs, invalid public-key lengths, and invalid signature lengths before attempting verification.
- It uses RAII-managed OpenSSL objects and returns rejection if key import, provider fetch, or signature verification fails.
- CMake enables this backend only when OpenSSL 3.5+ is detected. Older OpenSSL builds retain the rest of the native foundation without compiling ML-DSA-specific code.
- Conditional tests generate a real ML-DSA-65 keypair, sign a transaction, and verify the transaction through the ledger; a mutated signature must be rejected without state mutation.

## Test-only identifiers

The integration test uses suite ID `0x7FFE`. This is reserved for tests in this repository and is not a production network suite ID. Generic ledger tests also use a fake suite-1 verifier; neither test path may be enabled in a production node configuration.

## Parameter-set sizes

| Algorithm | Public key | Signature |
|---|---:|---:|
| ML-DSA-44 | 1,312 bytes | 2,420 bytes |
| ML-DSA-65 | 1,952 bytes | 3,309 bytes |
| ML-DSA-87 | 2,592 bytes | 4,627 bytes |

These are raw FIPS 204 encodings. Any future alternative container encoding must be specified separately and must not be silently mixed with raw-key encoding.

## Why this does not select a production suite

The backend supports all three standardized parameter sets, but this ADR does not select a production algorithm or suite ID. That decision must consider transaction bandwidth, block weight, storage amplification, signing performance on supported devices, wallet key custody, hardware support, interoperability vectors, and the chain's threat model.

## Validation and activation gates

- Run official or independently sourced FIPS 204 known-answer tests in addition to project round-trip tests.
- Test malformed, truncated, wrong-length, and wrong-algorithm public keys.
- Test signature mutation and cross-suite rejection.
- Benchmark key generation, signing, and verification on target ARM64 Android/Linux and x86-64 node hardware.
- Review OpenSSL provider configuration and error handling.
- Decide whether the network requires the OpenSSL FIPS provider and what that means for deployment certification.
- Assign a production suite ID only through a separately approved protocol change.
- Complete independent cryptographic and wallet security review before any public network activation.

## Limitations

This backend currently provides verification only to the ledger. There is no production wallet/key-generation UX, secure private-key storage, recovery flow, signing service, production suite ID, or mainnet activation. ML-KEM transport key establishment is not implemented by this ADR.
