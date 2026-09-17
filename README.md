# Firmware-mash

Firmware-mash is a modular communication OS/firmware project targeting the LILYGO T-Deck Plus first. The product goal is simple for the user and sophisticated underneath: one contact, one chat, one Send action; the firmware chooses the best currently valid path and keeps the message queued when no path exists.

## Mission

Build one coherent stack with:

- LoRa/SX1262 as mandatory off-grid backbone;
- optional ESP-NOW Normal/LR local lane;
- optional Wi-Fi/IP backhaul and gateway federation;
- future selected cellular PPP/modem provider;
- durable sender delayed delivery;
- optional BETA store-carry-forward custody through participating relays;
- one EnergyManager for battery/external-power policy;
- optional LAB Ambient RF Energy Assist hardware provider;
- smartphone-like local Messages/Contacts/Network/Settings UI;
- one reproducible release/flasher package after evidence gates pass.

## Project state

**ARCHITECTURE READY FOR CODING; PRODUCTION CODING STILL BLOCKED BY BASELINE EVIDENCE.**

The final controller pass reports zero unresolved architecture contradictions. Production source remains intentionally locked until `docs/BASELINE_APPROVED` exists with genuine baseline evidence. `main` remains the clean release line; preparation lives on `develop`.

## Core architecture

```text
User chat / Message service
          |
          v
  ReliabilityManager
     |          |
     |     optional Custody
     v          |
    HybridRouter <----- EnergyPolicySnapshot
     |
     +-- LoRa (required)
     +-- ESP-NOW Normal/LR (optional)
     +-- IP transport (optional)
           +-- Wi-Fi NetifProvider
           +-- selected Cellular NetifProvider
           +-- GatewayManager / Federation
```

One logical PacketId and one conversation survive retries, failover, gateway traversal, reboot and optional custody transfer.

## Hard rules

- Exactly one routing authority: `HybridRouter`.
- Exactly one device power-policy authority: `EnergyManager`.
- Destination E2E ACK is the only user-message `Delivered` truth.
- LoRa link success, ESP-NOW callback, TCP/TLS write, gateway acceptance and custody acceptance are **not** Delivered.
- Core boot/messaging/recovery works without microSD.
- Stable core works without Internet, cellular modem, custody or RF-harvest accessory.
- Sender-side pending messages survive reboot and retry automatically when credible connectivity returns.
- No fixed-distance retry trigger.
- Custody acceptance, when enabled, occurs only after relay durable commit.
- Custody replication/ownership is bounded; no epidemic flooding.
- Wi-Fi/cellular are IP bearers, not separate chats/routing stacks.
- No arbitrary third-party router is treated as an unconfigured Firmware-mash relay.
- No single mandatory cloud server owns chats/history.
- Every LoRa TX passes AirtimeManager.
- All queues/tables/retries/sessions/custody records are bounded.
- No custom cryptographic primitives.
- No compile/simulator result is called hardware validation.
- Missing approved project code is implemented under `mog_` components rather than silently dropped.
- Exact flash offsets/resource capacities are measured/generated, never guessed.

## Authoritative implementation order

After valid baseline approval:

1. pinned foundation integration;
2. internal MessageStore/no-SD durability;
3. packet/event/core primitives;
4. EnergyManager;
5. LoRa + AirtimeManager;
6. HybridRouter LoRa-only seam;
7. reliability/dedup/sender delayed delivery;
8. multipath/route scoring;
9. optional custody core on the proven LoRa/storage foundation;
10. ESP-NOW Normal/LR;
11. IP backhaul/gateway federation;
12. cross-transport queued/custody recovery;
13. health/metrics/controller review;
14. smartphone-like UI;
15. optional RF-harvest provider seam;
16. optional lab features;
17. release/flasher package.

See `docs/CODING_TRIGGER_CONTRACT.md`, `docs/ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` and `docs/IMPLEMENTATION_BLUEPRINT.md`.

## One-click coding readiness

A manual GitHub workflow exists at `.github/workflows/coding-readiness.yml`.

It is deliberately fail-closed. It checks:

- `develop` branch;
- valid `docs/BASELINE_APPROVED`;
- required architecture contracts;
- final controller status with zero architecture contradictions.

When green, a coding agent follows `docs/CODING_TRIGGER_CONTRACT.md`. The workflow validates readiness; it does not pretend GitHub Actions can autonomously write the whole firmware without a coding agent.

## One-click release/flasher target

`.github/workflows/release-package.yml` is the future manual package trigger. It deliberately fails today because production source, `scripts/release-package.sh` and release evidence do not exist yet.

After implementation it must generate from real build metadata:

```text
firmware binary / supported flash set
release-manifest.json
SHA256SUMS
flasher-manifest.json
README-RECOVERY.txt
```

Normal users should never concatenate binaries or calculate offsets manually. See `docs/FLASHER_RELEASE_CONTRACT.md`.

## Evidence tiers

- **STABLE:** no-SD standalone LoRa core, durable store, proven reliability/multipath, local UI, EnergyManager and tested release/recovery path.
- **BETA:** individually proven ESP-NOW, IP/gateway and/or custody capabilities.
- **LAB:** RF-harvest hardware, unproven cellular targets, NAN/NAT/RF-assist/backscatter experiments.

## Current known blockers

These are evidence/infrastructure blockers, not unresolved architecture:

- `docs/BASELINE_APPROVED` is not yet present;
- project GitHub Actions jobs are currently observed queued before any step executes;
- exact resource/partition values still require measured baseline build evidence;
- hardware-facing feature promotion requires real T-Deck/modem/harvester tests.

## Final controller

Read `docs/FINAL_CONTROLLER_REVIEW_2026-09-17.md` for the three-phase final audit and current go/no-go status.

## Documentation

Start at `docs/README.md`. Production source remains locked by preflight until the baseline gate genuinely passes.
