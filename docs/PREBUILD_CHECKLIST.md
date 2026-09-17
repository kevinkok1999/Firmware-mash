# Pre-Build Checklist

Production firmware work must not start until every mandatory item below is green or explicitly waived in an ADR.

## Foundation

- [ ] Exact upstream foundation repository selected.
- [ ] Exact upstream commit/tag pinned.
- [ ] T-Deck Plus target confirmed from source, not assumption.
- [ ] Baseline firmware can be reproduced locally/CI.
- [ ] Baseline host/unit tests pass.
- [ ] Baseline simulator tests pass.
- [ ] Baseline RAM, PSRAM and flash size recorded.
- [ ] Baseline LoRa-only behavior recorded.
- [ ] Baseline boots and reaches local UI with no microSD installed.

## Licensing

- [ ] Foundation license captured at pinned commit.
- [ ] Every reused upstream component has provenance recorded.
- [ ] GPL/custom-license sources are not silently copied into MIT code.
- [ ] Missing Firmware-mash integration/policy/provider code will be implemented originally or via compatible vendor/SDK APIs.
- [ ] Third-party notices plan exists.

## Architecture

- [ ] One routing authority only: HybridRouter.
- [ ] One device energy-policy authority only: EnergyManager.
- [ ] Transport adapters cannot mutate routing state directly.
- [ ] Energy providers cannot mutate routing/delivery state directly.
- [ ] Routing state has one writer/task owner.
- [ ] Logical packet identity survives transport changes.
- [ ] Route, neighbor, peer, ACK, event and packet pools are bounded.
- [ ] Every bounded structure has an overflow/eviction policy.
- [ ] LoRa-only build remains supported.
- [ ] Stable core has zero microSD dependency.
- [ ] Stable core has zero RF-harvesting-hardware dependency.

## Internal storage

- [ ] Identity/configuration remains on onboard internal flash.
- [ ] Durable pending/store-forward messages have a separate bounded internal MessageStore design.
- [ ] High-churn route/RF/energy metrics stay primarily in RAM/PSRAM.
- [ ] Message-store corruption cannot require wiping identity/configuration.
- [ ] Power-loss recovery strategy is specified for append/delete/compaction.
- [ ] Flash wear/compaction counters and full-store behavior are defined.
- [ ] Exact 16 MB partition layout is chosen only after baseline image size is measured.
- [ ] If OTA is retained, both app slots and rollback/growth margin fit alongside durable storage.

## Routing

- [ ] Baseline AODV behavior documented.
- [ ] Multipath route-set memory cost estimated.
- [ ] Loop prevention strategy specified.
- [ ] Route ageing and invalidation specified.
- [ ] Alternate-path diversity metric specified.
- [ ] Failover occurs before broad rediscovery where possible.
- [ ] Flooding has hop/duplicate/time bounds.
- [ ] Energy cost is a bounded score input, never a second routing engine.

## Reliability and security

- [ ] End-to-end ACK is distinct from link-layer success.
- [ ] Retry counts/backoff are bounded.
- [ ] Packet deduplication strategy specified.
- [ ] Replay protection remains intact.
- [ ] Energy-driven deferral cannot create false Delivered state.
- [ ] No custom cryptographic primitive introduced.
- [ ] Relays do not require plaintext message access.

## RF, power and regulatory

- [ ] EU868 configuration checked against current regional requirements.
- [ ] All LoRa TX routes through one airtime gate.
- [ ] Critical/multipath mode cannot bypass airtime limits.
- [ ] ESP-NOW/Wi-Fi/BLE coexistence assumptions are explicitly tested, not guessed.
- [ ] Optional 2.4 GHz discovery has a bounded/power-aware policy.
- [ ] EnergyManager states/telemetry sources are defined with hysteresis and unknown-value handling.
- [ ] RF-harvesting provider is architecturally separate from communications transports.
- [ ] Stable build remains complete with RF-harvesting provider OFF.
- [ ] Separate harvesting antenna/rectenna is the default future hardware assumption.
- [ ] Shared-antenna harvesting cannot be promoted without insertion-loss/desense/isolation evidence.
- [ ] RF-assist/backscatter features are disabled unless matching hardware exists.

## Test gates

- [ ] Basic multi-hop scenario defined.
- [ ] Primary-route failure scenario defined.
- [ ] Duplicate-arrival scenario defined.
- [ ] High-loss scenario defined.
- [ ] Congestion scenario defined.
- [ ] Queue/pool exhaustion scenarios defined.
- [ ] Store-and-forward scenario defined.
- [ ] No-SD cold-boot/reboot/queue-recovery scenario defined.
- [ ] Interrupted-store-write/power-cut recovery scenario defined.
- [ ] Hybrid ESP-NOW/LoRa scenario defined before enabling that feature.
- [ ] Energy-state hysteresis/noisy-telemetry scenario defined.
- [ ] Critical/survival bounded-policy scenario defined.
- [ ] RF-harvester absent/no-regression scenario defined.
- [ ] Mock harvester/TX-reserve scenario defined before real hardware integration.

## Release safety

- [ ] Stable/Beta/Lab feature matrix exists.
- [ ] Recovery flashing path defined before public field testing.
- [ ] No OTA until signing, integrity and rollback design are reviewed.
- [ ] A compile-only result is never labelled hardware validated.
- [ ] STABLE promotion requires the no-microSD acceptance gate.
- [ ] STABLE promotion does not require an RF-harvesting accessory.
- [ ] Ambient RF Energy Assist remains LAB until real measured hardware evidence exists.