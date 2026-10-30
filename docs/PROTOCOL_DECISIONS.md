# HJZ Native Protocol — Initial Decisions and Open Parameters

Status: engineering draft. These decisions are not a mainnet specification.

## Chosen implementation conventions

- Native independent chain; no EVM dependency in consensus.
- C++20 implementation.
- Consensus-critical amounts use unsigned integer base units, never floating point.
- Transaction encoding is deterministic, fixed-width for scalar fields, versioned, and domain-separated.
- Transactions bind to a chain ID to prevent cross-network replay.
- Account-state prototype uses sequential nonces and explicit fee accounting.
- Ledger application is atomic: invalid transactions leave the state unchanged.
- Signature verification is an injected interface. No placeholder or home-grown signature algorithm is accepted as production cryptography.
- Hashing uses SHA3-256 via OpenSSL EVP in the current foundation.

## Still unresolved — do not silently assume

1. Total supply and smallest unit / decimal display.
2. Genesis allocation, vesting, treasury and reserve policy.
3. Fee schedule and fee recipient/burn policy.
4. Consensus family, validator/miner eligibility, Sybil resistance and finality.
5. Block interval, maximum block weight, transaction limits and timestamp tolerance.
6. Account model versus UTXO model for the final protocol.
7. PQ signature parameter set, key/address encoding, rotation and recovery.
8. Upgrade governance and emergency response policy.

These parameters require economic, security and threat-model review before a public testnet with meaningful incentives or any mainnet launch.
