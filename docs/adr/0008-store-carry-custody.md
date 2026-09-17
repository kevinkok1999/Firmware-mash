# ADR 0008 — Durable store-carry-forward custody

Status: Accepted

## Context

Sender-side delayed delivery is not sufficient for the intended opportunistic mobile-network behavior. A sender that powers off before delivery cannot continue carrying the packet unless another participating node has durably accepted it.

## Decision

Firmware-mash may support relay custody as a separate BETA capability layered on top of ReliabilityManager and MessageStore.

A relay may accept custody only after durable local commit of the protected logical packet and bounded ownership metadata. Custody acceptance never equals end-to-end delivery. The same PacketId, destination identity, end-to-end protected envelope and user conversation survive the transfer.

Initial custody ownership is single-active-holder oriented and bounded. Uncontrolled epidemic replication is explicitly out of scope.

Custody state is owned through the normal networking/reliability architecture; no relay transport gets its own delivery engine.

## Consequences

- MessageStore gains optional relay-custody metadata.
- ReliabilityManager gains bounded transfer/reconciliation semantics.
- New custody tests are required before promotion.
- Sender-side WAITING_ROUTE remains available when custody is disabled.
- Real-world promotion requires T-Deck hardware evidence.

## Evidence / constraints

- Stable PacketId and exactly-once application presentation remain mandatory.
- No custom cryptography.
- Relay storage/energy pressure must cause deterministic rejection rather than false acceptance.
- TTL, dedup, resource bounds and no-SD durability remain mandatory.

## Supersedes / superseded by

None.