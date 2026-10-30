# ADR-0009: Consensus Architecture Evaluation Gate

Status: evaluation framework accepted; consensus mechanism NOT selected.
Date: 2026-10-30

## Context

HJZ Native Layer-1 currently has deterministic transaction execution and block structure, but it has no consensus protocol, fork-choice rule, validator/miner lifecycle, or finality mechanism. A block's parent hash and timestamp checks do not establish that the block is authorized by a decentralized network.

Consensus selection is a protocol-level decision. It affects Sybil resistance, network participation, finality, hardware requirements, censorship resistance, recovery from partitions, and monetary policy. It must not be inferred from the legacy ERC-20 supply or chosen merely for implementation convenience.

## Candidates to evaluate

### Proof of Work (PoW)
- Security derives from externally costly hash work and a fully specified chain-selection rule.
- Advantages: open participation and no validator stake registry.
- Costs: energy and hardware competition, probabilistic settlement, mining-pool concentration, and difficulty-adjustment attack surface.
- Required research: ASIC/CPU/GPU assumptions, difficulty adjustment, retarget manipulation, selfish mining, reorg limits, and sustainable security budget.

### Proof of Stake (PoS)
- Security derives from economically bonded validators and protocol-defined penalties/rewards.
- Advantages: lower energy use and potential for faster settlement.
- Costs: stake concentration, validator-set bootstrapping, long-range attacks, weak-subjectivity assumptions, key compromise, and complex slashing/accounting.
- Required research: validator entry/exit, unbonding, slashing, checkpointing, weak-subjectivity checkpoints, and stake distribution assumptions.

### BFT-style Proof of Stake
- A stake-weighted validator set runs a Byzantine fault-tolerant agreement protocol, potentially providing deterministic finality under stated fault assumptions.
- Advantages: fast finality and explicit safety/liveness guarantees when the validator set and network assumptions hold.
- Costs: validator-set governance, communication complexity, view-change and partition behavior, denial-of-service exposure, and a more complex bootstrap/trust model.
- Required research: fault threshold, validator rotation, quorum certificates, accountable safety, liveness recovery, and network synchrony assumptions.

## Mandatory protocol properties before selection

1. A formal threat model, including Sybil, eclipse, partition, censorship, equivocation, long-range, and denial-of-service attacks.
2. Explicit safety and liveness assumptions, including the network synchrony model.
3. A deterministic fork-choice/finality rule and reorganization policy.
4. Genesis validator/miner bootstrapping that does not depend on an undisclosed trusted party.
5. Measurable decentralization and hardware participation goals.
6. A complete fee, issuance, reward, and penalty model with overflow-safe integer accounting.
7. Economic attack analysis and simulation under adversarial participation.
8. Test vectors and multi-node adversarial simulations.

## Decision

No consensus mechanism or parameters are selected in this ADR. No block production, mining, validator rewards, token issuance, total supply, validator eligibility, or finality claims may be implemented as if already approved.

A separate ADR must record the selected mechanism, formal assumptions, parameters, threat analysis, and migration implications before consensus code is introduced.

## Current implementation boundary

The existing block validator checks structural properties only. It is not a consensus engine and must not accept untrusted network blocks as canonical solely because structural validation succeeds.
