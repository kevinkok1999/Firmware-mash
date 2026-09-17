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
- [ ] Baseline boots and reaches local UI with no microSD installed before STABLE promotion.

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
- [ ] GatewayManager/GatewayDiscovery cannot own a second route table.
- [ ] Custody policy cannot become a second delivery/routing engine.
- [ ] Routing state has one writer/task owner.
- [ ] Logical PacketId survives transport, gateway and custody changes.
- [ ] One contact remains one conversation across all paths.
- [ ] Route, neighbor, peer, gateway, custody, ACK, event and packet pools are bounded.
- [ ] Every bounded structure has an overflow/eviction policy.
- [ ] LoRa-only build remains supported.
- [ ] Stable core has zero microSD dependency.
- [ ] Stable core has zero Internet/gateway dependency.
- [ ] Stable core has zero RF-harvesting-hardware dependency.

## Internal storage

- [ ] Identity/configuration remains on onboard internal flash.
- [ ] Durable sender pending/store-forward messages have a separate bounded internal MessageStore design.
- [ ] Optional accepted relay custody uses the same durable boundary and cannot be accepted before commit.
- [ ] High-churn route/RF/energy/gateway metrics stay primarily in RAM/PSRAM.
- [ ] Message-store corruption cannot require wiping identity/configuration.
- [ ] Power-loss recovery strategy is specified for append/delete/compaction/custody update.
- [ ] Flash wear/compaction counters and full-store behavior are defined.
- [ ] Exact 16 MB partition layout is chosen only after baseline image size is measured.
- [ ] If OTA is retained, both app slots and rollback/growth margin fit alongside durable storage.

## Routing and reliability

- [ ] Baseline routing behavior documented.
- [ ] Multipath route-set memory cost estimated.
- [ ] Loop prevention strategy specified.
- [ ] Route ageing and invalidation specified.
- [ ] Alternate-path diversity metric specified.
- [ ] Failover occurs before broad rediscovery where possible.
- [ ] Flooding has hop/duplicate/time bounds.
- [ ] Energy cost is a bounded score input, never a second routing engine.
- [ ] End-to-end ACK is distinct from link, socket, gateway and custody success.
- [ ] Retry counts/backoff are bounded.
- [ ] Packet deduplication strategy specified.
- [ ] Replay protection remains intact.
- [ ] Energy-driven deferral cannot create false Delivered state.
- [ ] Custody acceptance cannot create false Delivered state.
- [ ] Relays/gateways do not require plaintext message access.
- [ ] No custom cryptographic primitive introduced.

## IP / gateway federation

- [ ] Wi-Fi/cellular are NetifProvider bearers below one `mog_transport_ip`.
- [ ] Normal third-party Wi-Fi routers are treated only as legitimate IP access, not unconfigured mesh relays.
- [ ] Gateway advertisements are authenticated/versioned/expiring/bounded.
- [ ] Multiple bootstrap/peer entrypoints are supported; no single mandatory cloud server owns conversations.
- [ ] Active authenticated peers can continue if a bootstrap endpoint disappears.
- [ ] Normal handheld inbound-listener policy is defined; public listeners are gateway-class roles.
- [ ] Gateway/session/advertisement exhaustion and malformed input fail closed.
- [ ] No plaintext message-body logging and no automatic precise GPS publication in discovery.
- [ ] Cellular claims remain disabled until selected modem/provider hardware evidence exists.

## Custody / store-carry-forward

- [ ] `STORE_CARRY_CUSTODY_CONTRACT.md` and ADR 0008 are normative.
- [ ] Relay durable commit precedes custody acceptance evidence.
- [ ] Initial ownership/replication policy is explicitly bounded.
- [ ] TTL/expiry/reconciliation semantics prevent immortal packets.
- [ ] Storage/energy pressure causes deterministic rejection rather than false acceptance.
- [ ] Custody can be disabled without breaking sender-side WAITING_ROUTE.

## RF, power and regulatory

- [ ] EU868 configuration checked against current regional requirements before release.
- [ ] All LoRa TX routes through one airtime gate.
- [ ] Critical/multipath/custody mode cannot bypass airtime limits.
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
- [ ] High-loss/congestion/exhaustion scenarios defined.
- [ ] Sender store-and-forward scenario defined.
- [ ] No-SD cold-boot/reboot/queue-recovery scenario defined.
- [ ] Interrupted-store-write/power-cut recovery scenario defined.
- [ ] Hybrid ESP-NOW/LoRa scenario defined before enabling that feature.
- [ ] Energy-state hysteresis/noisy-telemetry and critical/survival scenarios defined.
- [ ] RF-harvester absent/no-regression and mock TX-reserve scenarios defined.
- [ ] IP/gateway mixed-route, fallback, recovery, multi-peer and security-negative scenarios defined.
- [ ] Custody CUS-001..CUS-010 defined before custody implementation is promoted.
- [ ] One-flash/release RELSE tests defined before release automation is called complete.

## Release and coding trigger safety

- [ ] Stable/Beta/Lab feature matrix exists.
- [ ] `CODING_TRIGGER_CONTRACT.md` is the authoritative implementation entrypoint.
- [ ] Trigger refuses production source creation without valid `BASELINE_APPROVED`.
- [ ] Final controller report contains no unresolved architecture contradiction.
- [ ] `FLASHER_RELEASE_CONTRACT.md` defines generated manifest/hashes/flash descriptor.
- [ ] Exact flash offsets come from generated build/partition metadata, never guessed.
- [ ] Recovery flashing path defined and tied to exact release layout before STABLE.
- [ ] No OTA until signing, integrity and rollback design are reviewed.
- [ ] A compile-only result is never labelled hardware validated.
- [ ] STABLE promotion requires no-microSD hardware acceptance.
- [ ] STABLE does not require Internet, cellular or RF-harvesting accessory.
- [ ] Ambient RF Energy Assist remains LAB until real measured hardware evidence exists.
- [ ] Optional IP/ESP-NOW/custody features are advertised only at achieved evidence tier.
