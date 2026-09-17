# ADR 0001 — Single Routing Authority

**Status:** Accepted for pre-build architecture

## Context

The project may eventually expose LoRa, ESP-NOW, NAN and future external RF links. Allowing each technology to run an independent user-facing routing engine would create conflicting path state, duplicate reliability logic, bridge loops and hard-to-test failure behavior.

## Decision

`HybridRouter` is the only logical routing authority. Physical technologies are `TransportAdapter`s that report reachability/metrics and transmit frames. RF-assist mechanisms do not become logical hops unless they actually transport protocol packets.

## Consequences

- One route table and failover policy.
- One logical PacketId across transport changes.
- Reliability/dedup can be transport independent.
- New links can be added without changing the user messaging model.
- Adapters must not mutate routing state directly.
