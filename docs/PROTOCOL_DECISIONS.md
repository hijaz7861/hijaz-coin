# HJZ Native Protocol — Initial Decisions and Open Parameters

Status: engineering draft; not a mainnet specification.

## Implemented prototype conventions

- Independent native chain; no EVM dependency in consensus.
- C++20 implementation.
- Consensus-critical amounts use unsigned integer base units, never floating point.
- Transaction V3 has explicit version, authorization suite, public-key bytes, chain ID, sender/receiver, amount, fee, nonce, and signature.
- Account address derives from authorization suite + canonical public key using domain-separated SHA3-256.
- The ledger rejects sender/public-key mismatches and unsupported authorization suites before state mutation.
- Transaction signing payload and transaction ID use distinct domain separation.
- Account-state prototype uses sequential nonces and explicit fee accounting.
- Ledger application is atomic: invalid transactions leave state unchanged.
- Signature verification is injected; suite 1 is test-only and uses a fake verifier.
- Hashing uses SHA3-256 via OpenSSL EVP.

## Open parameters — do not silently assume

1. Total supply, smallest unit, and display precision.
2. Genesis allocation, vesting, treasury, and reserve policy.
3. Fee schedule and fee recipient/burn policy.
4. Consensus family, validator/miner eligibility, Sybil resistance, and finality.
5. Block interval, maximum block weight, transaction limits, and timestamp tolerance.
6. Account model versus UTXO model for final protocol.
7. Production PQ signature algorithm/parameter set, suite ID, key rotation, and recovery.
8. Upgrade governance and emergency response policy.
9. State-root construction, snapshots, and trusted checkpoint model.
10. Network bootstrap, peer identity, anti-eclipse protections, and transport security.

These parameters require economic, security, and threat-model review before a public testnet with meaningful incentives or any mainnet launch.
