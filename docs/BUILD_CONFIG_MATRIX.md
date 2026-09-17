# Build and Feature Configuration Matrix

## Goal

Every significant feature must compile in at least one CI configuration and optional features must prove they can be removed without breaking the stable core.

## Canonical configurations

### CFG-BASELINE

Purpose: reproduce pinned upstream T-Deck Plus foundation before Firmware-mash feature changes.

```text
LoRa: upstream baseline
HybridRouter: off/not yet introduced
MessageStore extension: off
Multipath: off
ESP-NOW: off
ESP-NOW LR: off
NAN: off
RF assist/backscatter: off
microSD requirement: forbidden for core acceptance
```

### CFG-LORA-STABLE

Purpose: permanent stable-core regression target.

```text
LoRa: ON
HybridRouter: ON
Internal MessageStore: ON
Reliability/dedup: ON
Multipath/failover: ON after validated
ESP-NOW: OFF
ESP-NOW LR: OFF
NAN: OFF
RF assist/backscatter: OFF
microSD: OPTIONAL only
```

### CFG-HYBRID-BETA

Purpose: compile/test hybrid local transport integration.

```text
LoRa: ON
HybridRouter: ON
Internal MessageStore: ON
Reliability/dedup: ON
Multipath/failover: ON
ESP-NOW Normal: ON
ESP-NOW LR: ON when target/IDF support is confirmed
RadioScheduler: ON
NAN: OFF
RF assist/backscatter: OFF
```

### CFG-LAB

Purpose: isolated experiments only.

May enable NAN/RF-assist/future external backscatter, but must retain ability to disable them and return to CFG-LORA-STABLE.

## Feature ownership

Feature flags control adapters/capabilities, not duplicate routing engines. Disabling ESP-NOW must not remove HybridRouter, PacketId, ReliabilityManager or MessageStore.

ESP-NOW Normal and ESP-NOW LR are modes/capabilities of the same transport adapter. Do not expose them as separate user-selected mesh networks.

## Runtime defaults

Normal end users do not select a build-time or runtime route mode manually. Safe runtime defaults:

- LoRa backbone enabled;
- automatic routing enabled;
- durable delayed delivery enabled;
- multipath enabled only after its validation gate passes;
- ESP-NOW auto mode enabled only in a build where hardware evidence supports it;
- ESP-NOW link-mode choice NORMAL/LR is automatic policy, not a required user setting;
- experimental transports disabled in STABLE.

## Compile gates

Once production code exists, CI must compile at minimum:

1. CFG-LORA-STABLE;
2. CFG-HYBRID-BETA when ESP-NOW code exists;
3. host/unit test configuration;
4. simulator configuration.

A feature hidden behind a flag is not considered covered unless at least one CI job turns that flag ON.

## Version/pinning rule

Pin and record:

- foundation commit;
- ESP-IDF version;
- component/dependency lock where applicable;
- board target/config;
- partition table version;
- packet/wire format version;
- route-score version;
- MessageStore record version.

Do not silently float the toolchain or protocol format in a stable release.