# Firmware-mash

Firmware-mash is a research-led, modular off-grid messaging firmware project targeting the LILYGO T-Deck Plus (ESP32-S3 + SX1262) first.

## Mission

Build one coherent communication stack in which a single `HybridRouter` can choose between available links, retain alternate paths, fail over automatically, and queue encrypted messages when no route exists. LoRa remains the mandatory long-range backbone; ESP-NOW is planned as an optional local fast lane. NAN, RF-assist/RIS and external backscatter remain experimental until proven useful on real hardware.

## Project state

**PRE-BUILD / ARCHITECTURE FREEZE.** No production firmware has been added yet. The `main` branch is the clean release line. Preparatory architecture, licensing, testing and research work lives on `develop`.

## Hard rules

- Exactly one logical routing authority: `HybridRouter`.
- LoRa-only operation must always remain possible.
- No unbounded flooding, queues, routing tables or retries.
- No custom cryptography.
- Experimental features stay behind build flags and cannot silently become stable.
- A compile result is not hardware validation.
- Upstream code is reused only when licensing is compatible and notices are preserved.
- EU868 duty-cycle and regional limits are treated as design constraints.

## Planned build order

1. Reproduce and pin the selected T-Deck Plus foundation.
2. Introduce a LoRa-only HybridRouter skeleton with no behavior regression.
3. Add bounded multipath route sets and deterministic failover.
4. Add ESP-NOW as an optional `TransportAdapter`.
5. Integrate hybrid forwarding and store-and-forward.
6. Run simulator, host and real multi-node acceptance tests.
7. Only then evaluate NAN, RF-assist, TDMA/regional routing and external backscatter.

See `docs/` for the architecture, upstream audit, license plan, risk register, roadmap and pre-build gates.
