# ADR-0014: Proof-of-Work Adaptive Difficulty Design

Status: proposed design for evaluation; NOT an adopted consensus protocol
Date: 2026-10-31
Related: ADR-0009, ADR-0012, ADR-0013

## 1. Purpose and scope

The project direction is to evaluate a Bitcoin-like, permissionless Proof-of-Work (PoW) design in which the network adjusts the required work to keep block production near a protocol target as aggregate hash rate changes. This ADR defines the engineering properties and validation gates for that design.

This is not a claim that HJZ has selected or implemented PoW. It does not activate mining, authorize blocks, define issuance, or make the current structural block validator a consensus engine. The target block interval, retarget window, proof hash, maximum target, timestamp policy, reward, total supply, and launch parameters remain unselected.

## 2. Core distinction: difficulty is dynamic; consensus rules are not

An increase in hash rate should normally cause the target to become harder at the next permitted retarget boundary. A decrease in hash rate should make the target easier, subject to bounded adjustment. The objective is stable block cadence, not progressively stricter transaction rules.

The following rules must remain deterministic and stable for a given protocol version:
- transaction validity, signatures, nonce/replay checks, and fee arithmetic;
- block structure, transaction ordering, Merkle commitment, and state transition;
- proof-of-work hash interpretation and target comparison;
- fork-choice and cumulative-work accounting;
- timestamp validity and retarget calculation.

A protocol upgrade may change these rules only through an explicitly versioned and coordinated activation process. Difficulty must never be changed by an individual node's local preference or by a miner's claimed hash rate.

## 3. Candidate PoW validity model

For a future header version that explicitly commits to a canonical target (or an equally unambiguous target encoding), a candidate block is eligible for PoW consideration only if all ordinary block, transaction, parent-context, and state-transition checks pass and:

`pow_hash(domain || canonical_header_without_mutable_nonce_fields || nonce_data) <= target`

The exact hash preimage, nonce representation, header version, target encoding, and hash function must be fixed by a future consensus ADR. The existing SHA3-256 utility is not automatically approved as a PoW algorithm. A cryptographic hash suitable for transaction IDs is not necessarily a suitable choice for the intended mining hardware and security economics.

The target must be:
- positive and no greater than the immutable chain proof-of-work limit;
- decoded with overflow and non-canonical encodings rejected;
- identical for all validating nodes at a given height;
- derived only from the accepted ancestor chain and immutable chain parameters.

A miner-provided target or difficulty claim is never authoritative.

## 4. Retarget design requirements

The candidate design uses a fixed retarget window of W blocks and a target timespan equal to W multiplied by the target block interval. W and the interval are open parameters.

At a retarget boundary, calculate observed elapsed time from timestamps committed by the relevant ancestor chain, then clamp the elapsed time to a bounded interval before scaling the previous target. The candidate arithmetic is:

- `clamped = clamp(observed_timespan, target_timespan / 4, target_timespan * 4)`
- `candidate_target = previous_target * clamped / target_timespan`
- `next_target = min(candidate_target, pow_limit)`

The 1/4-to-4x clamp is a candidate safety envelope for evaluation, not an approved HJZ parameter. The implementation must use checked, deterministic integer arithmetic with sufficient intermediate precision; floating point is forbidden. Division rounding, minimum target, boundary heights, genesis bootstrap, and behavior when an ancestor timestamp is unavailable must be specified exactly before implementation.

Between retarget boundaries, the expected target is unchanged. Every node independently derives the expected target from the same parent history. Blocks with any other target are invalid.

## 5. Timestamp and manipulation resistance

A retarget algorithm is only as robust as its timestamp rules. Before implementation, the consensus specification must define:
- a median-time-past (MTP) rule and its exact ancestor window;
- strict monotonicity relative to the required parent/median time;
- a maximum future-drift rule and a deterministic adjusted-network-time policy;
- which ancestor timestamps feed retarget calculation;
- checked handling of timestamp subtraction, multiplication, and boundary overflow;
- behavior during long stalls, clock skew, partitions, and hash-rate shocks.

Do not use a single miner-controlled timestamp as an unchecked measurement of real elapsed time. Simulate timestamp grinding, time-warp attacks, boundary manipulation, withheld blocks, abrupt hash-rate changes, and low-hash-rate liveness before choosing the final estimator.

ADR-0013's persisted-parent timestamp validation remains mandatory. Its adjusted-network-time derivation is still open and must be reconciled with the eventual PoW timestamp policy.

## 6. Fork choice and chain work

PoW requires a deterministic fork-choice rule based on cumulative expected work, not block height alone and not a peer's assertion that a chain is heavier.

For a valid target T, per-block work is conventionally derived from the target using exact integer arithmetic equivalent to the expected number of hashes required to find a valid result. The final specification must define the precise formula, integer width/encoding, overflow behavior, and tie-break rule. Cumulative chain work must be stored and compared exactly; arithmetic overflow must invalidate the candidate rather than wrap.

A node must validate every block's proof, target schedule, ancestry, transactions, and resulting state before crediting its work. Invalid or merely structurally valid blocks contribute zero accepted work. Reorganization limits, confirmation guidance, checkpoint policy (if any), and deep-reorg recovery must be specified separately; a height-only maximum reorg rule must not silently override the chain-work rule.

## 7. Mining participation and decentralization goals

Difficulty adjustment cannot guarantee that miners remain profitable or that mining remains decentralized. It only regulates the protocol's required work relative to observed block production.

The economic and operational evaluation must include:
- CPU, GPU, and ASIC availability and the risk of specialized-hardware concentration;
- pool concentration, block withholding, selfish mining, and censorship incentives;
- hash-rate shocks, miner exit, low-hash-rate liveness, and difficulty lag;
- energy cost and geographic concentration;
- attack cost, rental/borrowed hash rate, and the security budget over time;
- fee-market behavior and reward dependence.

Do not add an automatic miner subsidy, fee split, penalty, or total-supply schedule in this ADR. Monetary policy requires a separate explicit decision and simulation. A difficulty retarget cannot repair an economically unsustainable reward model.

## 8. Required adversarial and property tests

Before a PoW consensus implementation can be considered:
1. Golden test vectors for every retarget boundary and non-boundary height.
2. Exact integer arithmetic tests, including maximum values, division rounding, and overflow rejection.
3. Target encoding canonicality, zero, negative/impossible, over-limit, and malformed compact encodings.
4. Proof hash below/equal/above target boundary tests.
5. Wrong-target blocks rejected even when their proof hash meets the miner-claimed target.
6. Timestamp MTP, future drift, time-warp, timestamp grinding, and clock-skew adversarial tests.
7. Hash-rate step-up/step-down and prolonged-stall simulations.
8. Competing forks with different heights and cumulative work; lower-height heavier-work chain selection.
9. Equal-work deterministic tie handling and reorg/recovery tests.
10. Multi-node partition, eclipse, selfish-mining, withholding, and peer-adversarial simulations.
11. Restart/replay tests proving target and chain-work reconstruction is identical.
12. Fuzzing of target decoding, header parsing, retarget arithmetic, and chain selection.

## 9. Implementation sequence and release gates

1. Complete a formal threat model and parameter/economic simulation.
2. Approve a separate consensus ADR selecting PoW and every consensus-critical parameter.
3. Specify a new header version and canonical target/work encoding; do not retrofit consensus meaning into Block Header V2 silently.
4. Implement pure deterministic difficulty and chain-work functions with golden vectors.
5. Add adversarial/property/fuzz tests and independent review.
6. Integrate target validation and cumulative-work fork choice into the durable atomic block-commit path.
7. Implement mining only after node validation and chain selection are correct.
8. Run a multi-node private testnet through hash-rate shocks, partitions, restarts, and reorgs.
9. Complete cryptographic, economic, operational, and independent security review before any incentivized public testnet or mainnet.

## 10. Decision and non-decisions

The project has a preferred direction to evaluate Bitcoin-like adaptive PoW, based on the requested mining behavior. This is a design preference, not final consensus adoption.

Not selected: target block interval, retarget window, proof hash, nonce/header encoding, pow limit, retarget clamp, timestamp estimator, chain-work encoding, tie-break rule, reorg policy, mining hardware strategy, reward, issuance, supply, fees, genesis, and activation height.

No mining engine, difficulty calculator, PoW validator, chain-work fork choice, reward logic, or mainnet consensus is implemented by this ADR. Existing structural validation remains insufficient to authorize network blocks.
