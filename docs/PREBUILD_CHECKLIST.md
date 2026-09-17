# Pre-Build Checklist

Production firmware work must not start until every mandatory item below is green or explicitly waived in an ADR.

## Foundation

- [ ] Exact upstream foundation repository selected.
- [ ] Exact upstream commit/tag pinned.
- [ ] T-Deck Plus target confirmed from source, not assumption.
- [ ] Baseline firmware can be reproduced locally/CI.
- [ ] Baseline host/unit tests pass.
- [ ] Baseline simulator tests pass.
- [ ] Baseline RAM and flash size recorded.
- [ ] Baseline LoRa-only behavior recorded.

## Licensing

- [ ] Foundation license captured at pinned commit.
- [ ] Every reused upstream component has provenance recorded.
- [ ] GPL/custom-license sources are not silently copied into MIT code.
- [ ] Third-party notices plan exists.

## Architecture

- [ ] One routing authority only: HybridRouter.
- [ ] Transport adapters cannot mutate routing state directly.
- [ ] Routing state has one writer/task owner.
- [ ] Logical packet identity survives transport changes.
- [ ] Route, neighbor, peer, ACK, event and packet pools are bounded.
- [ ] Every bounded structure has an overflow/eviction policy.
- [ ] LoRa-only build remains supported.

## Routing

- [ ] Baseline AODV behavior documented.
- [ ] Multipath route-set memory cost estimated.
- [ ] Loop prevention strategy specified.
- [ ] Route ageing and invalidation specified.
- [ ] Alternate-path diversity metric specified.
- [ ] Failover occurs before broad rediscovery where possible.
- [ ] Flooding has hop/duplicate/time bounds.

## Reliability and security

- [ ] End-to-end ACK is distinct from link-layer success.
- [ ] Retry counts/backoff are bounded.
- [ ] Packet deduplication strategy specified.
- [ ] Replay protection remains intact.
- [ ] No custom cryptographic primitive introduced.
- [ ] Relays do not require plaintext message access.

## RF and regulatory

- [ ] EU868 configuration checked against current regional requirements.
- [ ] All LoRa TX routes through one airtime gate.
- [ ] Critical/multipath mode cannot bypass airtime limits.
- [ ] ESP-NOW/Wi-Fi/BLE coexistence assumptions are explicitly tested, not guessed.
- [ ] RF-assist/backscatter features are disabled unless matching hardware exists.

## Test gates

- [ ] Basic multi-hop scenario defined.
- [ ] Primary-route failure scenario defined.
- [ ] Duplicate-arrival scenario defined.
- [ ] High-loss scenario defined.
- [ ] Congestion scenario defined.
- [ ] Queue/pool exhaustion scenarios defined.
- [ ] Store-and-forward scenario defined.
- [ ] Hybrid ESP-NOW/LoRa scenario defined before enabling that feature.

## Release safety

- [ ] Stable/Beta/Lab feature matrix exists.
- [ ] Recovery flashing path defined before public field testing.
- [ ] No OTA until signing, integrity and rollback design are reviewed.
- [ ] A compile-only result is never labelled hardware validated.
