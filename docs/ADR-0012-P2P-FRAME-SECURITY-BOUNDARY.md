# ADR-0012: P2P Frame Security Boundary

Status: required security constraint; transport implementation pending.
Date: 2026-10-30

## Context

The current native P2P module encodes and decodes bounded frames with a fixed magic value, protocol version, message type, and payload length. The codec is a serialization primitive only. It is not a peer transport, authenticated session, or network security protocol.

A syntactically valid frame does not establish who sent it, whether it was modified in transit, whether it is fresh, or whether the sender is authorized to influence chain state.

## Decision

The current frame codec MUST be treated as untrusted-input parsing scaffolding. It MUST NOT be exposed directly as a production peer protocol or used as evidence of peer identity or block authorization.

Before production networking, the protocol and implementation must specify and test:

1. Authenticated peer identity with cryptographic keys independent of device identifiers.
2. An authenticated key-establishment and encrypted transport design using reviewed standards; ML-KEM may be evaluated for key establishment, but it is not a signature algorithm.
3. Integrity protection for every frame/session record and explicit replay/sequence handling.
4. A bounded handshake state machine, protocol/network identifiers, capability negotiation, and downgrade resistance.
5. Per-peer and global limits for connections, message sizes, queues, parsing time, and concurrent expensive cryptographic operations.
6. Rate limits, timeouts, backpressure, duplicate suppression, and resource accounting against DoS.
7. Peer scoring and eclipse-resistance strategy, without allowing unauthenticated peer claims to become consensus authority.
8. Fuzzing and adversarial tests for malformed, truncated, oversized, reordered, duplicated, stale, and resource-exhaustion traffic.
9. Explicit separation between transport authentication, transaction signature validation, block validity, and consensus authorization.

The frame encoder must reject unknown local message-type values as well as the decoder rejecting unknown wire values. Payload bounds must be enforced before allocation and dispatch.

## Current boundary

The current 14-byte HJZ1 frame header contains magic, protocol version, message type, and payload length. It has no checksum/MAC, peer authentication, encryption, replay protection, handshake state machine, or socket-level resource controls. The codec has no live socket or peer-manager implementation.

The latest encoder rejects unknown outbound message types and payloads larger than the configured 2 MiB maximum. These are parser/codec hardening measures only; they do not make the transport secure.

## Consequences

- No production network exposure is permitted based on the current codec.
- No inbound block may become canonical based only on transport identity or structural validation.
- Transport security, transaction authorization, block execution, and consensus must remain independent validation layers.
- A separate reviewed ADR is required before selecting the production peer transport and identity model.
