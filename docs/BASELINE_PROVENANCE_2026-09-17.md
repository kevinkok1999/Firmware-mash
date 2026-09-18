# Baseline Provenance — Bramble pin 2026-09-17

## Upstream source

```text
project=bramble
repository=https://github.com/justinlindh/bramble
commit=23854fd883fd29da14d8c0876f6eca4e14fbb938
license=MIT
license_copyright=Copyright (c) 2026 Justin Lindh
esp_idf=v5.4.1
primary_target=LILYGO T-Deck Plus
```

## Reuse strategy

Firmware-mash uses a **foundation/fork adaptation strategy** rather than copying isolated source fragments. The goal is to preserve Bramble's board support, tests, simulator coherence, security envelope and driver integration while adding Firmware-mash-owned `mog_` policy/components around or in carefully reviewed adaptation points.

Do not import GPL/custom-license reference implementations merely because they have a desired feature. Reuse source only when its license is compatible and its notice/provenance is preserved.

## Foundation areas expected to be retained/adapted

- T-Deck Plus board/BSP configuration;
- ESP32-S3 platform integration;
- SX1262/LoRa driver path;
- LVGL/display/input/GPS board support;
- existing identity/security envelope where compatible with Firmware-mash contracts;
- current routing/reliability primitives where extension is cleaner than duplication;
- internal SPIFFS/message-persistence implementation as the starting point for Phase 1A hardening;
- host tests, gosim/simulator and emulator infrastructure;
- build/flash tooling where compatible with the future Firmware-mash release pipeline.

## Firmware-mash-owned additions/hardening

Examples include:

- transactional MessageStore rollover/compaction and durability policy;
- reboot-safe PacketId/time/conversation persistence rules;
- HybridRouter ownership seam;
- multipath/path-diversity scoring;
- custody/store-carry-forward policy;
- EnergyManager;
- ESP-NOW Normal/LR adapter and RadioScheduler integration;
- IP NetifProvider, GatewayManager and federation policy;
- smartphone UX bindings;
- release/evidence/flasher hardening.

## Notice rule

Any copied/adapted substantial Bramble source retains the upstream MIT copyright/license notice through repository notices/license provenance as appropriate. Files newly written by Firmware-mash remain covered by the project's licensing policy while respecting upstream-derived notices.

## Reference-only sources

MeshCore, Meshtastic, Reticulum, Pyxis, MeshCom and LoRaMesher may inform architecture only according to their respective licenses unless a later provenance review explicitly approves source reuse.
