# Upstream Research Snapshot — 2026-09-17

This is a research snapshot, not a permanent dependency lock. Revalidate all upstream heads/licences at the start of Build Phase 1, then pin the exact foundation commit in a new baseline record.

## Observed upstream heads / notable refs

| Project | Observed ref | Why it matters to Firmware-mash |
|---|---|---|
| Bramble | `23854fd883fd29da14d8c0876f6eca4e14fbb938` | Leading foundation candidate. Recent fix demonstrates that RERR/fail-fast must be path-relevant and that stalled DM/session state needs an explicit recovery/ageing rule. |
| MeshCore main | `0679dbeffc504d562d2f09eb072fdc223f8ffc2a` | Stable/current main reference observed during audit; v1.17.1 appears at `d9296435...`. |
| MeshCore ESP-NOW companion PR #3410 | head `e81681829d0184c6e69da39dbda070f72be199a5`, base `dev` `34404309...` | Still OPEN and not hardware validated. Demonstrates a useful local-lane/bridge architecture, but must remain EXPERIMENTAL reference until real hardware proof. Upstream notes its XOR bridge secret is network isolation, not security. |
| LoRaMesher | `42fd29190748cc5f90fd3e3ae10408b6e3a14408` | Recent ESP32-S3-oriented optimization removed iostream usage and reportedly saved ~230 KB flash in a representative build. Strong signal to keep diagnostics allocation/lightweight-library costs out of embedded hot paths. |
| Meshtastic firmware | `f36a1ea821392426a03c35d2f142296df96d7b59` | Very active reference source. Recent work includes congestion-aware hop scaling, router/rebroadcast fixes and store-forward identity handling. GPL-3.0: architecture/reference only for an MIT core unless licensing strategy changes. |
| Reticulum | `1565126ffd08b9d7bc750ce5df82d5aa3e38183e` | Multi-interface/path-discovery architecture reference. Custom Reticulum License requires separate review before any source reuse. |
| Pyxis | `7ae47d5a381afc48c7de0c92aca97564bba95ab4` | GPL-3.0 T-Deck/multi-interface reference. Recent work also reinforces bounded-stall and wrap-safe timer design. |
| MeshCom-Firmware | `4058b25bc6db42ee26b42aa15be2e0d33d85a005` | MIT T-Deck/LoRa reference; inspect board/OTA/gateway components selectively. |

## Architecture lessons extracted now

### 1. Route errors must be path-scoped

A failure report for destination D must not automatically cancel every pending delivery to D. The reported failed hop/link must intersect the actual path or next-hop state being invalidated. This becomes a required simulator case.

### 2. Every long-lived state machine needs stale-state recovery

Handshake, route-discovery, pending-ACK and bridge states must have explicit ageing/recovery. No state is allowed to remain parked forever after one lost response.

### 3. Congestion must influence routing behavior

LoRa is airtime constrained. Route score/failover/discovery policy must include measured congestion/airtime, not only hop count or RSSI. Broad discovery should be a late fallback.

### 4. Embedded library choices matter

Avoid heavyweight formatting/stream facilities in firmware hot paths when small bounded alternatives work. Flash/RAM deltas become PR metrics once code starts.

### 5. Time arithmetic must be wrap-safe

Use a monotonic-time abstraction and explicit elapsed/deadline helpers. Do not overload `0` or other timestamp values as state sentinels when a separate boolean/state enum is safer. Add wrap-boundary host tests for timers used by routing, retries and store-forward.

### 6. Bridge/link security is not message security

ESP-NOW peer/link protection or a bridge shared secret does not replace end-to-end message security. Hybrid transports stay below the E2E security boundary.

### 7. CI must compile experimental seams explicitly

A feature hidden behind build flags is not covered merely because normal targets compile. When ESP-NOW/hybrid code lands, CI needs at least one configuration that actually compiles that adapter and its integration points, plus a LoRa-only configuration proving isolation.

## Current research status

- Bramble foundation: **PROMISING / requires reproducible baseline gate**.
- LoRa-only HybridRouter seam: **DESIGN READY, not implemented**.
- Multipath failover: **DESIGN READY for simulator-first implementation**.
- ESP-NOW local lane: **PROMISING / upstream inspiration compile-verified but not hardware-proven in the referenced MeshCore PR**.
- NAN: **EXPERIMENTAL**.
- RIS/passive RF assist: **EXPERIMENTAL external RF research**.
- Ambient/backscatter: **FUTURE EXTERNAL HARDWARE**, not a firmware-only T-Deck feature.
