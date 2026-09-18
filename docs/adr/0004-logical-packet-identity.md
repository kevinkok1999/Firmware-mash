# ADR 0004 — One Logical Packet Identity Across Transports

**Status:** Accepted for pre-build architecture

## Context

Hybrid forwarding means the same user message may cross ESP-NOW and LoRa, or be retried on another path. If each transport creates a new application identity, end-to-end ACK correlation, deduplication, replay handling and diagnostics become unreliable.

## Decision

A logical packet receives one stable `PacketId` before routing. That identity survives transport changes, retries and alternate-path delivery. Transport adapters may add local framing/sequence information, but they do not replace the logical PacketId.

## Consequences

- The destination can collapse duplicate arrivals into one application delivery.
- Reliability can correlate an end-to-end ACK independent of the physical path.
- Trace/metrics can reconstruct failover behavior.
- Transport-level sequence numbers remain local and disposable.
