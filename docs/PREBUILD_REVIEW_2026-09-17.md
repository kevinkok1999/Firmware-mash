# Pre-Build Red-Team Review — 2026-09-17

## Verdict

**CONDITIONAL GO for Build Phase 1 only.** The repository is ready to begin foundation reproduction/audit, but not ready to add production routing code until the baseline gate passes.

## Review findings and fixes already applied

### 1. Risk: coding could start before the foundation is reproducible

**Fix:** preflight CI blocks non-empty `components/`, `main/`, `firmware/` and `src/` until a machine-validated `docs/BASELINE_APPROVED` marker exists. A template and Build Phase 1 runbook define the evidence needed.

### 2. Risk: "combine all projects" could cause license contamination

**Fix:** project is MIT-first; Bramble/MeshCore/LoRaMesher/MeshCom are currently MIT candidates for selective reuse with notices. Meshtastic and Pyxis are GPL-3.0 references; Reticulum requires custom-license review. Provenance is mandatory before merging reused source.

### 3. Risk: multiple routing engines create loops and contradictory state

**Fix:** ADR 0001 makes HybridRouter the only logical routing authority. Transport adapters cannot own routing state.

### 4. Risk: multipath creates duplicate application messages

**Fix:** ADR 0004 establishes one logical PacketId across transports/routes. Dedup and end-to-end ACK correlation sit above physical links.

### 5. Risk: a route error for one path can incorrectly kill another path

**Evidence:** recent Bramble RERR fix exposed exactly this failure mode.

**Fix:** route-error handling must verify that the reported failure intersects the selected/pending path before invalidating it. Add simulator coverage before multipath promotion.

### 6. Risk: lost response parks a state machine forever

**Evidence:** recent Bramble DM handshake recovery fix.

**Fix:** handshake, route discovery, pending ACK, bridge and store-forward states require explicit ageing/recovery. No unbounded WAITING/HANDSHAKING state.

### 7. Risk: 32-bit timer wrap creates rare field failures

**Evidence:** current Pyxis and Meshtastic work both show timer/sentinel edge cases can survive normal testing.

**Fix:** use one monotonic-time abstraction, wrap-safe elapsed/deadline helpers, and separate state flags/enums from timestamp values. Add wrap-boundary host tests.

### 8. Risk: route scoring optimizes signal strength while congesting the channel

**Evidence:** current Meshtastic work uses measured channel congestion for hop/politeness behavior.

**Fix:** congestion/airtime becomes a first-class route metric and broad discovery is a late fallback.

### 9. Risk: feature-rich C++ diagnostics consume disproportionate flash

**Evidence:** recent LoRaMesher change reports a large flash reduction by avoiding iostream-heavy diagnostic string building on ESP32-S3.

**Fix:** embedded hot paths use bounded/lightweight formatting and every implementation PR reports flash/RAM deltas.

### 10. Risk: ESP-NOW proof is overstated

**Evidence:** MeshCore PR #3410 is currently open, compile-verified and explicitly not hardware validated.

**Fix:** ESP-NOW remains BETA/LAB until direct T-Deck and hybrid forwarding hardware tests pass. Link/bridge secrets do not replace end-to-end security.

### 11. Risk: normal CI never compiles experimental adapters

**Evidence:** MeshCore PR #3410 notes bridge code was absent from its normal build matrix.

**Fix:** when an adapter is added, CI must include at least one build with that adapter ON and one LoRa-only build with it OFF.

### 12. Risk: scale goals become claims without evidence

**Fix:** 50/100/500/1000+ nodes are simulator research tiers, not promised deployment capacity. Stable documentation may only claim measured scenarios and conditions.

## Remaining blockers before production code

- Select and pin foundation commit.
- Reproduce T-Deck Plus build.
- Run and record available tests/simulator.
- Record baseline RAM/flash.
- Confirm baseline licensing/provenance.
- Create valid `docs/BASELINE_APPROVED` only after all required evidence is PASS.

## Design quality conclusion

The current preparation intentionally optimizes for reversibility: LoRa-only remains the permanent fallback, each future radio feature is independently removable, routing state has one owner, and the repository refuses premature source import. That makes Build Phase 1 the next justified action rather than adding more speculative modules.
