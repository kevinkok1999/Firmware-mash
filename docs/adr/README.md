# Architecture Decision Records

ADRs capture decisions that shape multiple modules or are expensive to reverse.

## Current decisions

- `0001-single-routing-authority.md` — HybridRouter is the sole logical routing authority.
- `0002-foundation-selection-gate.md` — no production import before a reproducible baseline.
- `0003-feature-stability-and-flags.md` — STABLE/BETA/LAB isolation and LoRa-only regression target.
- `0004-logical-packet-identity.md` — one logical PacketId across transports and retries.
- `0005-standalone-internal-storage.md` — core operation must not depend on microSD.
- `0006-energy-manager-and-rf-harvest-isolation.md` — one EnergyManager owns device energy policy; RF harvesting is an optional external-hardware provider, never a routing transport.
- `0007-ip-backhaul-and-gateway-federation.md` — Wi-Fi/cellular are bearer providers of one IP transport; GatewayManager supplies reachability evidence while HybridRouter remains the sole router.

## When to create an ADR

Create or supersede an ADR when changing:

- foundation/fork strategy;
- routing ownership or protocol model;
- IP/gateway/federation ownership model;
- device energy-policy ownership or harvesting/provider boundary;
- packet/wire compatibility;
- persistence model or partition strategy;
- security boundary or cryptographic construction;
- release-channel semantics;
- a new top-level source architecture;
- a decision that would require coordinated changes across multiple components.

Small implementation details do not need ADRs.

## ADR format

Use the next sequential number and include:

```text
# ADR NNNN — Title

Status: Proposed / Accepted / Superseded

## Context
## Decision
## Consequences
## Evidence / constraints
## Supersedes / superseded by (when applicable)
```

Accepted ADRs are normative. If implementation discovers that an accepted decision is wrong, update the architecture through a new ADR rather than silently violating the old one.
