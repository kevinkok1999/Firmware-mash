# Firmware-mash

Firmware-mash is a research-led, modular off-grid messaging firmware project targeting the LILYGO T-Deck Plus (ESP32-S3 + SX1262) first.

## Mission

Build one coherent communication stack in which a single `HybridRouter` can choose between available links, retain alternate paths, fail over automatically, and queue encrypted messages when no route exists. LoRa remains the mandatory long-range backbone; ESP-NOW is planned as an optional local fast lane. NAN, RF-assist/RIS and external backscatter remain experimental until proven useful on real hardware.

## Project state

**PRE-BUILD / ARCHITECTURE FREEZE.** No production firmware has been added yet. The `main` branch is the clean release line. Preparatory architecture, licensing, testing and research work lives on `develop`.

## Hard rules

- Exactly one logical routing authority: `HybridRouter`.
- LoRa-only operation must always remain possible.
- Core boot, identity, send/receive, queued delivery and recovery must work without a microSD card.
- No unbounded flooding, queues, routing tables or retries.
- No custom cryptography.
- Experimental features stay behind build flags and cannot silently become stable.
- A compile result is not hardware validation.
- Upstream code is reused only when licensing is compatible and notices are preserved.
- EU868 duty-cycle and regional limits are treated as design constraints.
- Generated build output and local credentials never belong in Git; dependency lockfiles used for reproducible application builds do.

## Planned build order

1. Reproduce and pin the selected T-Deck Plus foundation.
2. Prove standalone internal persistence/recovery with no microSD dependency.
3. Introduce a LoRa-only HybridRouter skeleton with no behavior regression.
4. Add bounded multipath route sets and deterministic failover.
5. Harden reliability, deduplication and store-and-forward behavior.
6. Add ESP-NOW as an optional `TransportAdapter` and validate hybrid forwarding.
7. Run simulator, host and real multi-node/battery/power-loss acceptance tests.
8. Only then evaluate NAN, RF-assist, TDMA/regional routing and external backscatter.

## Documentation

Start at [`docs/README.md`](docs/README.md). It indexes the architecture, API contracts, routing specification, no-SD requirements, resource budget, foundation runbooks, risk reviews and ADRs.

The intended future code layout is defined in [`docs/REPOSITORY_LAYOUT.md`](docs/REPOSITORY_LAYOUT.md). Production source remains locked by CI until the baseline evidence gate passes.
