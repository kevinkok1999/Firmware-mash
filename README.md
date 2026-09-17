# Firmware-mash

Firmware-mash is a foundation-first communication firmware project for the **LILYGO T-Deck Plus**. The goal is one user-facing communicator that can keep the same conversation and message identity while the system automatically uses the best available approved path: LoRa first/stable, then optional ESP-NOW/LR and IP/gateway assistance as those layers are proven.

## Current status

**Phase 1 implementation has started on `develop`.**

The pinned foundation is `justinlindh/bramble` at commit `23854fd883fd29da14d8c0876f6eca4e14fbb938`, recorded in `docs/BASELINE_APPROVED`. New Firmware-mash production code is built as a controlled overlay on that exact foundation rather than as an unrelated parallel stack.

The project is intentionally built like a house:

1. **Phase 1 — Foundation and durable core**: pinned board/toolchain, internal MessageStore durability, reboot/power-loss recovery, host/fault tests.
2. **Phase 2 — Communication engine**: PacketId/events, EnergyManager, LoRa/AirtimeManager, HybridRouter, reliability, custody, ESP-NOW and IP/gateway federation.
3. **Phase 3 — Product and flasher**: smartphone-like UI, configuration, recovery/OTA, exact T-Deck build, release manifests and supported one-action flashing.

See `docs/THREE_PHASE_IMPLEMENTATION_PLAN.md` for the mandatory gates.

## Core invariants

- one logical `HybridRouter`;
- one logical message/PacketId across retry, failover and transport changes;
- one continuous conversation per contact/group, independent of route or transport;
- destination end-to-end ACK is the only source of user-visible `Delivered` truth;
- custody/store-carry-forward never pretends to be final delivery;
- all LoRa airtime passes through the approved AirtimeManager path;
- one `EnergyManager` owns device energy policy;
- all queues, retries and durable stores are bounded;
- internal flash is the core durability path; microSD is optional only;
- no custom cryptographic primitives;
- optional transports cannot become dependencies of the stable LoRa-only core.

## Phase 1 storage work

The pinned Bramble foundation already has internal SPIFFS message persistence. Source review found two bounded durability risks for Firmware-mash requirements: in-place rollover/compaction and in-place record/status updates. A power cut during either can damage committed state.

Firmware-mash therefore adds:

- an **append-only CRC journal** for normal durable mutations;
- **dual-generation transactional snapshots** for compaction/recovery;
- host fault tests before hardware promotion;
- explicit hardware power-cut/no-SD evidence before STABLE release.

See `docs/PHASE1_STORAGE_DURABILITY_DESIGN.md`.

## Build gates

The intended local/CI entry points are:

```bash
bash scripts/phase-gate.sh 1
bash scripts/phase-gate.sh 2
bash scripts/phase-gate.sh 3
```

Phase 2 and Phase 3 intentionally remain red until their actual source/tests exist. The release workflow also refuses to generate a user-facing package until Phase 3 passes.

At the time of the latest project check, recent GitHub Actions jobs were being created but remained queued before step 1. That is treated as an external runner/scheduling limitation, not as a compiler/test result. Re-check Actions status before relying on this note.

## User experience target

The end user should simply open a contact and send a message. The firmware may internally choose or change between LoRa, participating relays, ESP-NOW/LR and approved IP gateways, but those changes never create a new chat or duplicate user message.

The final supported release path is intended to produce exact T-Deck Plus firmware binaries, hashes, a flasher manifest and recovery instructions from one verified build. Flashability and STABLE status are not claimed until the target build and real-device evidence gates pass.
