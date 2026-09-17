# Firmware-mash

Firmware-mash is a research-led, modular off-grid messaging firmware project targeting the LILYGO T-Deck Plus (ESP32-S3 + SX1262) first.

## Mission

Build one coherent communication stack in which a single `HybridRouter` can choose between available links, retain alternate paths, fail over automatically, and queue encrypted messages when no route exists. LoRa remains the mandatory long-range backbone; ESP-NOW Normal/Long Range is planned as an optional local/hybrid lane. NAN, RF-assist/RIS and external backscatter remain experimental until proven useful on real hardware.

The normal user experience must feel like a compact familiar smartphone: Home, Messages, Contacts, Network and Settings. The user sends a message; routing, retries, LoRa/ESP-NOW choice, multipath and delayed delivery happen automatically in the background.

## Project state

**PRE-BUILD / CODE-READY SUBJECT TO BASELINE EVIDENCE.** No production firmware has been added yet. The `main` branch is the clean release line. Preparatory architecture, licensing, testing, UX and implementation contracts live on `develop`.

## Hard rules

- Exactly one logical routing authority: `HybridRouter`.
- LoRa-only operation must always remain possible.
- Core boot, identity, send/receive, queued delivery and recovery must work without a microSD card.
- A queued message is retried automatically when credible usable connectivity returns; there is no fixed-distance retry trigger.
- One logical PacketId survives retries, transport changes, failover and reboot recovery.
- ESP-NOW Normal and Long Range are capabilities of one transport adapter, not separate user-selected networks.
- No unbounded flooding, queues, routing tables or retries.
- No custom cryptography.
- Experimental features stay behind build flags and cannot silently become stable.
- A compile result is not hardware validation.
- Upstream code is reused only when licensing is compatible and notices are preserved.
- EU868 duty-cycle and regional limits are treated as design constraints.
- Generated build output and local credentials never belong in Git; dependency lockfiles used for reproducible application builds do.
- Normal users are not the engineering test harness.

## Planned build order

1. Reproduce and pin the selected T-Deck Plus foundation.
2. Prove standalone internal persistence/recovery with no microSD dependency.
3. Add PacketId/event/core primitives while preserving the foundation security/wire envelope.
4. Introduce a LoRa-only HybridRouter skeleton with no behavior regression.
5. Add bounded reliability/dedup and durable delayed delivery.
6. Add bounded multipath route sets and deterministic failover.
7. Add ESP-NOW Normal + Long Range capability under one optional `TransportAdapter` and validate hybrid forwarding.
8. Integrate the smartphone-like local UI so Messages/Contacts/Network/Settings remain simple while routing stays automatic.
9. Run simulator, host and real multi-node/no-SD/power-loss/re-entry/UI/battery acceptance tests.
10. Produce a one-flash release package only for features that meet their evidence tier.
11. Only then evaluate NAN, RF-assist, TDMA/regional routing and external backscatter.

## One-shot implementation

Once `docs/BASELINE_APPROVED` contains genuine PASS evidence, implementation should follow [`docs/ONE_SHOT_IMPLEMENTATION_RUNBOOK.md`](docs/ONE_SHOT_IMPLEMENTATION_RUNBOOK.md). That runbook defines source-of-truth precedence, coding order, automatic problem-resolution behavior, controller passes and completion gates so implementation does not repeatedly reopen settled architecture decisions.

## Documentation

Start at [`docs/README.md`](docs/README.md). It indexes architecture, API/wire/delivery contracts, the no-SD storage design, smartphone UX, build configurations, test traceability, resource budgets, reviews and ADRs.

The intended future code layout is defined in [`docs/REPOSITORY_LAYOUT.md`](docs/REPOSITORY_LAYOUT.md). Production source remains locked by CI until the baseline evidence gate passes.
