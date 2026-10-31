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
- Block transaction execution is ordered and all-or-nothing across the entire block transaction list.
- Ledger state roots commit to sorted address/balance/next-nonce leaves and accumulated fees using domain-separated SHA3-256.
- Block header V2 includes the state root in canonical serialization and block hashing; unsupported header versions are rejected.
- Block execution must reproduce the header state root before its candidate state can be committed.
- Signature verification is injected through a suite-aware interface; the generic fake suite 1 is test-only.
- An OpenSSL 3.5+ backend implements FIPS 204 ML-DSA-44/65/87 verification; the real ML-DSA-65 integration test uses reserved test-only suite 0x7FFE. No production suite ID is assigned.
- Hashing uses SHA3-256 via OpenSSL EVP.

## Storage direction

- ADR-0008 defines the vendor-neutral atomic LedgerStore boundary; it is an interface, not persistence.
- ADR-0011 documents a critical trust boundary: a durable backend must replay transactions from its own persisted parent state with a production signature verifier; matching a caller-supplied state root is not sufficient.
- ADR-0010 selects SQLite as the first durable-backend implementation candidate, subject to implementation, benchmarks, crash-recovery tests, and ARM64 validation.
- SQLite's signed 64-bit INTEGER must not be used to silently store the full uint64 protocol domain; encode values as fixed-width BLOBs or define an explicit protocol bound.

## Consensus status

- ADR-0009 defines the evaluation gate for PoW, PoS, and BFT-style PoS.
- Bitcoin-like adaptive PoW is the preferred candidate for detailed design evaluation; ADR-0014 specifies the proposed retarget, target validation, timestamp protections, cumulative-chain-work requirements, and adversarial test gates.
- PoW is NOT yet adopted. No target block interval, retarget window, proof hash, pow limit, timestamp estimator, chain-work encoding, fork-choice tie-break, reorg policy, miner eligibility, issuance, reward, or penalty parameters have been selected.
- Structural block validation is not consensus and does not authorize a block as canonical.

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
